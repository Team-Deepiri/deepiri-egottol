#pragma once

#include <queue>
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace deepiri {

using TimePoint = std::chrono::steady_clock::time_point;
using SimTime = std::chrono::nanoseconds;

struct LogicEvent {
    size_t signal_id;
    bool value;
    TimePoint time;

    bool operator<(const LogicEvent& other) const {
        return time > other.time;
    }
};

class EventQueue {
public:
    void schedule(size_t signal_id, bool value, SimTime delay);
    void schedule_absolute(size_t signal_id, bool value, TimePoint at);
    bool process_next();
    std::vector<LogicEvent> peek_ready() const;
    bool empty() const;
    void advance_time(SimTime delta);
    TimePoint current_time() const;
    void clear();

private:
    std::priority_queue<LogicEvent> queue_;
    TimePoint current_time_{std::chrono::steady_clock::now()};
};

}