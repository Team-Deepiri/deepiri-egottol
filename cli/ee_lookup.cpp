#include "ee_lookup.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace deepiri {

namespace {

struct Entry {
    const char* id;
    const char* symptoms;  // space-separated keywords
    const char* combination;
    const char* behavior;
    const char* use;
};

const Entry kEntries[] = {
    {"flyback", "relay solenoid motor spike inductive kick flyback diode kills transistor mosfet",
     "Diode || coil (reversed)", "Recirculates inductive current when switch opens",
     "Mandatory on every relay/solenoid/motor driver"},
    {"led-burn", "led burned current bright resistor series",
     "Resistor + LED (series)", "Limits LED current", "Every indicator LED"},
    {"decouple", "noise rail ringing brownout decoupling capacitor",
     "0.1uF || IC pins + bulk nearby", "Shunts HF; bulk holds di/dt", "Every digital/analog IC"},
    {"ac-couple", "dc offset audio mic block dc capacitor series",
     "Capacitor in series with signal", "High-pass; blocks DC", "Audio / BJT base coupling"},
    {"rc-lpf", "low pass anti alias high frequency noise filter",
     "R series + C to GND", "Passes DC/low; attenuates HF", "ADC anti-alias, sensors"},
    {"rc-hpf", "high pass tweeter bass crossover",
     "C series + R to GND", "Blocks DC/low; passes HF", "Audio crossovers"},
    {"lc-tank", "radio tuner resonance tank frequency",
     "L || C parallel tank", "Peak Z at f0", "Tuners / band-stop"},
    {"boost", "step up boost converter flash hv 5v to 12v",
     "L + switch + diode + Cout", "Flyback energy raises Vout", "Boost converters"},
    {"buck", "step down buck efficient 12v to 5v vrm",
     "MOSFET + L + C || load + freewheel", "PWM averages voltage; L smooths I", "CPU/USB regulators"},
    {"crystal", "crystal clock oscillate mux two crystals",
     "Load caps + bias R; MUX for two clocks", "Never parallel crystals", "MCU clocks"},
    {"hbridge", "motor reverse bidirectional h-bridge",
     "Four transistors + motor", "Reverses polarity", "Robot drives"},
    {"floorplan", "pcb place floorplan dirty clean zone",
     "Dirty / Digital / Clean zones", "Partition by noise before routing", "All boards"},
    {"darlington", "high beta tiny current darlington",
     "BJT Darlington pair", "β multiplies", "High-gain switches"},
    {"gate-protect", "mosfet gate overvoltage floating zener",
     "Pull-down R + Zener || G-S", "Hold off + clamp Vgs", "Logic / industrial FETs"},
    {"pdn", "voltage droop power integrity pdn",
     "Bulk + mid + HF capacitor hierarchy", "Keep Z < ΔV/ΔI", "SoC boards"},
};

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

}  // namespace

std::vector<EeHit> lookupEeDesign(const std::string& query, int limit) {
    std::string q = lower(query);
    struct Scored { int score; EeHit hit; };
    std::vector<Scored> scored;
    for (const auto& e : kEntries) {
        std::string bag = lower(std::string(e.symptoms) + " " + e.combination + " " + e.use);
        int score = 0;
        std::istringstream iss(q);
        std::string tok;
        while (iss >> tok) {
            if (tok.size() < 2) continue;
            if (bag.find(tok) != std::string::npos) score += (tok.size() > 3 ? 3 : 2);
        }
        if (score > 0) {
            scored.push_back({score, EeHit{e.id, e.combination, e.behavior, e.use}});
        }
    }
    std::sort(scored.begin(), scored.end(),
              [](const Scored& a, const Scored& b) { return a.score > b.score; });
    std::vector<EeHit> out;
    for (size_t i = 0; i < scored.size() && static_cast<int>(i) < limit; ++i) {
        out.push_back(scored[i].hit);
    }
    return out;
}

std::string formatEeLookup(const std::string& query, int limit) {
    auto hits = lookupEeDesign(query, limit);
    std::ostringstream oss;
    if (hits.empty()) {
        oss << "No EE hits for '" << query << "'. Try: flyback, LED, buck, crystal, floorplan\n"
            << "Full docs: docs/ee/\n";
        return oss.str();
    }
    oss << "EE design matches for '" << query << "':\n";
    for (const auto& h : hits) {
        oss << "  [" << h.id << "] " << h.combination << "\n"
            << "      " << h.behavior << "\n"
            << "      Use: " << h.use << "\n";
    }
    oss << "Docs: docs/ee/  |  Python: egottol.knowledge.lookup_ee_design\n";
    return oss.str();
}

}
