#include "internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

namespace Metriqos::Internal {

// Forward declaration (event.cpp)
nlohmann::json batchToJson(const std::vector<Event>& events);

static constexpr const char* kUserAgent = "metriqos-sdk-cpp/0.1.0";

static bool isExemptEvent(const std::string& name) {
    return name == "session_start" || name == "session_end" || name == "heartbeat";
}

void enqueueEvent(Event event) {
    auto& s = getState();
    if (s.optedOut.load()) return;

    bool shouldFlush = false;
    {
        std::lock_guard<std::mutex> lock(s.queueMutex);
        s.eventQueue.push_back(std::move(event));
        shouldFlush = static_cast<int>(s.eventQueue.size()) >= s.config.batchSize;
    }

    if (shouldFlush) {
        doFlush();
    }
}

void doFlush() {
    auto& s = getState();

    std::vector<Event> batch;
    {
        std::lock_guard<std::mutex> lock(s.queueMutex);
        if (s.eventQueue.empty()) return;
        batch.swap(s.eventQueue);
    }

    std::string url = s.config.endpoint + "/v1/events/batch";
    std::string payload = batchToJson(batch).dump();

    int attempt = 0;
    while (true) {
        auto resp = httpPost(url, payload, s.config.apiKey, kUserAgent);

        if (resp.statusCode == 202) {
            // Success
            if (!resp.quotaRemaining.empty()) {
                try {
                    s.quotaRemaining = std::stoll(resp.quotaRemaining);
                } catch (...) {}
            }
            log(LogLevel::Debug, "Flushed " + std::to_string(batch.size()) + " events");
            return;
        }

        if (resp.statusCode == 429 && !resp.body.empty()) {
            // Check if quota_exceeded
            try {
                auto bodyJson = nlohmann::json::parse(resp.body);
                if (bodyJson.contains("code") && bodyJson["code"] == "quota_exceeded") {
                    // Re-queue only exempt events
                    std::lock_guard<std::mutex> lock(s.queueMutex);
                    for (auto& e : batch) {
                        if (isExemptEvent(e.eventName)) {
                            s.eventQueue.push_back(std::move(e));
                        }
                    }
                    log(LogLevel::Warn, "Quota exceeded, dropped non-exempt events");
                    return;
                }
            } catch (...) {}
        }

        if (resp.statusCode == 400 || resp.statusCode == 413) {
            log(LogLevel::Error, "Batch rejected (HTTP " + std::to_string(resp.statusCode) + "), dropping events");
            return;
        }

        // Retryable: 429 (rate limit) or 503 or network error (statusCode == 0)
        if (resp.statusCode == 429 || resp.statusCode == 503 || resp.statusCode == 0) {
            int retryAfter = 0;
            if (!resp.retryAfter.empty()) {
                try { retryAfter = std::stoi(resp.retryAfter); } catch (...) {}
            }

            int backoffMs = calculateBackoffMs(attempt, retryAfter);
            if (backoffMs < 0) {
                if (s.offlineStorage) {
                    s.offlineStorage->store(batch);
                    log(LogLevel::Warn, "Max retries exceeded, stored " + std::to_string(batch.size()) + " events offline");
                } else {
                    log(LogLevel::Error, "Max retries exceeded, dropping " + std::to_string(batch.size()) + " events");
                }
                return;
            }

            log(LogLevel::Warn, "Retrying flush in " + std::to_string(backoffMs) + "ms (attempt " + std::to_string(attempt + 1) + ")");
            std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
            ++attempt;
            continue;
        }

        // Any other unexpected status
        log(LogLevel::Error, "Unexpected HTTP " + std::to_string(resp.statusCode) + ", dropping events");
        return;
    }
}

static void drainOfflineStorage() {
    auto& s = getState();
    if (!s.offlineStorage) return;

    while (s.running.load() && s.offlineStorage->count() > 0) {
        auto payloads = s.offlineStorage->retrieve(50);
        if (payloads.empty()) break;

        // Build a batch JSON wrapping the stored event payloads
        nlohmann::json batch;
        batch["events"] = nlohmann::json::array();
        for (const auto& p : payloads) {
            try {
                batch["events"].push_back(nlohmann::json::parse(p));
            } catch (...) {
                // Skip malformed payloads
            }
        }

        std::string url = s.config.endpoint + "/v1/events/batch";
        auto resp = httpPost(url, batch.dump(), s.config.apiKey, kUserAgent);

        if (resp.statusCode == 202) {
            s.offlineStorage->remove(static_cast<int>(payloads.size()));
            log(LogLevel::Debug, "Drained " + std::to_string(payloads.size()) + " offline events");
        } else {
            // Network still down or server error; stop draining
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void flushThreadLoop() {
    auto& s = getState();
    while (s.running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(s.config.batchIntervalMs));
        if (!s.running.load()) break;
        drainOfflineStorage();
        doFlush();
    }
}

} // namespace Metriqos::Internal
