#include "signal_buffer.h"

namespace deepiri {

SignalBuffer::SignalBuffer(size_t width) 
    : width_(width), mode_(BufferMode::passthrough), delay_ns_(0) {
    data_.resize(width, false);
    prev_data_.resize(width, false);
}

void SignalBuffer::write(const std::vector<bool>& data) {
    if (data.size() != width_) return;
    prev_data_ = data_;
    data_ = data;
}

std::vector<bool> SignalBuffer::read() const {
    return data_;
}

void SignalBuffer::write_bit(size_t index, bool value) {
    if (index >= width_) return;
    prev_data_[index] = data_[index];
    data_[index] = value;
}

bool SignalBuffer::read_bit(size_t index) const {
    if (index >= width_) return false;
    return data_[index];
}

void SignalBuffer::set_mode(BufferMode mode) {
    mode_ = mode;
}

void SignalBuffer::set_delay_ns(double delay) {
    delay_ns_ = delay;
}

BufferMode SignalBuffer::get_mode() const {
    return mode_;
}

size_t SignalBuffer::width() const {
    return width_;
}

void SignalBuffer::clock() {
    switch (mode_) {
        case BufferMode::invert:
            for (size_t i = 0; i < width_; ++i) {
                data_[i] = !data_[i];
            }
            break;
        default:
            break;
    }
}

void SignalBuffer::reset() {
    for (size_t i = 0; i < width_; ++i) {
        data_[i] = false;
        prev_data_[i] = false;
    }
}

SignalBus::SignalBus(const std::string& name, size_t width)
    : name_(name), width_(width), id_(0), data_(width, false) {}

void SignalBus::write(const std::vector<bool>& data) {
    if (data.size() == width_) {
        data_ = data;
    }
}

std::vector<bool> SignalBus::read() const {
    return data_;
}

std::string SignalBus::name() const {
    return name_;
}

size_t SignalBus::width() const {
    return width_;
}

size_t SignalBus::id() const {
    return id_;
}

void SignalBus::set_id(size_t id) {
    id_ = id;
}

SignalManager& SignalManager::instance() {
    static SignalManager instance;
    return instance;
}

size_t SignalManager::create_signal(const std::string& name, size_t width) {
    SignalBus bus(name, width);
    bus.set_id(next_id_++);
    signals_.push_back(bus);
    return bus.id();
}

SignalBus* SignalManager::get_signal(size_t id) {
    for (auto& bus : signals_) {
        if (bus.id() == id) return &bus;
    }
    return nullptr;
}

void SignalManager::write(size_t id, const std::vector<bool>& data) {
    auto* bus = get_signal(id);
    if (bus) bus->write(data);
}

std::vector<bool> SignalManager::read(size_t id) const {
    for (const auto& bus : signals_) {
        if (bus.id() == id) return bus.read();
    }
    return {};
}

std::string SignalManager::signal_name(size_t id) const {
    for (const auto& bus : signals_) {
        if (bus.id() == id) return bus.name();
    }
    return "";
}

SignalManager::SignalManager() : next_id_(0) {}

}