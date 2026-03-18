#include "internal.h"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace Metriqos::Internal {

// Forward declaration (event.cpp)
nlohmann::json eventToJson(const Event& e);

OfflineStorage::OfflineStorage(const std::string& path, int maxQueueSize)
    : maxSize(maxQueueSize) {
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db);
        sqlite3_close(db);
        db = nullptr;
        throw std::runtime_error("Failed to open offline DB: " + err);
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  payload TEXT NOT NULL,"
        "  created_at INTEGER DEFAULT (strftime('%s','now'))"
        ")";
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create events table: " + err);
    }
}

OfflineStorage::~OfflineStorage() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void OfflineStorage::store(const std::vector<Event>& events) {
    if (!db || events.empty()) return;

    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO events (payload) VALUES (?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    for (const auto& e : events) {
        std::string payload = eventToJson(e).dump();
        sqlite3_bind_text(stmt, 1, payload.c_str(),
                          static_cast<int>(payload.size()), SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);

    trimToSize();
}

std::vector<std::string> OfflineStorage::retrieve(int limit) {
    std::vector<std::string> results;
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT payload FROM events ORDER BY id ASC LIMIT ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            results.emplace_back(text);
        }
    }

    sqlite3_finalize(stmt);
    return results;
}

void OfflineStorage::remove(int count) {
    if (!db || count <= 0) return;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "DELETE FROM events WHERE id IN ("
        "  SELECT id FROM events ORDER BY id ASC LIMIT ?"
        ")";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    sqlite3_bind_int(stmt, 1, count);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void OfflineStorage::wipeNonExempt() {
    if (!db) return;

    const char* sql =
        "DELETE FROM events WHERE "
        "payload NOT LIKE '%\"eventName\":\"session_start\"%' AND "
        "payload NOT LIKE '%\"eventName\":\"session_end\"%' AND "
        "payload NOT LIKE '%\"eventName\":\"heartbeat\"%'";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

int OfflineStorage::count() {
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM events";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return result;
}

void OfflineStorage::trimToSize() {
    int current = count();
    if (current > maxSize) {
        remove(current - maxSize);
    }
}

} // namespace Metriqos::Internal
