#pragma once

#include <vector>
#include <string>
#include <functional>

namespace deepiri {

enum class BufferMode {
    passthrough,
    delay,
    hold,
    invert
};

class SignalBuffer {
public:
    SignalBuffer(size_t width = 1);
    
    void write(const std::vector<bool>& data);
    std::vector<bool> read() const;
    void write_bit(size_t index, bool value);
    bool read_bit(size_t index) const;
    
    void set_mode(BufferMode mode);
    void set_delay_ns(double delay);
    BufferMode get_mode() const;
    size_t width() const;
    
    void clock();
    void reset();

private:
    std::vector<bool> data_;
    std::vector<bool> prev_data_;
    size_t width_;
    BufferMode mode_;
    double delay_ns_;
};

class SignalBus {
public:
    SignalBus(const std::string& name, size_t width = 1);
    
    void write(const std::vector<bool>& data);
    std::vector<bool> read() const;
    
    std::string name() const;
    size_t width() const;
    size_t id() const;

private:
    std::string name_;
    size_t width_;
    size_t id_;
    std::vector<bool> data_;

public:
    void set_id(size_t id);
};

class SignalManager {
public:
    static SignalManager& instance();
    
    size_t create_signal(const std::string& name, size_t width = 1);
    SignalBus* get_signal(size_t id);
    void write(size_t id, const std::vector<bool>& data);
    std::vector<bool> read(size_t id) const;
    std::string signal_name(size_t id) const;

private:
    SignalManager();
    std::vector<SignalBus> signals_;
    size_t next_id_;
};

}