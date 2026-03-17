#include "internal.h"
#include <random>
#include <algorithm>

namespace Metriqos::Internal {

int calculateBackoffMs(int attempt, int retryAfterSec) {
    constexpr int maxRetries = 5;
    constexpr int baseSec = 1;
    constexpr int capSec = 60;

    if (attempt >= maxRetries) {
        return -1;
    }

    int delaySec;
    if (retryAfterSec > 0) {
        delaySec = retryAfterSec;
    } else {
        delaySec = baseSec * (1 << attempt); // 1, 2, 4, 8, 16
    }

    delaySec = std::min(delaySec, capSec);

    int delayMs = delaySec * 1000;

    // Jitter: +/- 25%
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> jitter(0.75, 1.25);
    delayMs = static_cast<int>(delayMs * jitter(gen));

    return delayMs;
}

} // namespace Metriqos::Internal
