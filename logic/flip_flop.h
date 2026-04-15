#pragma once

#include <functional>
#include <unordered_map>
#include <string>

namespace deepiri {

enum class FFType {
    D,
    JK,
    T
};

class FlipFlop {
public:
    FlipFlop(FFType type);
    
    void set_d(bool value);
    void set_j(bool value);
    void set_k(bool value);
    void set_t(bool value);
    void set_clk(bool edge);
    void set_rst(bool value);
    void set_preset(bool value);
    
    bool get_q() const;
    bool get_qn() const;
    
    void clock_edge();
    void reset();
    void preset();
    
    static std::string ff_name(FFType type);

private:
    void evaluate_d();
    void evaluate_jk();
    void evaluate_t();

    FFType type_;
    bool q_ = false;
    bool qn_ = true;
    bool clk_prev_ = false;
    bool d_ = false;
    bool j_ = false;
    bool k_ = false;
    bool t_ = false;
    bool rst_ = false;
    bool preset_ = false;
};

class FlipFlopLibrary {
public:
    static FlipFlopLibrary& instance();
    FlipFlop create_flip_flop(FFType type);

private:
    FlipFlopLibrary();
};

}