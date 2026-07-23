#include "rtl_shadow_sim.h"

namespace deepiri {

RtlShadowFallbackSim::RtlShadowFallbackSim(std::vector<GateSpec> gates,
                                            std::vector<DffSpec> dffs,
                                            std::vector<std::string> output_nets)
    : gates_(std::move(gates)), dffs_(std::move(dffs)), output_nets_(std::move(output_nets)) {}

size_t RtlShadowFallbackSim::net_id(const std::string& net) {
    auto it = net_ids_.find(net);
    if (it != net_ids_.end()) return it->second;
    size_t id = id_to_net_.size();
    id_to_net_.push_back(net);
    net_ids_.emplace(net, id);
    return id;
}

void RtlShadowFallbackSim::set_input(const std::string& net, bool value) {
    net_values_[net] = value;
}

bool RtlShadowFallbackSim::get_net(const std::string& net) const {
    auto it = net_values_.find(net);
    return it != net_values_.end() && it->second;
}

void RtlShadowFallbackSim::eval_combinational() {
    // Evaluate every gate against the state settled by the *previous*
    // tick (matching the Python fallback's single-pass _solve_logic),
    // then drain the queue to commit results — multi-level combinational
    // nets need one extra tick per level to fully propagate, same as
    // the original EventDrivenSimulator-backed implementation.
    event_queue_.clear();
    std::unordered_map<size_t, bool> pending;
    for (const auto& g : gates_) {
        bool a = get_net(g.net_a);
        bool b = get_net(g.net_b);
        auto func = LogicGate::get_function(g.type);
        bool result = func({a, b});
        size_t id = net_id(g.net_out);
        event_queue_.schedule(id, result, SimTime(1));
        pending[id] = result;
    }
    while (event_queue_.process_next()) {
        // process_next() only advances the queue's clock; the value to
        // commit for each drained signal_id was captured above.
    }
    for (const auto& [id, value] : pending) {
        net_values_[id_to_net_[id]] = value;
    }
}

void RtlShadowFallbackSim::tick() {
    clk_ = !clk_;
    if (clk_) {
        for (const auto& dff : dffs_) {
            net_values_[dff.net_q] = get_net(dff.net_d);
        }
    } else {
        eval_combinational();
    }
}

std::unordered_map<std::string, bool> RtlShadowFallbackSim::outputs() const {
    std::unordered_map<std::string, bool> result;
    const std::vector<std::string>* keys = &output_nets_;
    std::vector<std::string> all_nets;
    if (keys->empty()) {
        all_nets.reserve(net_values_.size());
        for (const auto& [net, _] : net_values_) all_nets.push_back(net);
        keys = &all_nets;
    }
    for (const auto& net : *keys) {
        result[net] = get_net(net);
    }
    return result;
}

}  // namespace deepiri
