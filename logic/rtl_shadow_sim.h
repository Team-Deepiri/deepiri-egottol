#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "event_queue.h"
#include "logic_gate.h"

namespace deepiri {

// Mirrors the gate-list built by egottol/engines/rtl_shadow.py's
// CycleAccurateFallback.from_verilog(): each `assign out = a & b;` /
// `assign out = a | b;` line becomes one GateSpec, keyed by net name
// (not by a generic Component/Wire graph) so it can be fed straight
// from a future native Verilog-netlist exporter.
struct GateSpec {
    GateType type;  // GateType::AND or GateType::OR
    std::string net_a;
    std::string net_b;
    std::string net_out;
};

// Mirrors `always @(posedge clk) q <= d;`.
struct DffSpec {
    std::string net_q;
    std::string net_d;
};

// Native port of CycleAccurateFallback. tick() reproduces the Python
// original's half-period stepping: odd ticks are the clock's posedge
// (DFFs latch D into Q), even ticks are the negedge (combinational
// AND/OR nets settle one propagation level). A full clock cycle is
// therefore two tick() calls, exactly as in the Python fallback.
class RtlShadowFallbackSim {
public:
    RtlShadowFallbackSim(std::vector<GateSpec> gates,
                          std::vector<DffSpec> dffs,
                          std::vector<std::string> output_nets);

    void set_input(const std::string& net, bool value);
    bool get_net(const std::string& net) const;

    void tick();

    std::unordered_map<std::string, bool> outputs() const;

private:
    void eval_combinational();
    size_t net_id(const std::string& net);

    std::vector<GateSpec> gates_;
    std::vector<DffSpec> dffs_;
    std::vector<std::string> output_nets_;

    std::unordered_map<std::string, bool> net_values_;
    std::unordered_map<std::string, size_t> net_ids_;
    std::vector<std::string> id_to_net_;

    EventQueue event_queue_;
    bool clk_ = false;
};

}  // namespace deepiri
