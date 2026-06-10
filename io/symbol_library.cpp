#include "symbol_library.h"
#include <fstream>
#include <sstream>

namespace deepiri {

class SymbolLibrary::Impl {
public:
    std::map<std::string, SymbolDefinition> symbols_;
    std::string library_path_;

    void registerStandardSymbols() {
        SymbolDefinition r;
        r.name = "R";
        r.library = "Device";
        r.type = SymbolType::Component;
        r.footprint = "Resistor_SMD";
        r.description = "Resistor";
        r.pins = {};
        symbols_["R"] = r;

        SymbolDefinition c;
        c.name = "C";
        c.library = "Device";
        c.type = SymbolType::Component;
        c.footprint = "Capacitor_SMD";
        c.description = "Capacitor";
        c.pins = {};
        symbols_["C"] = c;

        SymbolDefinition l;
        l.name = "L";
        l.library = "Device";
        l.type = SymbolType::Component;
        l.footprint = "Inductor_SMD";
        l.description = "Inductor";
        l.pins = {};
        symbols_["L"] = l;

        // --- Batch 1: Passive & Discrete Expansion ---
        SymbolDefinition varistor; varistor.name="VARISTOR"; varistor.library="Device"; varistor.type=SymbolType::Component; varistor.footprint="SMD_1206"; varistor.description="Varistor"; symbols_["VARISTOR"]=varistor;
        SymbolDefinition therm_ntc; therm_ntc.name="THERM_NTC"; therm_ntc.library="Device"; therm_ntc.type=SymbolType::Component; therm_ntc.footprint="SMD_0805"; therm_ntc.description="NTC Thermistor"; symbols_["THERM_NTC"]=therm_ntc;
        SymbolDefinition therm_ptc; therm_ptc.name="THERM_PTC"; therm_ptc.library="Device"; therm_ptc.type=SymbolType::Component; therm_ptc.footprint="SMD_0805"; therm_ptc.description="PTC Thermistor"; symbols_["THERM_PTC"]=therm_ptc;
        SymbolDefinition trimmer; trimmer.name="TRIMMER"; trimmer.library="Device"; trimmer.type=SymbolType::Component; trimmer.footprint="Potentiometer_THT"; trimmer.description="Trimmer Potentiometer"; symbols_["TRIMMER"]=trimmer;
        SymbolDefinition cap_elec; cap_elec.name="CAP_ELEC"; cap_elec.library="Device"; cap_elec.type=SymbolType::Component; cap_elec.footprint="Capacitor_THT_Radial"; cap_elec.description="Electrolytic Capacitor"; symbols_["CAP_ELEC"]=cap_elec;
        SymbolDefinition cap_cer; cap_cer.name="CAP_CER"; cap_cer.library="Device"; cap_cer.type=SymbolType::Component; cap_cer.footprint="Capacitor_SMD_0603"; cap_cer.description="Ceramic Capacitor"; symbols_["CAP_CER"]=cap_cer;
        SymbolDefinition cap_film; cap_film.name="CAP_FILM"; cap_film.library="Device"; cap_film.type=SymbolType::Component; cap_film.footprint="Capacitor_THT_Film"; cap_film.description="Film Capacitor"; symbols_["CAP_FILM"]=cap_film;
        SymbolDefinition cap_tant; cap_tant.name="CAP_TANT"; cap_tant.library="Device"; cap_tant.type=SymbolType::Component; cap_tant.footprint="Capacitor_SMD_Tantalum"; cap_tant.description="Tantalum Capacitor"; symbols_["CAP_TANT"]=cap_tant;
        SymbolDefinition cap_trim; cap_trim.name="CAP_TRIM"; cap_trim.library="Device"; cap_trim.type=SymbolType::Component; cap_trim.footprint="Capacitor_THT_Trimmer"; cap_trim.description="Trimmer Capacitor"; symbols_["CAP_TRIM"]=cap_trim;
        SymbolDefinition ind_ferrite; ind_ferrite.name="IND_FERRITE"; ind_ferrite.library="Device"; ind_ferrite.type=SymbolType::Component; ind_ferrite.footprint="SMD_0805"; ind_ferrite.description="Ferrite Bead"; symbols_["IND_FERRITE"]=ind_ferrite;
        SymbolDefinition ind_var; ind_var.name="IND_VAR"; ind_var.library="Device"; ind_var.type=SymbolType::Component; ind_var.footprint="Inductor_THT_Variable"; ind_var.description="Variable Inductor"; symbols_["IND_VAR"]=ind_var;
        SymbolDefinition fuse; fuse.name="FUSE"; fuse.library="Device"; fuse.type=SymbolType::Component; fuse.footprint="Fuse_THT_5x20mm"; fuse.description="Fuse"; symbols_["FUSE"]=fuse;
        SymbolDefinition fuse_ptc; fuse_ptc.name="FUSE_PTC"; fuse_ptc.library="Device"; fuse_ptc.type=SymbolType::Component; fuse_ptc.footprint="Fuse_SMD_1206"; fuse_ptc.description="PTC Resettable Fuse"; symbols_["FUSE_PTC"]=fuse_ptc;
        SymbolDefinition crystal; crystal.name="CRYSTAL"; crystal.library="Device"; crystal.type=SymbolType::Component; crystal.footprint="Crystal_HC49"; crystal.description="Crystal Oscillator"; symbols_["CRYSTAL"]=crystal;
        SymbolDefinition resonator; resonator.name="RESONATOR"; resonator.library="Device"; resonator.type=SymbolType::Component; resonator.footprint="Resonator_THT"; resonator.description="Ceramic Resonator"; symbols_["RESONATOR"]=resonator;
        SymbolDefinition buzzer; buzzer.name="BUZZER"; buzzer.library="Device"; buzzer.type=SymbolType::Component; buzzer.footprint="Buzzer_THT"; buzzer.description="Piezo Buzzer"; symbols_["BUZZER"]=buzzer;
        SymbolDefinition relay_spst; relay_spst.name="RELAY_SPST"; relay_spst.library="Device"; relay_spst.type=SymbolType::Component; relay_spst.footprint="Relay_THT_SPST"; relay_spst.description="SPST Relay"; symbols_["RELAY_SPST"]=relay_spst;
        SymbolDefinition relay_dpdt; relay_dpdt.name="RELAY_DPDT"; relay_dpdt.library="Device"; relay_dpdt.type=SymbolType::Component; relay_dpdt.footprint="Relay_THT_DPDT"; relay_dpdt.description="DPDT Relay"; symbols_["RELAY_DPDT"]=relay_dpdt;
        SymbolDefinition photodiode; photodiode.name="PHOTODIODE"; photodiode.library="Device"; photodiode.type=SymbolType::Component; photodiode.footprint="SMD_1206"; photodiode.description="Photodiode"; symbols_["PHOTODIODE"]=photodiode;
        SymbolDefinition phototrans; phototrans.name="PHOTOTRANS"; phototrans.library="Device"; phototrans.type=SymbolType::Component; phototrans.footprint="SMD_1206"; phototrans.description="Phototransistor"; symbols_["PHOTOTRANS"]=phototrans;
        SymbolDefinition optocoupler; optocoupler.name="OPTOCOUPLER"; optocoupler.library="Device"; optocoupler.type=SymbolType::Component; optocoupler.footprint="DIP-4"; optocoupler.description="Optocoupler"; symbols_["OPTOCOUPLER"]=optocoupler;
        SymbolDefinition bridge_rect; bridge_rect.name="BRIDGE_RECT"; bridge_rect.library="Device"; bridge_rect.type=SymbolType::Component; bridge_rect.footprint="Bridge_THT"; bridge_rect.description="Bridge Rectifier"; symbols_["BRIDGE_RECT"]=bridge_rect;
        SymbolDefinition tvs; tvs.name="TVS"; tvs.library="Device"; tvs.type=SymbolType::Component; tvs.footprint="SMD_1206"; tvs.description="TVS Diode"; symbols_["TVS"]=tvs;
        SymbolDefinition schottky; schottky.name="SCHOTTKY"; schottky.library="Device"; schottky.type=SymbolType::Component; schottky.footprint="SMD_1206"; schottky.description="Schottky Diode"; symbols_["SCHOTTKY"]=schottky;
        SymbolDefinition diode_tunnel; diode_tunnel.name="DIODE_TUNNEL"; diode_tunnel.library="Device"; diode_tunnel.type=SymbolType::Component; diode_tunnel.footprint="SMD_1206"; diode_tunnel.description="Tunnel Diode"; symbols_["DIODE_TUNNEL"]=diode_tunnel;
    }
};

SymbolLibrary::SymbolLibrary() : pImpl(std::make_unique<Impl>()) {
    pImpl->registerStandardSymbols();
}

SymbolLibrary::SymbolLibrary(const std::string& library_path) : pImpl(std::make_unique<Impl>()) {
    pImpl->library_path_ = library_path;
    pImpl->registerStandardSymbols();
    load(library_path);
}

SymbolLibrary::~SymbolLibrary() = default;

bool SymbolLibrary::load(const std::string& library_path) {
    pImpl->library_path_ = library_path;
    return true;
}

bool SymbolLibrary::save(const std::string& library_path) {
    pImpl->library_path_ = library_path;
    return true;
}

void SymbolLibrary::addSymbol(const SymbolDefinition& symbol) {
    pImpl->symbols_[symbol.name] = symbol;
}

void SymbolLibrary::removeSymbol(const std::string& name) {
    pImpl->symbols_.erase(name);
}

std::optional<SymbolDefinition> SymbolLibrary::getSymbol(const std::string& name) const {
    auto it = pImpl->symbols_.find(name);
    if (it != pImpl->symbols_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> SymbolLibrary::listSymbols() const {
    std::vector<std::string> names;
    for (const auto& sym : pImpl->symbols_) {
        names.push_back(sym.first);
    }
    return names;
}

std::vector<std::string> SymbolLibrary::listLibraries() const {
    std::vector<std::string> libs;
    for (const auto& sym : pImpl->symbols_) {
        libs.push_back(sym.second.library);
    }
    return libs;
}

bool SymbolLibrary::hasSymbol(const std::string& name) const {
    return pImpl->symbols_.find(name) != pImpl->symbols_.end();
}

}