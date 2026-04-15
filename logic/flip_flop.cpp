#include "flip_flop.h"

namespace deepiri {

FlipFlop::FlipFlop(FFType type) : type_(type) {}

void FlipFlop::set_d(bool value) {
    d_ = value;
}

void FlipFlop::set_j(bool value) {
    j_ = value;
}

void FlipFlop::set_k(bool value) {
    k_ = value;
}

void FlipFlop::set_t(bool value) {
    t_ = value;
}

void FlipFlop::set_clk(bool edge) {
    if (edge && !clk_prev_) {
        clock_edge();
    }
    clk_prev_ = edge;
}

void FlipFlop::set_rst(bool value) {
    rst_ = value;
    if (rst_) reset();
}

void FlipFlop::set_preset(bool value) {
    preset_ = value;
    if (preset_) preset();
}

bool FlipFlop::get_q() const {
    return q_;
}

bool FlipFlop::get_qn() const {
    return qn_;
}

void FlipFlop::clock_edge() {
    if (rst_) return;
    
    switch (type_) {
        case FFType::D:
            evaluate_d();
            break;
        case FFType::JK:
            evaluate_jk();
            break;
        case FFType::T:
            evaluate_t();
            break;
    }
}

void FlipFlop::reset() {
    q_ = false;
    qn_ = true;
}

void FlipFlop::preset() {
    q_ = true;
    qn_ = false;
}

void FlipFlop::evaluate_d() {
    q_ = d_;
    qn_ = !d_;
}

void FlipFlop::evaluate_jk() {
    if (j_ && k_) {
        q_ = !q_;
    } else if (j_ && !k_) {
        q_ = true;
    } else if (!j_ && k_) {
        q_ = false;
    }
    qn_ = !q_;
}

void FlipFlop::evaluate_t() {
    if (t_) {
        q_ = !q_;
        qn_ = !q_;
    }
}

std::string FlipFlop::ff_name(FFType type) {
    switch (type) {
        case FFType::D: return "D";
        case FFType::JK: return "JK";
        case FFType::T: return "T";
    }
    return "UNKNOWN";
}

FlipFlopLibrary& FlipFlopLibrary::instance() {
    static FlipFlopLibrary instance;
    return instance;
}

FlipFlop FlipFlopLibrary::create_flip_flop(FFType type) {
    return FlipFlop(type);
}

FlipFlopLibrary::FlipFlopLibrary() {}

}