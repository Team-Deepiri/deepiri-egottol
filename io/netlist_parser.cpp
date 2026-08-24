#include "netlist_parser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace deepiri {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool isExactTypeKeyword(const std::string& t) {
    const std::string l = toLower(t);
    return l == "r" || l == "res" ||
           l == "c" || l == "cap" ||
           l == "l" || l == "ind" ||
           l == "v" || l == "vsrc" ||
           l == "i" || l == "isrc" ||
           l == "m" || l == "mos" ||
           l == "q" || l == "bjt" ||
           l == "d" || l == "diode" ||
           l == "x";
}

NetlistElementType typeFromKeyword(const std::string& t) {
    const std::string l = toLower(t);
    if (l == "r" || l == "res") return NetlistElementType::Resistor;
    if (l == "c" || l == "cap") return NetlistElementType::Capacitor;
    if (l == "l" || l == "ind") return NetlistElementType::Inductor;
    if (l == "v" || l == "vsrc") return NetlistElementType::VoltageSource;
    if (l == "i" || l == "isrc") return NetlistElementType::CurrentSource;
    if (l == "m" || l == "mos") return NetlistElementType::MOSFET;
    if (l == "q" || l == "bjt") return NetlistElementType::BJT;
    if (l == "d" || l == "diode") return NetlistElementType::Diode;
    if (l == "x") return NetlistElementType::Instance;
    return NetlistElementType::Instance;
}

NetlistElementType typeFromNamePrefix(const std::string& name) {
    if (name.empty()) return NetlistElementType::Instance;
    char c = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    switch (c) {
        case 'R': return NetlistElementType::Resistor;
        case 'C': return NetlistElementType::Capacitor;
        case 'L': return NetlistElementType::Inductor;
        case 'V': return NetlistElementType::VoltageSource;
        case 'I': return NetlistElementType::CurrentSource;
        case 'M': return NetlistElementType::MOSFET;
        case 'Q': return NetlistElementType::BJT;
        case 'D': return NetlistElementType::Diode;
        case 'X': return NetlistElementType::Instance;
        default:  return NetlistElementType::Instance;
    }
}

bool looksNumeric(const std::string& token) {
    if (token.empty()) return false;
    // KEY=value → treat as not a plain node
    if (token.find('=') != std::string::npos) return false;
    char c0 = token[0];
    if (c0 == '+' || c0 == '-' || c0 == '.') return token.size() > 1 && std::isdigit(static_cast<unsigned char>(token[1]));
    return std::isdigit(static_cast<unsigned char>(c0));
}

// Expand PULSE(a b c) / SIN(...) into separate tokens; split KEY=VAL.
std::vector<std::string> tokenizeSpice(const std::string& raw) {
    std::string s = raw;
    // Replace parentheses with spaces so PULSE(0 1 0) → PULSE 0 1 0
    for (char& c : s) {
        if (c == '(' || c == ')' || c == ',') c = ' ';
    }
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

bool parseKeyedValue(const std::string& token, std::string& key, double& value) {
    size_t eq = token.find('=');
    if (eq == std::string::npos || eq == 0) return false;
    key = toLower(token.substr(0, eq));
    return NetlistParser::parseValue(token.substr(eq + 1), value);
}

std::string controlKind(const std::string& line) {
    if (line.empty() || line[0] != '.') return "";
    size_t i = 1;
    while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
        ++i;
    }
    return toLower(line.substr(1, i - 1));
}

bool isKnownControl(const std::string& kind) {
    return kind == "end" || kind == "ends" || kind == "include" || kind == "lib" ||
           kind == "param" || kind == "option" || kind == "options" ||
           kind == "tran" || kind == "ac" || kind == "dc" || kind == "op" ||
           kind == "step" || kind == "model" || kind == "subckt" ||
           kind == "ic" || kind == "nodeset" || kind == "print" || kind == "probe" ||
           kind == "save" || kind == "plot" || kind == "four" || kind == "noise" ||
           kind == "tf" || kind == "sens" || kind == "measure" || kind == "meas" ||
           kind == "global" || kind == "func" || kind == "title";
}

}  // namespace

class NetlistParser::Impl {
public:
    std::vector<NetlistElement> elements_;
    std::map<std::string, std::vector<std::string>> nets_;
    std::vector<std::string> controls_;
    std::vector<NetlistControl> directives_;
    std::map<std::string, SpiceModel> models_;
};

bool NetlistParser::parseValue(const std::string& token, double& out) {
    if (token.empty()) return false;

    std::string s = token;
    // Strip surrounding quotes if present.
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        s = s.substr(1, s.size() - 2);
    }

    size_t idx = 0;
    try {
        double magnitude = std::stod(s, &idx);
        if (idx == 0) return false;

        std::string suffix = s.substr(idx);
        // Drop trailing unit letters that aren't scale factors (ohm, farad, henry, volt, amp, hz, s).
        // Scale factor is the leading engineering letter(s).
        suffix = toLower(suffix);
        double scale = 1.0;
        if (!suffix.empty()) {
            if (suffix.rfind("meg", 0) == 0) {
                scale = 1e6;
            } else {
                switch (suffix[0]) {
                    case 't': scale = 1e12; break;
                    case 'g': scale = 1e9; break;
                    case 'k': scale = 1e3; break;
                    case 'm': scale = 1e-3; break;
                    case 'u': scale = 1e-6; break;
                    case 'n': scale = 1e-9; break;
                    case 'p': scale = 1e-12; break;
                    case 'f': scale = 1e-15; break;
                    case 'a': // ampere — no scale
                    case 'v': // volt
                    case 'o': // ohm
                    case 'h': // henry / hz
                    case 's': // second
                    case 'c': // coulomb / celsius — ignore
                        scale = 1.0;
                        break;
                    default:
                        // Unknown trailing junk → still accept the magnitude.
                        scale = 1.0;
                        break;
                }
            }
        }
        out = magnitude * scale;
        return true;
    } catch (...) {
        return false;
    }
}

int NetlistParser::expectedNodeCount(NetlistElementType type) {
    switch (type) {
        case NetlistElementType::Resistor:
        case NetlistElementType::Capacitor:
        case NetlistElementType::Inductor:
        case NetlistElementType::VoltageSource:
        case NetlistElementType::CurrentSource:
        case NetlistElementType::Diode:
            return 2;
        case NetlistElementType::BJT:
            return 3;
        case NetlistElementType::MOSFET:
            return 4;
        case NetlistElementType::Subckt:
        case NetlistElementType::Instance:
        default:
            return -1;
    }
}

NetlistParser::NetlistParser() : pImpl(std::make_unique<Impl>()) {}
NetlistParser::~NetlistParser() = default;

bool NetlistParser::parse(const std::string& netlist_content) {
    pImpl->elements_.clear();
    pImpl->nets_.clear();
    pImpl->controls_.clear();
    pImpl->directives_.clear();
    pImpl->models_.clear();

    std::istringstream iss(netlist_content);
    std::string line;
    std::string continued;

    auto flushLine = [&](std::string raw) {
        raw = trim(raw);
        if (raw.empty() || raw[0] == '*' || raw[0] == ';') {
            return;
        }

        // Strip inline comments starting with ';' or unquoted '*'.
        size_t comment = raw.find(';');
        if (comment != std::string::npos) {
            raw = trim(raw.substr(0, comment));
        }
        if (raw.empty()) return;

        if (raw[0] == '.') {
            NetlistControl ctrl;
            ctrl.raw = raw;
            ctrl.kind = controlKind(raw);
            pImpl->controls_.push_back(raw);

            std::istringstream cs(raw);
            std::string first;
            cs >> first;  // ".tran" etc.
            std::string tok;
            while (cs >> tok) {
                ctrl.tokens.push_back(tok);
                double v = 0.0;
                if (parseValue(tok, v)) {
                    ctrl.numbers.push_back(v);
                }
            }
            if (isKnownControl(ctrl.kind) || !ctrl.kind.empty()) {
                pImpl->directives_.push_back(ctrl);
            }
            // `.model Name TYPE (IS=1e-14 N=1 VTO=0.7 …)`
            if (ctrl.kind == "model" && ctrl.tokens.size() >= 2) {
                SpiceModel model;
                model.name = toLower(ctrl.tokens[0]);
                model.type = toLower(ctrl.tokens[1]);
                for (size_t ti = 2; ti < ctrl.tokens.size(); ++ti) {
                    std::string key;
                    double val = 0.0;
                    // tokens may still be KEY=VAL or bare numbers after TYPE
                    size_t eq = ctrl.tokens[ti].find('=');
                    if (eq != std::string::npos && eq > 0) {
                        key = toLower(ctrl.tokens[ti].substr(0, eq));
                        if (parseValue(ctrl.tokens[ti].substr(eq + 1), val)) {
                            model.params[key] = val;
                        }
                    }
                }
                // Also accept parenthesized form already flattened by tokenizeSpice on element
                // lines; for control lines we still have raw. Re-tokenize raw for KEY=VAL.
                std::string rawFlat = ctrl.raw;
                for (char& c : rawFlat) {
                    if (c == '(' || c == ')' || c == ',') c = ' ';
                }
                std::istringstream rs(rawFlat);
                std::string rt;
                rs >> rt;  // .model
                if (rs >> rt) model.name = toLower(rt);
                if (rs >> rt) model.type = toLower(rt);
                while (rs >> rt) {
                    size_t eq = rt.find('=');
                    if (eq != std::string::npos && eq > 0) {
                        std::string key = toLower(rt.substr(0, eq));
                        double val = 0.0;
                        if (parseValue(rt.substr(eq + 1), val)) {
                            model.params[key] = val;
                        }
                    }
                }
                pImpl->models_[model.name] = model;
            }
            return;
        }

        std::vector<std::string> tokens = tokenizeSpice(raw);
        if (tokens.empty()) return;

        NetlistElement elem;
        size_t restStart = 0;

        // Named-type form: `R name n1 n2 1k` — first token is an exact type keyword.
        // Standard SPICE form: `R1 n1 n2 1k` — first token is the instance name.
        if (tokens.size() >= 2 && isExactTypeKeyword(tokens[0])) {
            elem.type = typeFromKeyword(tokens[0]);
            elem.name = tokens[1];
            restStart = 2;
            if (elem.type == NetlistElementType::Instance) {
                elem.subckt_name = tokens[0];
            }
        } else {
            elem.name = tokens[0];
            elem.type = typeFromNamePrefix(tokens[0]);
            restStart = 1;
        }

        std::vector<std::string> rest(tokens.begin() + static_cast<std::ptrdiff_t>(restStart),
                                      tokens.end());

        int nNodes = expectedNodeCount(elem.type);
        if (nNodes < 0) {
            // Instance / X: last non-numeric token is subcircuit/model name; rest are nodes.
            if (!rest.empty()) {
                // Find last token that does not parse as a value → subckt name.
                int subIdx = static_cast<int>(rest.size()) - 1;
                while (subIdx >= 0 && looksNumeric(rest[static_cast<size_t>(subIdx)])) {
                    --subIdx;
                }
                if (subIdx >= 0) {
                    elem.subckt_name = rest[static_cast<size_t>(subIdx)];
                    elem.model_name = elem.subckt_name;
                    for (int i = 0; i < subIdx; ++i) {
                        NetlistNode n;
                        n.name = rest[static_cast<size_t>(i)];
                        n.net = n.name;
                        elem.nodes.push_back(n);
                        pImpl->nets_[n.net].push_back(elem.name);
                    }
                    for (size_t i = static_cast<size_t>(subIdx) + 1; i < rest.size(); ++i) {
                        std::string key;
                        double v = 0.0;
                        if (parseKeyedValue(rest[i], key, v)) {
                            elem.named_parameters[key] = v;
                            elem.parameters.push_back(v);
                        } else if (parseValue(rest[i], v)) {
                            elem.parameters.push_back(v);
                        }
                    }
                } else {
                    for (const auto& nodeTok : rest) {
                        NetlistNode n;
                        n.name = nodeTok;
                        n.net = nodeTok;
                        elem.nodes.push_back(n);
                        pImpl->nets_[n.net].push_back(elem.name);
                    }
                }
            }
        } else {
            size_t i = 0;
            for (; i < rest.size() && static_cast<int>(elem.nodes.size()) < nNodes; ++i) {
                // Don't treat KEY=val as a node name.
                if (rest[i].find('=') != std::string::npos) break;
                NetlistNode n;
                n.name = rest[i];
                n.net = rest[i];
                elem.nodes.push_back(n);
                pImpl->nets_[n.net].push_back(elem.name);
            }
            // Remaining: PULSE/SIN keywords, model name, keyed params, values.
            for (; i < rest.size(); ++i) {
                std::string key;
                double v = 0.0;
                std::string low = toLower(rest[i]);
                if (low == "pulse" || low == "sin" || low == "exp" || low == "pwl" || low == "ac") {
                    // Collect following numeric args into parameters; mark source type via named flag.
                    elem.named_parameters[low] = 1.0;
                    continue;
                }
                if (parseKeyedValue(rest[i], key, v)) {
                    elem.named_parameters[key] = v;
                    elem.parameters.push_back(v);
                } else if (parseValue(rest[i], v)) {
                    elem.parameters.push_back(v);
                } else if (elem.model_name.empty()) {
                    elem.model_name = rest[i];
                }
            }
        }

        pImpl->elements_.push_back(std::move(elem));
    };

    while (std::getline(iss, line)) {
        // SPICE line continuation: lines starting with '+' append to previous.
        std::string t = trim(line);
        if (!t.empty() && t[0] == '+') {
            continued += " " + t.substr(1);
            continue;
        }
        if (!continued.empty()) {
            flushLine(continued);
            continued.clear();
        }
        continued = line;
    }
    if (!continued.empty()) {
        flushLine(continued);
    }

    return true;
}

bool NetlistParser::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

std::vector<NetlistElement> NetlistParser::getElements() const {
    return pImpl->elements_;
}

std::map<std::string, std::vector<std::string>> NetlistParser::getNets() const {
    return pImpl->nets_;
}

std::vector<std::string> NetlistParser::getControls() const {
    return pImpl->controls_;
}

std::vector<NetlistControl> NetlistParser::getControlDirectives() const {
    return pImpl->directives_;
}

std::map<std::string, SpiceModel> NetlistParser::getModels() const {
    return pImpl->models_;
}

std::string NetlistParser::toNetlist() const {
    std::ostringstream oss;

    for (const auto& elem : pImpl->elements_) {
        // Emit standard SPICE form: name embeds the type prefix.
        oss << elem.name;
        for (const auto& node : elem.nodes) {
            oss << " " << node.net;
        }
        if (!elem.model_name.empty()) {
            oss << " " << elem.model_name;
        }
        for (double p : elem.parameters) {
            oss << " " << p;
        }
        if (!elem.subckt_name.empty() && elem.type == NetlistElementType::Instance &&
            elem.model_name.empty()) {
            oss << " " << elem.subckt_name;
        }
        oss << "\n";
    }

    for (const auto& ctrl : pImpl->controls_) {
        oss << ctrl << "\n";
    }

    return oss.str();
}

}
