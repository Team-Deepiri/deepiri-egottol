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
#include "../models/cccs.h"
#include "../models/ccvs.h"
#include "../models/vswitch.h"
#include "../models/iswitch.h"
#include "../models/coupled_inductor.h"
#include "../models/source_signal.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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
    return buildCircuitFromElements(
        parser.expandedElements(), parser.getModels(), parser.getControlDirectives());
}

BuiltCircuit buildCircuitFromElements(
    const std::vector<NetlistElement>& elements,
    const std::map<std::string, SpiceModel>& models,
    const std::vector<NetlistControl>& /*controls*/
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
                if (elem.named_parameters.count("kf")) dev->setKF(elem.named_parameters.at("kf"));
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
                model.level_ = static_cast<int>(modelParam(mod, "level", 1));
                model.u0_ = modelParam(mod, "u0", 0.0);
                model.tox_ = modelParam(mod, "tox", 0.0);
                model.ucrit_ = modelParam(mod, "ucrit", modelParam(mod, "vmax", 0.0));
                if (model.u0_ > 0.0 && model.tox_ > 0.0) {
                    // KP = μ0·Cox, Cox=εox/tox; μ0 often in cm²/Vs → convert to m²/Vs
                    constexpr double epsOx = 3.45e-11;
                    double u0_m = model.u0_ * 1e-4;  // cm² → m²
                    model.kp_ = u0_m * epsOx / model.tox_;
                }
                if (elem.named_parameters.count("vto")) model.vt0_ = elem.named_parameters.at("vto");
                if (elem.named_parameters.count("lambda")) model.lambda_ = elem.named_parameters.at("lambda");
                if (elem.named_parameters.count("gamma")) model.gamma_ = elem.named_parameters.at("gamma");
                if (elem.named_parameters.count("phi")) model.phi_ = elem.named_parameters.at("phi");
                if (elem.named_parameters.count("kp")) model.kp_ = elem.named_parameters.at("kp");
                if (elem.named_parameters.count("level"))
                    model.level_ = static_cast<int>(elem.named_parameters.at("level"));
                if (elem.named_parameters.count("ucrit"))
                    model.ucrit_ = elem.named_parameters.at("ucrit");
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
            case NetlistElementType::CCCS: {
                if (!needNodes(2)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double gain = paramOr(elem, 0, 1.0);
                auto dev = std::make_shared<CCCS>(elem.name, gain);
                if (!elem.model_name.empty()) dev->setSenseName(elem.model_name);
                dev->setNodes(np, nn);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::CCVS: {
                if (!needNodes(2)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double gain = paramOr(elem, 0, 1.0);
                auto dev = std::make_shared<CCVS>(elem.name, gain);
                if (!elem.model_name.empty()) dev->setSenseName(elem.model_name);
                dev->setNodes(np, nn);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::VSwitch: {
                if (!needNodes(4)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                size_t ncp = resolveNode(elem.nodes[2].net, out.nodeMap, nextIndex);
                size_t ncn = resolveNode(elem.nodes[3].net, out.nodeMap, nextIndex);
                double vt = 0.0, ron = 1.0, roff = 1e12;
                const SpiceModel* mod = lookupModel(elem.model_name);
                if (mod) {
                    vt = modelParam(mod, "vt", vt);
                    ron = modelParam(mod, "ron", ron);
                    roff = modelParam(mod, "roff", roff);
                }
                if (elem.named_parameters.count("vt")) vt = elem.named_parameters.at("vt");
                if (elem.named_parameters.count("ron")) ron = elem.named_parameters.at("ron");
                if (elem.named_parameters.count("roff")) roff = elem.named_parameters.at("roff");
                auto dev = std::make_shared<VSwitch>(elem.name, vt, ron, roff);
                dev->setNodes(np, nn, ncp, ncn);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::ISwitch: {
                if (!needNodes(2)) return out;
                size_t np = resolveNode(elem.nodes[0].net, out.nodeMap, nextIndex);
                size_t nn = resolveNode(elem.nodes[1].net, out.nodeMap, nextIndex);
                double it = 0.0, ron = 1.0, roff = 1e12;
                const SpiceModel* mod = lookupModel(elem.subckt_name);
                if (mod) {
                    it = modelParam(mod, "it", it);
                    ron = modelParam(mod, "ron", ron);
                    roff = modelParam(mod, "roff", roff);
                }
                if (elem.named_parameters.count("it")) it = elem.named_parameters.at("it");
                if (elem.named_parameters.count("ron")) ron = elem.named_parameters.at("ron");
                if (elem.named_parameters.count("roff")) roff = elem.named_parameters.at("roff");
                auto dev = std::make_shared<ISwitch>(elem.name, it, ron, roff);
                if (!elem.model_name.empty()) dev->setSenseName(elem.model_name);
                dev->setNodes(np, nn);
                out.devices.push_back(dev);
                break;
            }
            case NetlistElementType::Mutual:
                // Applied in a second pass after all inductors exist.
                break;
        }
    }

    // Second pass: Kxxx L1 L2 k → CoupledInductor replacing L1/L2.
    for (const auto& elem : elements) {
        if (elem.type != NetlistElementType::Mutual) continue;
        if (elem.subckt_name.empty() || elem.model_name.empty() || elem.parameters.empty()) {
            out.error = "Mutual " + elem.name + " needs L1 L2 k";
            out.ok = false;
            return out;
        }
        std::string l1n = elem.subckt_name;
        std::string l2n = elem.model_name;
        double k = elem.parameters[0];
        auto lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        };
        std::shared_ptr<Inductor> a, b;
        size_t ia = out.devices.size(), ib = out.devices.size();
        for (size_t i = 0; i < out.devices.size(); ++i) {
            if (out.devices[i]->type() != "Inductor") continue;
            if (lower(out.devices[i]->name()) == lower(l1n)) {
                a = std::dynamic_pointer_cast<Inductor>(out.devices[i]);
                ia = i;
            }
            if (lower(out.devices[i]->name()) == lower(l2n)) {
                b = std::dynamic_pointer_cast<Inductor>(out.devices[i]);
                ib = i;
            }
        }
        if (!a || !b) {
            out.error = "Mutual " + elem.name + ": inductors not found";
            out.ok = false;
            return out;
        }
        auto coupled = std::make_shared<CoupledInductor>(
            elem.name, a->inductance(), b->inductance(), k,
            a->nodeP(), a->nodeN(), b->nodeP(), b->nodeN());
        // Remove higher index first
        if (ia > ib) std::swap(ia, ib);
        out.devices.erase(out.devices.begin() + static_cast<std::ptrdiff_t>(ib));
        out.devices.erase(out.devices.begin() + static_cast<std::ptrdiff_t>(ia));
        out.devices.push_back(coupled);
    }

    out.numNodes = nextIndex > 0 ? nextIndex - 1 : 0;
    out.ok = out.error.empty();
    if (out.ok && out.devices.empty()) {
        out.error = "No supported devices in netlist";
        out.ok = false;
    }
    return out;
}

std::vector<MeasureResult> evaluateMeasures(
    const std::vector<NetlistControl>& controls,
    const BuiltCircuit& circuit,
    const std::vector<double>& timePoints,
    const std::vector<std::vector<double>>& nodeVoltages
) {
    std::vector<MeasureResult> results;
    auto nodeIdx = [&](const std::string& net) -> int {
        auto it = circuit.nodeMap.find(net);
        if (it == circuit.nodeMap.end() || it->second == 0) return -1;
        return static_cast<int>(it->second - 1);
    };
    auto parseProbe = [&](const std::string& tok) -> int {
        // V(out) or out
        if (tok.size() >= 4 && (tok[0] == 'V' || tok[0] == 'v') && tok[1] == '(' && tok.back() == ')') {
            return nodeIdx(tok.substr(2, tok.size() - 3));
        }
        return nodeIdx(tok);
    };

    auto parseAtTime = [&](const std::string& tok, double& atOut) -> bool {
        std::string low = tok;
        std::transform(low.begin(), low.end(), low.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (low.rfind("at=", 0) == 0) {
            return NetlistParser::parseValue(tok.substr(3), atOut);
        }
        return false;
    };

    auto parseWhen = [&](const std::string& tok, int& whenProbe, double& whenVal) -> bool {
        std::string low = tok;
        std::transform(low.begin(), low.end(), low.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        size_t wpos = low.find("when");
        if (wpos == std::string::npos) return false;
        std::string rest = tok.substr(wpos + 4);
        size_t eq = rest.find('=');
        if (eq == std::string::npos) return false;
        auto trimTok = [](std::string s) {
            size_t a = s.find_first_not_of(" \t");
            if (a == std::string::npos) return std::string();
            size_t b = s.find_last_not_of(" \t");
            return s.substr(a, b - a + 1);
        };
        whenProbe = parseProbe(trimTok(rest.substr(0, eq)));
        return whenProbe >= 0 && NetlistParser::parseValue(trimTok(rest.substr(eq + 1)), whenVal);
    };

    for (const auto& d : controls) {
        if (d.kind != "measure" && d.kind != "meas") continue;
        MeasureResult mr;
        // .measure [tran|dc|ac] name OP V(node)
        size_t ti = 0;
        if (!d.tokens.empty()) {
            std::string t0 = d.tokens[0];
            std::transform(t0.begin(), t0.end(), t0.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (t0 == "tran" || t0 == "dc" || t0 == "ac" || t0 == "op") ti = 1;
        }
        mr.name = (ti < d.tokens.size()) ? d.tokens[ti] : "meas";
        std::string op = "max";
        int probe = -1;
        double atTime = -1.0;
        int whenProbe = -1;
        double whenVal = 0.0;
        bool hasWhen = false;
        for (size_t i = 0; i < d.tokens.size(); ++i) {
            std::string t = d.tokens[i];
            std::string low = t;
            std::transform(low.begin(), low.end(), low.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (low == "max" || low == "min" || low == "avg" || low == "mean" ||
                low == "find" || low == "pp" || low == "rms") {
                op = low;
            }
            double atParsed = 0.0;
            if (parseAtTime(t, atParsed)) atTime = atParsed;
            int wp = -1;
            double wv = 0.0;
            if (parseWhen(t, wp, wv)) {
                whenProbe = wp;
                whenVal = wv;
                hasWhen = true;
            }
            int p = parseProbe(t);
            if (p >= 0) probe = p;
        }
        if (probe < 0 || timePoints.empty() || nodeVoltages.empty()) {
            mr.message = "measure probe missing";
            results.push_back(mr);
            continue;
        }
        double acc = 0.0;
        double mn = 1e300, mx = -1e300;
        size_t n = 0;
        for (size_t i = 0; i < nodeVoltages.size(); ++i) {
            if (static_cast<size_t>(probe) >= nodeVoltages[i].size()) continue;
            double v = nodeVoltages[i][static_cast<size_t>(probe)];
            mn = std::min(mn, v);
            mx = std::max(mx, v);
            acc += v;
            ++n;
        }
        if (n == 0) {
            mr.message = "no samples";
            results.push_back(mr);
            continue;
        }
        if (op == "max") mr.value = mx;
        else if (op == "min") mr.value = mn;
        else if (op == "avg" || op == "mean") mr.value = acc / static_cast<double>(n);
        else if (op == "pp") mr.value = mx - mn;
        else if (op == "rms") {
            double s = 0.0;
            for (size_t i = 0; i < nodeVoltages.size(); ++i) {
                if (static_cast<size_t>(probe) >= nodeVoltages[i].size()) continue;
                double v = nodeVoltages[i][static_cast<size_t>(probe)];
                s += v * v;
            }
            mr.value = std::sqrt(s / static_cast<double>(n));
        }         else if (op == "find") {
            if (hasWhen && whenProbe >= 0) {
                bool found = false;
                for (size_t i = 1; i < timePoints.size(); ++i) {
                    if (static_cast<size_t>(whenProbe) >= nodeVoltages[i - 1].size() ||
                        static_cast<size_t>(whenProbe) >= nodeVoltages[i].size() ||
                        static_cast<size_t>(probe) >= nodeVoltages[i].size()) {
                        continue;
                    }
                    double y0 = nodeVoltages[i - 1][static_cast<size_t>(whenProbe)];
                    double y1 = nodeVoltages[i][static_cast<size_t>(whenProbe)];
                    if ((y0 - whenVal) * (y1 - whenVal) <= 0.0) {
                        double t0 = timePoints[i - 1];
                        double t1 = timePoints[i];
                        double frac = (std::abs(y1 - y0) > 1e-18)
                                          ? ((whenVal - y0) / (y1 - y0))
                                          : 0.0;
                        double tcross = t0 + frac * (t1 - t0);
                        // Interpolate probe at crossing time.
                        double v0 = nodeVoltages[i - 1][static_cast<size_t>(probe)];
                        double v1 = nodeVoltages[i][static_cast<size_t>(probe)];
                        mr.value = v0 + frac * (v1 - v0);
                        (void)tcross;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    mr.message = "WHEN crossing not found";
                    results.push_back(mr);
                    continue;
                }
            } else if (atTime >= 0.0) {
                size_t best = 0;
                double bestDt = 1e300;
                for (size_t i = 0; i < timePoints.size(); ++i) {
                    double dt = std::abs(timePoints[i] - atTime);
                    if (dt < bestDt) {
                        bestDt = dt;
                        best = i;
                    }
                }
                if (static_cast<size_t>(probe) < nodeVoltages[best].size()) {
                    mr.value = nodeVoltages[best][static_cast<size_t>(probe)];
                } else {
                    mr.message = "AT sample missing";
                    results.push_back(mr);
                    continue;
                }
            } else {
                mr.value = nodeVoltages.back()[static_cast<size_t>(probe)];
            }
        } else {
            mr.value = mx;
        }
        mr.ok = true;
        mr.message = "ok";
        results.push_back(mr);
    }
    return results;
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
