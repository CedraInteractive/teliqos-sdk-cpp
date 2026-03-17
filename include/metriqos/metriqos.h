#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace Metriqos {

struct Vec3 { float x = 0, y = 0, z = 0; };

struct EventData {
    std::unordered_map<std::string, double> nums;
    std::unordered_map<std::string, std::string> strs;
    std::vector<std::string> tags;
    Vec3 pos;
    bool hasPos = false;
    std::string mapId;
    std::string category;
};

enum class LogLevel { None, Error, Warn, Debug };

struct Config {
    std::string apiKey;
    std::string appVersion;
    std::string endpoint = "https://api.metriqos.io";
    int batchIntervalMs = 30000;
    int batchSize = 50;
    bool collectDeviceInfo = true;
    bool debug = false;
    int offlineQueueSize = 500;
    int heartbeatIntervalSec = 60;
    LogLevel logLevel = LogLevel::Warn;
    std::function<void(LogLevel, const std::string&)> logCallback = nullptr;
};

void init(const Config& config);
void shutdown();
void identify(const std::string& playerId);
void setUserProperty(const std::string& key, const std::string& value);
void setUserProperty(const std::string& key, double value);
void track(const std::string& eventName, const EventData& data = {});
void setOptOut(bool optOut);
void flush();

struct Status {
    int queuedEvents;
    int offlineEvents;
    int64_t quotaRemaining;
    bool connected;
};
Status getStatus();

} // namespace Metriqos
