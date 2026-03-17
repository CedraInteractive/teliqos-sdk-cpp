#include "internal.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace Metriqos::Internal {

std::string generateUUID() {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, 15);
    std::uniform_int_distribution<uint32_t> dist2(8, 11); // variant bits: 8,9,a,b

    const char hex[] = "0123456789abcdef";
    // xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    const char pattern[] = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";

    std::string uuid;
    uuid.reserve(36);

    for (char c : std::string(pattern)) {
        if (c == 'x') {
            uuid += hex[dist(gen)];
        } else if (c == 'y') {
            uuid += hex[dist2(gen)];
        } else {
            uuid += c; // '-' or '4'
        }
    }

    return uuid;
}

std::string nowISO8601() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << 'Z';
    return oss.str();
}

} // namespace Metriqos::Internal
