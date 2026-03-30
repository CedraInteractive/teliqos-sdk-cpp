#include "internal.h"

namespace Teliqos::Internal {

void startSession() {
    auto& s = getState();
    s.sessionId = generateUUID();
    s.sessionStartTime = std::chrono::steady_clock::now();

    Event e;
    e.eventName = "session_start";
    e.sessionId = s.sessionId;
    e.playerId = s.playerId;
    e.deviceId = s.deviceId;
    e.timestamp = nowISO8601();
    e.appVersion = s.config.appVersion;
    e.device = s.deviceInfo;

    enqueueEvent(std::move(e));
    log(LogLevel::Debug, "Session started: " + s.sessionId);
}

void endSession() {
    auto& s = getState();
    if (s.sessionId.empty()) return;

    Event e;
    e.eventName = "session_end";
    e.sessionId = s.sessionId;
    e.playerId = s.playerId;
    e.deviceId = s.deviceId;
    e.timestamp = nowISO8601();
    e.appVersion = s.config.appVersion;

    auto elapsed = std::chrono::steady_clock::now() - s.sessionStartTime;
    double durationSec = std::chrono::duration<double>(elapsed).count();
    e.nums["duration_sec"] = durationSec;

    enqueueEvent(std::move(e));
    log(LogLevel::Debug, "Session ended: " + s.sessionId);
}

void sendHeartbeat() {
    auto& s = getState();
    if (s.sessionId.empty()) return;

    Event e;
    e.eventName = "heartbeat";
    e.sessionId = s.sessionId;
    e.playerId = s.playerId;
    e.deviceId = s.deviceId;
    e.timestamp = nowISO8601();
    e.appVersion = s.config.appVersion;

    enqueueEvent(std::move(e));
}

void heartbeatThreadLoop() {
    auto& s = getState();
    while (s.running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(s.config.heartbeatIntervalSec));
        if (!s.running.load()) break;
        sendHeartbeat();
    }
}

} // namespace Teliqos::Internal
