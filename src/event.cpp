#include "internal.h"
#include <nlohmann/json.hpp>

namespace Metriqos::Internal {

nlohmann::json eventToJson(const Event& e) {
    nlohmann::json j;
    j["eventName"] = e.eventName;
    j["sessionId"] = e.sessionId;
    j["playerId"] = e.playerId;
    j["timestamp"] = e.timestamp;

    if (!e.category.empty()) {
        j["category"] = e.category;
    }
    if (!e.appVersion.empty()) {
        j["appVersion"] = e.appVersion;
    }
    if (!e.mapId.empty()) {
        j["mapId"] = e.mapId;
    }
    if (!e.nums.empty()) {
        j["nums"] = e.nums;
    }
    if (!e.strs.empty()) {
        j["strs"] = e.strs;
    }
    if (!e.tags.empty()) {
        j["tags"] = e.tags;
    }
    if (e.hasPos) {
        j["pos"] = { e.pos[0], e.pos[1], e.pos[2] };
    }
    if (!e.device.empty()) {
        j["device"] = e.device;
    }

    return j;
}

nlohmann::json batchToJson(const std::vector<Event>& events) {
    nlohmann::json j;
    j["events"] = nlohmann::json::array();
    for (const auto& e : events) {
        j["events"].push_back(eventToJson(e));
    }
    return j;
}

} // namespace Metriqos::Internal
