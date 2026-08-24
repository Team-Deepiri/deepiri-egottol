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
                // Optional AC mag as second param.
                if (elem.parameters.size() >= 2) {
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
                // Device base class is 2-terminal; stamp collector-emitter
                // for a first-order path. Full 3-terminal MNA is future work.
                size_t c = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t /*b*/ _b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t e = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                (void)_b;
                auto dev = std::make_shared<BJT>(elem.name);
                dev->setNodes(c, e);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::MOSFET: {
                if (!needNodes(4)) return out;
                size_t d = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t /*g*/ _g = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t s = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                size_t /*b*/ _b = resolveNode(elem.nodes[3].net, out.nodeMap, nextIndex);
                (void)_g;
                (void)_b;
                auto dev = std::make_shared<MOSFET>(elem.name);
                if (elem.parameters.size() >= 1) {
                    // W= / L= often appear as params; treat first two numbers as W, L.
                    dev->setWidth(elem.parameters[0]);
                }
                if (elem.parameters.size() >= 2) {
                    dev->setLength(elem.parameters[1]);
                }
                dev->setNodes(d, s);
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
