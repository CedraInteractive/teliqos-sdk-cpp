#pragma once
#include "teliqos/teliqos.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <memory>

struct sqlite3;

namespace Teliqos::Internal {

struct Event {
    std::string eventName;
    std::string category;
    std::string sessionId;
    std::string playerId;
    std::string deviceId;
    std::string timestamp;
    std::unordered_map<std::string, double> nums;
    std::unordered_map<std::string, std::string> strs;
    std::vector<std::string> tags;
    float pos[3] = {0, 0, 0};
    bool hasPos = false;
    std::string mapId;
    std::unordered_map<std::string, std::string> device;
    std::string appVersion;
};

// storage.cpp
class OfflineStorage {
    sqlite3* db = nullptr;
    int maxSize;
public:
    OfflineStorage(const std::string& path, int maxQueueSize);
    ~OfflineStorage();

    void store(const std::vector<Event>& events);
    std::vector<std::string> retrieve(int limit);
    void remove(int count);
    void wipeNonExempt();
    int count();

private:
    void trimToSize();
};

struct State {
    Config config;
    std::string playerId;
    std::string deviceId;
    std::string sessionId;
    std::chrono::steady_clock::time_point sessionStartTime;
    std::unordered_map<std::string, std::string> userStrProps;
    std::unordered_map<std::string, double> userNumProps;

    std::mutex queueMutex;
    std::vector<Event> eventQueue;

    std::atomic<bool> running{false};
    std::atomic<bool> optedOut{false};
    std::thread flushThread;
    std::thread heartbeatThread;

    int64_t quotaRemaining = -1;
    int retryAfterSec = 0;

    std::unordered_map<std::string, std::string> deviceInfo;

    std::unique_ptr<OfflineStorage> offlineStorage;
};

State& getState();

// uuid.cpp
std::string generateUUID();
std::string nowISO8601();

// client.cpp
void log(LogLevel level, const std::string& msg);

// transport.cpp
struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string retryAfter;
    std::string quotaRemaining;
};

HttpResponse httpPost(const std::string& url, const std::string& body,
                      const std::string& apiKey, const std::string& userAgent);

// retry.cpp
int calculateBackoffMs(int attempt, int retryAfterSec);

// batcher.cpp
void enqueueEvent(Event event);
void doFlush();
void flushThreadLoop();

// session.cpp
void startSession();
void endSession();
void sendHeartbeat();
void heartbeatThreadLoop();

// device.cpp
std::unordered_map<std::string, std::string> collectDeviceInfo();
std::string getHardwareId();

// storage.cpp (settings)
std::string getSetting(const std::string& dbPath, const std::string& key);
void setSetting(const std::string& dbPath, const std::string& key, const std::string& value);

} // namespace Teliqos::Internal
