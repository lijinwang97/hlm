#include "hlm_time.h"
#include <chrono>

int64_t getCurrentTimeInMicroseconds() {
    auto now = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}
