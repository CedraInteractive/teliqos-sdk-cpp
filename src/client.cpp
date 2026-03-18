#include "internal.h"
#include <curl/curl.h>
#include <cstdio>

namespace Metriqos {

namespace Internal {

static State globalState;

State& getState() {
    return globalState;
}

void log(LogLevel level, const std::string& msg) {
    auto& s = getState();
    if (level > s.config.logLevel) return;
    if (s.config.logCallback) {
        s.config.logCallback(level, msg);
    } else if (s.config.debug) {
        fprintf(stderr, "[metriqos] %s\n", msg.c_str());
    }
}

} // namespace Internal

void init(const Config& config) {
    auto& s = Internal::getState();
    s.config = config;
    s.playerId = Internal::generateUUID();
    s.optedOut.store(false);
    s.quotaRemaining = -1;
    s.retryAfterSec = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize offline storage
    try {
        s.offlineStorage = std::make_unique<Internal::OfflineStorage>(
            "metriqos_offline.db", config.offlineQueueSize);
    } catch (const std::exception& e) {
        Internal::log(LogLevel::Error, std::string("Failed to init offline storage: ") + e.what());
    }

    if (config.collectDeviceInfo) {
        s.deviceInfo = Internal::collectDeviceInfo();
    }

    s.running.store(true);
    Internal::startSession();

    s.flushThread = std::thread(Internal::flushThreadLoop);
    s.heartbeatThread = std::thread(Internal::heartbeatThreadLoop);

    Internal::log(LogLevel::Debug, "SDK initialized");
}

void shutdown() {
    auto& s = Internal::getState();
    if (!s.running.load()) return;

    s.running.store(false);

    Internal::endSession();
    Internal::doFlush();

    if (s.flushThread.joinable()) s.flushThread.join();
    if (s.heartbeatThread.joinable()) s.heartbeatThread.join();

    s.offlineStorage.reset();

    curl_global_cleanup();
    Internal::log(LogLevel::Debug, "SDK shut down");
}

void identify(const std::string& playerId) {
    auto& s = Internal::getState();
    s.playerId = playerId;
    Internal::log(LogLevel::Debug, "Player identified: " + playerId);
}

void setUserProperty(const std::string& key, const std::string& value) {
    auto& s = Internal::getState();
    s.userStrProps[key] = value;
}

void setUserProperty(const std::string& key, double value) {
    auto& s = Internal::getState();
    s.userNumProps[key] = value;
}

void track(const std::string& eventName, const EventData& data) {
    auto& s = Internal::getState();

    Internal::Event e;
    e.eventName = eventName;
    e.sessionId = s.sessionId;
    e.playerId = s.playerId;
    e.timestamp = Internal::nowISO8601();
    e.appVersion = s.config.appVersion;
    e.category = data.category;
    e.mapId = data.mapId;

    // Merge user-level string properties, then event-level overrides
    e.strs = s.userStrProps;
    for (const auto& [k, v] : data.strs) {
        e.strs[k] = v;
    }

    // Merge user-level numeric properties, then event-level overrides
    e.nums = s.userNumProps;
    for (const auto& [k, v] : data.nums) {
        e.nums[k] = v;
    }

    e.tags = data.tags;

    if (data.hasPos) {
        e.hasPos = true;
        e.pos[0] = data.pos.x;
        e.pos[1] = data.pos.y;
        e.pos[2] = data.pos.z;
    }

    if (s.config.collectDeviceInfo) {
        e.device = s.deviceInfo;
    }

    Internal::enqueueEvent(std::move(e));
}

void setOptOut(bool optOut) {
    auto& s = Internal::getState();
    s.optedOut.store(optOut);
    Internal::log(LogLevel::Debug, std::string("Opt-out set to ") + (optOut ? "true" : "false"));
}

void flush() {
    Internal::doFlush();
}

Status getStatus() {
    auto& s = Internal::getState();
    Status st;
    {
        std::lock_guard<std::mutex> lock(s.queueMutex);
        st.queuedEvents = static_cast<int>(s.eventQueue.size());
    }
    st.offlineEvents = s.offlineStorage ? s.offlineStorage->count() : 0;
    st.quotaRemaining = s.quotaRemaining;
    st.connected = true; // simplified for now
    return st;
}

} // namespace Metriqos
