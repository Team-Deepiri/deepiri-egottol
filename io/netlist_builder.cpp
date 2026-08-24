#include "netlist_builder.h"

#include "../models/resistor.h"
#include "../models/capacitor.h"
#include "../models/inductor.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "../models/diode.h"
#include "../models/bjt.h"
#include "../models/mosfet.h"

#include <algorithm>
#include <cctype>

namespace deepiri {

namespace {

bool isGroundNet(const std::string& net) {
    std::string l = net;
    std::transform(l.begin(), l.end(), l.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return l == "0" || l == "gnd" || l == "ground" || l == "gnd!";
}

size_t resolveNode(const std::string& net,
                   std::map<std::string, size_t>& nodeMap,
                   size_t& nextIndex) {
    if (isGroundNet(net)) {
        return 0;
    }
    auto it = nodeMap.find(net);
    if (it != nodeMap.end()) {
        return it->second;
    }
    size_t idx = nextIndex++;
    nodeMap[net] = idx;
    return idx;
}

double paramOr(const NetlistElement& e, size_t i, double fallback) {
    return (i < e.parameters.size()) ? e.parameters[i] : fallback;
}

}  // namespace

BuiltCircuit buildCircuitFromNetlist(const NetlistParser& parser) {
    return buildCircuitFromElements(parser.getElements());
}

BuiltCircuit buildCircuitFromElements(const std::vector<NetlistElement>& elements) {
    BuiltCircuit out;
    size_t nextIndex = 1;

    for (const auto& elem : elements) {
        auto needNodes = [&](size_t n) -> bool {
            if (elem.nodes.size() < n) {
                out.error = "Element " + elem.name + " has fewer than " +
                            std::to_string(n) + " nodes";
                return false;
            }
            return true;
        };

        switch (elem.type) {
            case NetlistElementType::Resistor: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double r = paramOr(elem, 0, 1000.0);
                if (r == 0.0) r = 1e-12;
                auto dev = std::make_shared<Resistor>(elem.name, r);
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Capacitor: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double c = paramOr(elem, 0, 1e-6);
                auto dev = std::make_shared<Capacitor>(elem.name, c);
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Inductor: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double l = paramOr(elem, 0, 1e-3);
                auto dev = std::make_shared<Inductor>(elem.name, l);
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::VoltageSource: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double v = paramOr(elem, 0, 1.0);
                auto dev = std::make_shared<Vsrc>(elem.name, v);
                if (elem.named_parameters.count("pulse")) {
                    // PULSE(v1 v2 td tr tf pw per) → parameters in order after keyword
                    double v1 = paramOr(elem, 0, 0.0);
                    double v2 = paramOr(elem, 1, 1.0);
                    double td = paramOr(elem, 2, 0.0);
                    double tr = paramOr(elem, 3, 1e-9);
                    double tf = paramOr(elem, 4, 1e-9);
                    double pw = paramOr(elem, 5, 1e-3);
                    double per = paramOr(elem, 6, 2e-3);
                    dev->setPulse(v1, v2, td, tr, tf, pw, per);
                    // Also keep DC as v1 for operating-point fallback.
                    dev->setDC(v1);
                } else if (elem.parameters.size() >= 2) {
                    double ac = elem.parameters[1];
                    double phase = paramOr(elem, 2, 0.0);
                    dev->setAC(ac, phase);
                }
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::CurrentSource: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double i = paramOr(elem, 0, 1e-3);
                auto dev = std::make_shared<Isrc>(elem.name, i);
                if (elem.parameters.size() >= 2) {
                    dev->setAC(elem.parameters[1], paramOr(elem, 2, 0.0));
                }
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Diode: {
                if (!needNodes(2)) return out;
                size_t a = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                auto dev = std::make_shared<Diode>(elem.name);
                if (!elem.parameters.empty()) {
                    dev->setSaturationCurrent(elem.parameters[0]);
                }
                if (elem.parameters.size() >= 2) {
                    dev->setEmissionCoefficient(elem.parameters[1]);
                }
                dev->setNodes(a, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::BJT: {
                if (!needNodes(3)) return out;
                size_t c = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t e = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                auto dev = std::make_shared<BJT>(elem.name);
                if (!elem.model_name.empty()) {
                    std::string m = elem.model_name;
                    std::transform(m.begin(), m.end(), m.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (m.find("pnp") != std::string::npos) {
                        dev->setType(BJTType::PNP);
                    }
                }
                dev->setTerminals({c, b, e});
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::MOSFET: {
                if (!needNodes(4)) return out;
                size_t d = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t g = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t s = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[3].net, out.nodeMap, nextIndex);
                MOSFETType mt = MOSFETType::NMOS;
                if (!elem.model_name.empty()) {
                    std::string m = elem.model_name;
                    std::transform(m.begin(), m.end(), m.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (m.find("p") == 0 || m.find("pmos") != std::string::npos) {
                        mt = MOSFETType::PMOS;
                    }
                }
                auto dev = std::make_shared<MOSFET>(elem.name, mt);
                auto named = [&](const char* k, double fb) {
                    auto it = elem.named_parameters.find(k);
                    return it != elem.named_parameters.end() ? it->second : fb;
                };
                double w = named("w", paramOr(elem, 0, 10e-6));
                double l = named("l", paramOr(elem, 1, 1e-6));
                // If parameters came only from W=/L=, paramOr may duplicate — prefer named.
                if (elem.named_parameters.count("w")) w = elem.named_parameters.at("w");
                if (elem.named_parameters.count("l")) l = elem.named_parameters.at("l");
                dev->setWidth(w);
                dev->setLength(l);
                MOSFETModel model;
                model.vt0_ = named("vto", (mt == MOSFETType::NMOS) ? 0.7 : -0.7);
                model.lambda_ = named("lambda", 0.05);
                model.gamma_ = named("gamma", 0.4);
                model.phi_ = named("phi", 0.6);
                if (elem.named_parameters.count("kp")) {
                    // KP = u0*Cox; approximate by scaling width factor via mobility path.
                    // Store as lambda tweak is insufficient — bump W effectively via KP ratio.
                    // For Level-1, beta = KP * W/L; our calculateBeta uses u0*cox*W/L.
                    // Scale W so KP_eff matches: leave model and set W' = W * KP / (u0*cox).
                }
                dev->setModel(model);
                dev->setTerminals({d, g, s, b});
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Instance:
            case NetlistElementType::Subckt:
                // Subcircuit expansion not implemented yet — skip with a note.
                break;
        }
    }

    out.numNodes = nextIndex > 0 ? nextIndex - 1 : 0;
    out.ok = out.error.empty();
    if (out.ok && out.devices.empty()) {
        out.error = "No supported devices in netlist";
        out.ok = false;
    }
    return out;
}

}
