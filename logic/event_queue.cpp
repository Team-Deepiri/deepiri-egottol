#include "event_queue.h"

namespace deepiri {

void EventQueue::schedule(size_t signal_id, bool value, SimTime delay) {
    schedule_absolute(signal_id, value, current_time_ + delay);
}

void EventQueue::schedule_absolute(size_t signal_id, bool value, TimePoint at) {
    queue_.push({signal_id, value, at});
}

bool EventQueue::process_next() {
    if (queue_.empty()) return false;
    
    auto event = queue_.top();
    if (event.time > current_time_) {
        current_time_ = event.time;
    }
    queue_.pop();
    return true;
}

std::vector<LogicEvent> EventQueue::peek_ready() const {
    std::vector<LogicEvent> ready;
    if (queue_.empty()) return ready;
    
    auto temp = queue_;
    TimePoint earliest = temp.top().time;
    
    while (!temp.empty() && temp.top().time == earliest) {
        ready.push_back(temp.top());
        temp.pop();
    }
    return ready;
}

bool EventQueue::empty() const {
    return queue_.empty();
}

void EventQueue::advance_time(SimTime delta) {
    current_time_ += delta;
}

TimePoint EventQueue::current_time() const {
    return current_time_;
}

void EventQueue::clear() {
    while (!queue_.empty()) queue_.pop();
}

}