#include "netlist_builder.h"

#include "../models/resistor.h"
#include "../models/capacitor.h"
#include "../models/inductor.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "../models/diode.h"
#include "../models/bjt.h"
#include "../models/mosfet.h"
#include "../models/vcvs.h"
#include "../models/vccs.h"
#include "../models/source_signal.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

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
    return buildCircuitFromElements(parser.expandedElements(), parser.getModels());
}

BuiltCircuit buildCircuitFromElements(
    const std::vector<NetlistElement>& elements,
    const std::map<std::string, SpiceModel>& models
) {
    BuiltCircuit out;
    size_t nextIndex = 1;

    auto lookupModel = [&](const std::string& name) -> const SpiceModel* {
        if (name.empty()) return nullptr;
        std::string key = name;
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        auto it = models.find(key);
        return it == models.end() ? nullptr : &it->second;
    };

    auto modelParam = [](const SpiceModel* m, const char* key, double fb) {
        if (!m) return fb;
        auto it = m->params.find(key);
        return it != m->params.end() ? it->second : fb;
    };

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
                SourceSignal sig;
                sig.dc = v;
                applyWaveformFromParams(sig, elem.named_parameters, elem.parameters);
                dev->setSignal(sig);
                if (sig.kind == SourceWaveform::DC && elem.parameters.size() >= 2 &&
                    !elem.named_parameters.count("pulse") && !elem.named_parameters.count("sin") &&
                    !elem.named_parameters.count("exp") && !elem.named_parameters.count("pwl")) {
                    // V1 n1 n2 DC 5 AC 1 0  — or V1 n1 n2 5 AC 1
                    // If "ac" flagged, params may be DC then AC; already handled.
                    // Legacy: two numbers without keyword → DC + AC mag
                    if (!elem.named_parameters.count("ac")) {
                        dev->setDC(elem.parameters[0]);
                        dev->setAC(elem.parameters[1], paramOr(elem, 2, 0.0));
                    }
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
                SourceSignal sig;
                sig.dc = i;
                applyWaveformFromParams(sig, elem.named_parameters, elem.parameters);
                dev->setSignal(sig);
                if (sig.kind == SourceWaveform::DC && elem.parameters.size() >= 2 &&
                    !elem.named_parameters.count("pulse") && !elem.named_parameters.count("sin") &&
                    !elem.named_parameters.count("exp") && !elem.named_parameters.count("pwl") &&
                    !elem.named_parameters.count("ac")) {
                    dev->setDC(elem.parameters[0]);
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
                const SpiceModel* mod = lookupModel(elem.model_name);
                double is = modelParam(mod, "is", paramOr(elem, 0, 1e-14));
                double n = modelParam(mod, "n", paramOr(elem, 1, 1.0));
                auto itIs = elem.named_parameters.find("is");
                if (itIs != elem.named_parameters.end()) is = itIs->second;
                auto itN = elem.named_parameters.find("n");
                if (itN != elem.named_parameters.end()) n = itN->second;
                double rs = modelParam(mod, "rs", 0.0);
                auto itRs = elem.named_parameters.find("rs");
                if (itRs != elem.named_parameters.end()) rs = itRs->second;
                double bv = modelParam(mod, "bv", 0.0);

                size_t anode = a;
                if (rs > 0.0) {
                    // Explicit series resistor (stable) instead of folded Thevenin Rs.
                    size_t mid = nextIndex++;
                    std::string midName = elem.name + "#rs";
                    out.nodeMap[midName] = mid;
                    auto rser = std::make_shared<Resistor>(elem.name + "_rs", rs);
                    rser->setNodes(a, mid);
                    out.devices.push_back(rser);
                    anode = mid;
                }
                auto dev = std::make_shared<Diode>(elem.name, is, n);
                if (bv > 0) dev->setBreakdownVoltage(bv);
                dev->setNodes(anode, b);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::BJT: {
                if (!needNodes(3)) return out;
                size_t c = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t b = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t e = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                const SpiceModel* mod = lookupModel(elem.model_name);
                BJTType bt = BJTType::NPN;
                if (mod && mod->type.find("pnp") != std::string::npos) bt = BJTType::PNP;
                if (!elem.model_name.empty()) {
                    std::string m = elem.model_name;
                    std::transform(m.begin(), m.end(), m.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (m.find("pnp") != std::string::npos) bt = BJTType::PNP;
                }
                double is = modelParam(mod, "is", 1e-16);
                double bf = modelParam(mod, "bf", 100.0);
                auto dev = std::make_shared<BJT>(elem.name, bt, is, bf);
                if (mod) {
                    if (mod->params.count("br")) dev->setBR(mod->params.at("br"));
                    if (mod->params.count("vaf")) dev->setVAF(mod->params.at("vaf"));
                    if (mod->params.count("var")) dev->setVAR(mod->params.at("var"));
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
                const SpiceModel* mod = lookupModel(elem.model_name);
                MOSFETType mt = MOSFETType::NMOS;
                if (mod && (mod->type.find("p") == 0 || mod->type.find("pmos") != std::string::npos)) {
                    mt = MOSFETType::PMOS;
                }
                if (!elem.model_name.empty()) {
                    std::string m = elem.model_name;
                    std::transform(m.begin(), m.end(), m.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (m.find("p") == 0 || m.find("pmos") != std::string::npos) mt = MOSFETType::PMOS;
                }
                auto dev = std::make_shared<MOSFET>(elem.name, mt);
                auto named = [&](const char* k, double fb) {
                    auto it = elem.named_parameters.find(k);
                    return it != elem.named_parameters.end() ? it->second : fb;
                };
                double w = named("w", paramOr(elem, 0, 10e-6));
                double l = named("l", paramOr(elem, 1, 1e-6));
                if (elem.named_parameters.count("w")) w = elem.named_parameters.at("w");
                if (elem.named_parameters.count("l")) l = elem.named_parameters.at("l");
                dev->setWidth(w);
                dev->setLength(l);
                MOSFETModel model;
                double defVt = (mt == MOSFETType::NMOS) ? 0.7 : -0.7;
                // Model card first, instance KEY=val overrides.
                model.vt0_ = modelParam(mod, "vto", defVt);
                model.lambda_ = modelParam(mod, "lambda", 0.05);
                model.gamma_ = modelParam(mod, "gamma", 0.4);
                model.phi_ = modelParam(mod, "phi", 0.6);
                model.kp_ = modelParam(mod, "kp", 2e-5);
                if (elem.named_parameters.count("vto")) model.vt0_ = elem.named_parameters.at("vto");
                if (elem.named_parameters.count("lambda")) model.lambda_ = elem.named_parameters.at("lambda");
                if (elem.named_parameters.count("gamma")) model.gamma_ = elem.named_parameters.at("gamma");
                if (elem.named_parameters.count("phi")) model.phi_ = elem.named_parameters.at("phi");
                if (elem.named_parameters.count("kp")) model.kp_ = elem.named_parameters.at("kp");
                dev->setModel(model);
                dev->setTerminals({d, g, s, b});
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Instance:
            case NetlistElementType::Subckt:
                // X/subckt definitions are flattened by NetlistParser::expandedElements()
                // before buildCircuitFromNetlist(); leftover defs here are skipped.
                break;
            case NetlistElementType::VCVS: {
                if (!needNodes(4)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t ncp = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                size_t ncn = resolveNode(elem.nodes[3].net, out.nodeMap, nextIndex);
                double gain = paramOr(elem, 0, 1.0);
                auto it = elem.named_parameters.find("gain");
                if (it != elem.named_parameters.end()) gain = it->second;
                auto dev = std::make_shared<VCVS>(elem.name, gain);
                dev->setNodes(np, nn, ncp, ncn);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::VCCS: {
                if (!needNodes(4)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t ncp = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                size_t ncn = resolveNode(elem.nodes[3].net, out.nodeMap, nextIndex);
                double gm = paramOr(elem, 0, 1.0);
                auto it = elem.named_parameters.find("gm");
                if (it != elem.named_parameters.end()) gm = it->second;
                auto dev = std::make_shared<VCCS>(elem.name, gm);
                dev->setNodes(np, nn, ncp, ncn);
                out.devices.push_back(dev);
                break;
            }
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

std::map<std::string, double> parseNodeVoltagesFromControls(
    const std::vector<NetlistControl>& controls,
    const char* kind
) {
    std::map<std::string, double> out;
    std::string want = kind;
    for (char& c : want) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& d : controls) {
        if (d.kind != want) continue;
        // Tokens like: V(mid)=2.5  mid=1  V(n1)=0
        for (const auto& tok : d.tokens) {
            auto eq = tok.find('=');
            if (eq == std::string::npos) continue;
            std::string left = tok.substr(0, eq);
            std::string right = tok.substr(eq + 1);
            double v = 0.0;
            if (!NetlistParser::parseValue(right, v)) continue;
            // Strip V(…) wrapper
            if (left.size() >= 4 && (left[0] == 'V' || left[0] == 'v') && left[1] == '(' &&
                left.back() == ')') {
                left = left.substr(2, left.size() - 3);
            }
            out[left] = v;
        }
        // Also support space-separated: .ic V(mid) 2.5
        for (size_t i = 0; i + 1 < d.tokens.size(); ++i) {
            std::string left = d.tokens[i];
            if (left.find('=') != std::string::npos) continue;
            double v = 0.0;
            if (!NetlistParser::parseValue(d.tokens[i + 1], v)) continue;
            if (left.size() >= 4 && (left[0] == 'V' || left[0] == 'v') && left[1] == '(' &&
                left.back() == ')') {
                left = left.substr(2, left.size() - 3);
            }
            out[left] = v;
            ++i;
        }
    }
    return out;
}

std::vector<double> initialConditionVector(
    const BuiltCircuit& circuit,
    const std::map<std::string, double>& named
) {
    std::vector<double> v(circuit.numNodes, 0.0);
    for (const auto& kv : named) {
        auto it = circuit.nodeMap.find(kv.first);
        if (it == circuit.nodeMap.end() || it->second == 0) continue;
        size_t idx = it->second - 1;
        if (idx < v.size()) v[idx] = kv.second;
    }
    return v;
}

std::shared_ptr<Device> findSourceByName(
    const BuiltCircuit& circuit,
    const std::string& name
) {
    std::string want = name;
    std::transform(want.begin(), want.end(), want.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& d : circuit.devices) {
        std::string n = d->name();
        std::transform(n.begin(), n.end(), n.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (n == want) return d;
    }
    return nullptr;
}

}
