#pragma once
#include "metriqos/metriqos.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>

namespace Metriqos::Internal {

struct Event {
    std::string eventName;
    std::string category;
    std::string sessionId;
    std::string playerId;
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

struct State {
    Config config;
    std::string playerId;
    std::string sessionId;
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
};

State& getState();

std::string generateUUID();
std::string nowISO8601();
void log(LogLevel level, const std::string& msg);

} // namespace Metriqos::Internal
