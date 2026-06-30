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
    SymbolDefinition varistor;
    varistor.name = "VARISTOR";
    varistor.library = "Device";
    varistor.type = SymbolType::Component;
    varistor.footprint = "SMD_1206";
    varistor.description = "Varistor";
    symbols_["VARISTOR"] = varistor;
    SymbolDefinition therm_ntc;
    therm_ntc.name = "THERM_NTC";
    therm_ntc.library = "Device";
    therm_ntc.type = SymbolType::Component;
    therm_ntc.footprint = "SMD_0805";
    therm_ntc.description = "NTC Thermistor";
    symbols_["THERM_NTC"] = therm_ntc;
    SymbolDefinition therm_ptc;
    therm_ptc.name = "THERM_PTC";
    therm_ptc.library = "Device";
    therm_ptc.type = SymbolType::Component;
    therm_ptc.footprint = "SMD_0805";
    therm_ptc.description = "PTC Thermistor";
    symbols_["THERM_PTC"] = therm_ptc;
    SymbolDefinition trimmer;
    trimmer.name = "TRIMMER";
    trimmer.library = "Device";
    trimmer.type = SymbolType::Component;
    trimmer.footprint = "Potentiometer_THT";
    trimmer.description = "Trimmer Potentiometer";
    symbols_["TRIMMER"] = trimmer;
    SymbolDefinition cap_elec;
    cap_elec.name = "CAP_ELEC";
    cap_elec.library = "Device";
    cap_elec.type = SymbolType::Component;
    cap_elec.footprint = "Capacitor_THT_Radial";
    cap_elec.description = "Electrolytic Capacitor";
    symbols_["CAP_ELEC"] = cap_elec;
    SymbolDefinition cap_cer;
    cap_cer.name = "CAP_CER";
    cap_cer.library = "Device";
    cap_cer.type = SymbolType::Component;
    cap_cer.footprint = "Capacitor_SMD_0603";
    cap_cer.description = "Ceramic Capacitor";
    symbols_["CAP_CER"] = cap_cer;
    SymbolDefinition cap_film;
    cap_film.name = "CAP_FILM";
    cap_film.library = "Device";
    cap_film.type = SymbolType::Component;
    cap_film.footprint = "Capacitor_THT_Film";
    cap_film.description = "Film Capacitor";
    symbols_["CAP_FILM"] = cap_film;
    SymbolDefinition cap_tant;
    cap_tant.name = "CAP_TANT";
    cap_tant.library = "Device";
    cap_tant.type = SymbolType::Component;
    cap_tant.footprint = "Capacitor_SMD_Tantalum";
    cap_tant.description = "Tantalum Capacitor";
    symbols_["CAP_TANT"] = cap_tant;
    SymbolDefinition cap_trim;
    cap_trim.name = "CAP_TRIM";
    cap_trim.library = "Device";
    cap_trim.type = SymbolType::Component;
    cap_trim.footprint = "Capacitor_THT_Trimmer";
    cap_trim.description = "Trimmer Capacitor";
    symbols_["CAP_TRIM"] = cap_trim;
    SymbolDefinition ind_ferrite;
    ind_ferrite.name = "IND_FERRITE";
    ind_ferrite.library = "Device";
    ind_ferrite.type = SymbolType::Component;
    ind_ferrite.footprint = "SMD_0805";
    ind_ferrite.description = "Ferrite Bead";
    symbols_["IND_FERRITE"] = ind_ferrite;
    SymbolDefinition ind_var;
    ind_var.name = "IND_VAR";
    ind_var.library = "Device";
    ind_var.type = SymbolType::Component;
    ind_var.footprint = "Inductor_THT_Variable";
    ind_var.description = "Variable Inductor";
    symbols_["IND_VAR"] = ind_var;
    SymbolDefinition fuse;
    fuse.name = "FUSE";
    fuse.library = "Device";
    fuse.type = SymbolType::Component;
    fuse.footprint = "Fuse_THT_5x20mm";
    fuse.description = "Fuse";
    symbols_["FUSE"] = fuse;
    SymbolDefinition fuse_ptc;
    fuse_ptc.name = "FUSE_PTC";
    fuse_ptc.library = "Device";
    fuse_ptc.type = SymbolType::Component;
    fuse_ptc.footprint = "Fuse_SMD_1206";
    fuse_ptc.description = "PTC Resettable Fuse";
    symbols_["FUSE_PTC"] = fuse_ptc;
    SymbolDefinition crystal;
    crystal.name = "CRYSTAL";
    crystal.library = "Device";
    crystal.type = SymbolType::Component;
    crystal.footprint = "Crystal_HC49";
    crystal.description = "Crystal Oscillator";
    symbols_["CRYSTAL"] = crystal;
    SymbolDefinition resonator;
    resonator.name = "RESONATOR";
    resonator.library = "Device";
    resonator.type = SymbolType::Component;
    resonator.footprint = "Resonator_THT";
    resonator.description = "Ceramic Resonator";
    symbols_["RESONATOR"] = resonator;
    SymbolDefinition buzzer;
    buzzer.name = "BUZZER";
    buzzer.library = "Device";
    buzzer.type = SymbolType::Component;
    buzzer.footprint = "Buzzer_THT";
    buzzer.description = "Piezo Buzzer";
    symbols_["BUZZER"] = buzzer;
    SymbolDefinition relay_spst;
    relay_spst.name = "RELAY_SPST";
    relay_spst.library = "Device";
    relay_spst.type = SymbolType::Component;
    relay_spst.footprint = "Relay_THT_SPST";
    relay_spst.description = "SPST Relay";
    symbols_["RELAY_SPST"] = relay_spst;
    SymbolDefinition relay_dpdt;
    relay_dpdt.name = "RELAY_DPDT";
    relay_dpdt.library = "Device";
    relay_dpdt.type = SymbolType::Component;
    relay_dpdt.footprint = "Relay_THT_DPDT";
    relay_dpdt.description = "DPDT Relay";
    symbols_["RELAY_DPDT"] = relay_dpdt;
    SymbolDefinition photodiode;
    photodiode.name = "PHOTODIODE";
    photodiode.library = "Device";
    photodiode.type = SymbolType::Component;
    photodiode.footprint = "SMD_1206";
    photodiode.description = "Photodiode";
    symbols_["PHOTODIODE"] = photodiode;
    SymbolDefinition phototrans;
    phototrans.name = "PHOTOTRANS";
    phototrans.library = "Device";
    phototrans.type = SymbolType::Component;
    phototrans.footprint = "SMD_1206";
    phototrans.description = "Phototransistor";
    symbols_["PHOTOTRANS"] = phototrans;
    SymbolDefinition optocoupler;
    optocoupler.name = "OPTOCOUPLER";
    optocoupler.library = "Device";
    optocoupler.type = SymbolType::Component;
    optocoupler.footprint = "DIP-4";
    optocoupler.description = "Optocoupler";
    symbols_["OPTOCOUPLER"] = optocoupler;
    SymbolDefinition bridge_rect;
    bridge_rect.name = "BRIDGE_RECT";
    bridge_rect.library = "Device";
    bridge_rect.type = SymbolType::Component;
    bridge_rect.footprint = "Bridge_THT";
    bridge_rect.description = "Bridge Rectifier";
    symbols_["BRIDGE_RECT"] = bridge_rect;
    SymbolDefinition tvs;
    tvs.name = "TVS";
    tvs.library = "Device";
    tvs.type = SymbolType::Component;
    tvs.footprint = "SMD_1206";
    tvs.description = "TVS Diode";
    symbols_["TVS"] = tvs;
    SymbolDefinition schottky;
    schottky.name = "SCHOTTKY";
    schottky.library = "Device";
    schottky.type = SymbolType::Component;
    schottky.footprint = "SMD_1206";
    schottky.description = "Schottky Diode";
    symbols_["SCHOTTKY"] = schottky;
    SymbolDefinition diode_tunnel;
    diode_tunnel.name = "DIODE_TUNNEL";
    diode_tunnel.library = "Device";
    diode_tunnel.type = SymbolType::Component;
    diode_tunnel.footprint = "SMD_1206";
    diode_tunnel.description = "Tunnel Diode";
    symbols_["DIODE_TUNNEL"] = diode_tunnel;

    // --- Batch 2: Active & Semiconductor ---
    SymbolDefinition darlington;
    darlington.name = "DARLINGTON";
    darlington.library = "Device";
    darlington.type = SymbolType::Component;
    darlington.footprint = "TO-92";
    darlington.description = "Darlington Transistor";
    symbols_["DARLINGTON"] = darlington;
    SymbolDefinition jfet_n;
    jfet_n.name = "JFET_N";
    jfet_n.library = "Device";
    jfet_n.type = SymbolType::Component;
    jfet_n.footprint = "SOT-23";
    jfet_n.description = "N-Channel JFET";
    symbols_["JFET_N"] = jfet_n;
    SymbolDefinition jfet_p;
    jfet_p.name = "JFET_P";
    jfet_p.library = "Device";
    jfet_p.type = SymbolType::Component;
    jfet_p.footprint = "SOT-23";
    jfet_p.description = "P-Channel JFET";
    symbols_["JFET_P"] = jfet_p;
    SymbolDefinition scr;
    scr.name = "SCR";
    scr.library = "Device";
    scr.type = SymbolType::Component;
    scr.footprint = "TO-220";
    scr.description = "SCR Thyristor";
    symbols_["SCR"] = scr;
    SymbolDefinition triac;
    triac.name = "TRIAC";
    triac.library = "Device";
    triac.type = SymbolType::Component;
    triac.footprint = "TO-220";
    triac.description = "TRIAC";
    symbols_["TRIAC"] = triac;
    SymbolDefinition diac;
    diac.name = "DIAC";
    diac.library = "Device";
    diac.type = SymbolType::Component;
    diac.footprint = "SOD-123";
    diac.description = "DIAC";
    symbols_["DIAC"] = diac;
    SymbolDefinition igbt;
    igbt.name = "IGBT";
    igbt.library = "Device";
    igbt.type = SymbolType::Component;
    igbt.footprint = "TO-247";
    igbt.description = "IGBT";
    symbols_["IGBT"] = igbt;
    SymbolDefinition phototriac;
    phototriac.name = "PHOTOTRIAC";
    phototriac.library = "Device";
    phototriac.type = SymbolType::Component;
    phototriac.footprint = "DIP-4";
    phototriac.description = "Phototriac";
    symbols_["PHOTOTRIAC"] = phototriac;
    SymbolDefinition varactor;
    varactor.name = "VARACTOR";
    varactor.library = "Device";
    varactor.type = SymbolType::Component;
    varactor.footprint = "SOD-323";
    varactor.description = "Varactor Diode";
    symbols_["VARACTOR"] = varactor;
    SymbolDefinition led_rgb;
    led_rgb.name = "LED_RGB";
    led_rgb.library = "Device";
    led_rgb.type = SymbolType::Component;
    led_rgb.footprint = "LED_SMD_5050";
    led_rgb.description = "RGB LED";
    symbols_["LED_RGB"] = led_rgb;
    SymbolDefinition laser_diode;
    laser_diode.name = "LASER_DIODE";
    laser_diode.library = "Device";
    laser_diode.type = SymbolType::Component;
    laser_diode.footprint = "TO-18";
    laser_diode.description = "Laser Diode";
    symbols_["LASER_DIODE"] = laser_diode;
    SymbolDefinition solar_cell;
    solar_cell.name = "SOLAR_CELL";
    solar_cell.library = "Device";
    solar_cell.type = SymbolType::Component;
    solar_cell.footprint = "Solar_Cell_52x52mm";
    solar_cell.description = "Solar Cell";
    symbols_["SOLAR_CELL"] = solar_cell;
    SymbolDefinition led_7seg;
    led_7seg.name = "LED_7SEG";
    led_7seg.library = "Device";
    led_7seg.type = SymbolType::Component;
    led_7seg.footprint = "LED_7Seg_DIP10";
    led_7seg.description = "7-Segment LED";
    symbols_["LED_7SEG"] = led_7seg;
    SymbolDefinition thermocouple;
    thermocouple.name = "THERMOCOUPLE";
    thermocouple.library = "Device";
    thermocouple.type = SymbolType::Component;
    thermocouple.footprint = "Thermocouple_TypeK";
    thermocouple.description = "Thermocouple";
    symbols_["THERMOCOUPLE"] = thermocouple;
    SymbolDefinition peltier;
    peltier.name = "PELTIER";
    peltier.library = "Device";
    peltier.type = SymbolType::Component;
    peltier.footprint = "Peltier_40x40mm";
    peltier.description = "Peltier Element";
    symbols_["PELTIER"] = peltier;

    // --- Batch 3: IC Blocks ---
    SymbolDefinition lm393;
    lm393.name = "LM393";
    lm393.library = "IC";
    lm393.type = SymbolType::Component;
    lm393.footprint = "DIP-8";
    lm393.description = "LM393 Dual Comparator";
    symbols_["LM393"] = lm393;
    SymbolDefinition lm311;
    lm311.name = "LM311";
    lm311.library = "IC";
    lm311.type = SymbolType::Component;
    lm311.footprint = "DIP-8";
    lm311.description = "LM311 Comparator";
    symbols_["LM311"] = lm311;
    SymbolDefinition lm324;
    lm324.name = "LM324";
    lm324.library = "IC";
    lm324.type = SymbolType::Component;
    lm324.footprint = "DIP-14";
    lm324.description = "LM324 Quad Op-Amp";
    symbols_["LM324"] = lm324;
    SymbolDefinition ne5532;
    ne5532.name = "NE5532";
    ne5532.library = "IC";
    ne5532.type = SymbolType::Component;
    ne5532.footprint = "DIP-8";
    ne5532.description = "NE5532 Dual Op-Amp";
    symbols_["NE5532"] = ne5532;
    SymbolDefinition opa2134;
    opa2134.name = "OPA2134";
    opa2134.library = "IC";
    opa2134.type = SymbolType::Component;
    opa2134.footprint = "DIP-8";
    opa2134.description = "OPA2134 Audio Op-Amp";
    symbols_["OPA2134"] = opa2134;
    SymbolDefinition ad823;
    ad823.name = "AD823";
    ad823.library = "IC";
    ad823.type = SymbolType::Component;
    ad823.footprint = "SOIC-8";
    ad823.description = "AD823 Instr Amp";
    symbols_["AD823"] = ad823;
    SymbolDefinition ina128;
    ina128.name = "INA128";
    ina128.library = "IC";
    ina128.type = SymbolType::Component;
    ina128.footprint = "SOIC-8";
    ina128.description = "INA128 Precision Instr Amp";
    symbols_["INA128"] = ina128;
    SymbolDefinition lm386;
    lm386.name = "LM386";
    lm386.library = "IC";
    lm386.type = SymbolType::Component;
    lm386.footprint = "DIP-8";
    lm386.description = "LM386 Audio Amp";
    symbols_["LM386"] = lm386;
    SymbolDefinition tda7297;
    tda7297.name = "TDA7297";
    tda7297.library = "IC";
    tda7297.type = SymbolType::Component;
    tda7297.footprint = "TO-220-15";
    tda7297.description = "TDA7297 Audio Power Amp";
    symbols_["TDA7297"] = tda7297;
    SymbolDefinition max232;
    max232.name = "MAX232";
    max232.library = "IC";
    max232.type = SymbolType::Component;
    max232.footprint = "DIP-16";
    max232.description = "MAX232 RS-232 Driver";
    symbols_["MAX232"] = max232;
    SymbolDefinition max485;
    max485.name = "MAX485";
    max485.library = "IC";
    max485.type = SymbolType::Component;
    max485.footprint = "DIP-8";
    max485.description = "MAX485 RS-485 Transceiver";
    symbols_["MAX485"] = max485;
    SymbolDefinition max31855;
    max31855.name = "MAX31855";
    max31855.library = "IC";
    max31855.type = SymbolType::Component;
    max31855.footprint = "SOIC-8";
    max31855.description = "MAX31855 Thermocouple IF";
    symbols_["MAX31855"] = max31855;
    SymbolDefinition ads1115;
    ads1115.name = "ADS1115";
    ads1115.library = "IC";
    ads1115.type = SymbolType::Component;
    ads1115.footprint = "MSOP-10";
    ads1115.description = "ADS1115 16-bit ADC";
    symbols_["ADS1115"] = ads1115;
    SymbolDefinition mcp3008;
    mcp3008.name = "MCP3008";
    mcp3008.library = "IC";
    mcp3008.type = SymbolType::Component;
    mcp3008.footprint = "DIP-16";
    mcp3008.description = "MCP3008 10-bit ADC";
    symbols_["MCP3008"] = mcp3008;
    SymbolDefinition dac0808;
    dac0808.name = "DAC0808";
    dac0808.library = "IC";
    dac0808.type = SymbolType::Component;
    dac0808.footprint = "DIP-16";
    dac0808.description = "DAC0808 8-bit DAC";
    symbols_["DAC0808"] = dac0808;
    SymbolDefinition mcp4725;
    mcp4725.name = "MCP4725";
    mcp4725.library = "IC";
    mcp4725.type = SymbolType::Component;
    mcp4725.footprint = "SOT-23-5";
    mcp4725.description = "MCP4725 I2C DAC";
    symbols_["MCP4725"] = mcp4725;
    SymbolDefinition ds1307;
    ds1307.name = "DS1307";
    ds1307.library = "IC";
    ds1307.type = SymbolType::Component;
    ds1307.footprint = "DIP-8";
    ds1307.description = "DS1307 RTC";
    symbols_["DS1307"] = ds1307;

    // --- Batch 5: Power & RF ---
    SymbolDefinition lm2596;
    lm2596.name = "LM2596";
    lm2596.library = "IC";
    lm2596.type = SymbolType::Component;
    lm2596.footprint = "TO-220-5";
    lm2596.description = "LM2596 Buck Converter";
    symbols_["LM2596"] = lm2596;
    SymbolDefinition ams1117;
    ams1117.name = "AMS1117";
    ams1117.library = "IC";
    ams1117.type = SymbolType::Component;
    ams1117.footprint = "SOT-223";
    ams1117.description = "AMS1117 Regulator";
    symbols_["AMS1117"] = ams1117;
    SymbolDefinition irf520;
    irf520.name = "IRF520";
    irf520.library = "Device";
    irf520.type = SymbolType::Component;
    irf520.footprint = "TO-220";
    irf520.description = "IRF520 Power N-MOSFET";
    symbols_["IRF520"] = irf520;
    SymbolDefinition irf9530;
    irf9530.name = "IRF9530";
    irf9530.library = "Device";
    irf9530.type = SymbolType::Component;
    irf9530.footprint = "TO-220";
    irf9530.description = "IRF9530 Power P-MOSFET";
    symbols_["IRF9530"] = irf9530;
    SymbolDefinition irlz44;
    irlz44.name = "IRLZ44";
    irlz44.library = "Device";
    irlz44.type = SymbolType::Component;
    irlz44.footprint = "TO-220";
    irlz44.description = "IRLZ44 Logic-Level MOSFET";
    symbols_["IRLZ44"] = irlz44;

    // --- Batch 6: Quantum ---
    SymbolDefinition qubit;
    qubit.name = "QUBIT";
    qubit.library = "Quantum";
    qubit.type = SymbolType::Component;
    qubit.footprint = "Qubit";
    qubit.description = "Single Qubit";
    symbols_["QUBIT"] = qubit;
    SymbolDefinition hadamard;
    hadamard.name = "HADAMARD";
    hadamard.library = "Quantum";
    hadamard.type = SymbolType::Component;
    hadamard.footprint = "QGate";
    hadamard.description = "Hadamard Gate";
    symbols_["HADAMARD"] = hadamard;
    SymbolDefinition pauli_x;
    pauli_x.name = "PAULI_X";
    pauli_x.library = "Quantum";
    pauli_x.type = SymbolType::Component;
    pauli_x.footprint = "QGate";
    pauli_x.description = "Pauli-X Gate";
    symbols_["PAULI_X"] = pauli_x;
    SymbolDefinition cnot;
    cnot.name = "CNOT";
    cnot.library = "Quantum";
    cnot.type = SymbolType::Component;
    cnot.footprint = "QGate2";
    cnot.description = "CNOT Gate";
    symbols_["CNOT"] = cnot;
    SymbolDefinition measure;
    measure.name = "MEASURE";
    measure.library = "Quantum";
    measure.type = SymbolType::Component;
    measure.footprint = "QMeas";
    measure.description = "Measurement Gate";
    symbols_["MEASURE"] = measure;

    // --- Batch 7: Connectors & Electromechanical ---
    SymbolDefinition conn2;
    conn2.name = "CONN_2PIN";
    conn2.library = "Connector";
    conn2.type = SymbolType::Component;
    conn2.footprint = "Conn_THT_2pin";
    conn2.description = "2-Pin Connector";
    symbols_["CONN_2PIN"] = conn2;
    SymbolDefinition conn3;
    conn3.name = "CONN_3PIN";
    conn3.library = "Connector";
    conn3.type = SymbolType::Component;
    conn3.footprint = "Conn_THT_3pin";
    conn3.description = "3-Pin Connector";
    symbols_["CONN_3PIN"] = conn3;
    SymbolDefinition conn4;
    conn4.name = "CONN_4PIN";
    conn4.library = "Connector";
    conn4.type = SymbolType::Component;
    conn4.footprint = "Conn_THT_4pin";
    conn4.description = "4-Pin Connector";
    symbols_["CONN_4PIN"] = conn4;
    SymbolDefinition motor_dc;
    motor_dc.name = "MOTOR_DC";
    motor_dc.library = "Device";
    motor_dc.type = SymbolType::Component;
    motor_dc.footprint = "Motor_DC";
    motor_dc.description = "DC Motor";
    symbols_["MOTOR_DC"] = motor_dc;
    SymbolDefinition speaker;
    speaker.name = "SPEAKER";
    speaker.library = "Device";
    speaker.type = SymbolType::Component;
    speaker.footprint = "Speaker_8ohm";
    speaker.description = "Speaker";
    symbols_["SPEAKER"] = speaker;
    SymbolDefinition microphone;
    microphone.name = "MICROPHONE";
    microphone.library = "Device";
    microphone.type = SymbolType::Component;
    microphone.footprint = "Electret_Mic";
    microphone.description = "Electret Microphone";
    symbols_["MICROPHONE"] = microphone;
  }
};

SymbolLibrary::SymbolLibrary() : pImpl(std::make_unique<Impl>()) {
  pImpl->registerStandardSymbols();
}

SymbolLibrary::SymbolLibrary(const std::string &library_path)
    : pImpl(std::make_unique<Impl>()) {
  pImpl->library_path_ = library_path;
  pImpl->registerStandardSymbols();
  load(library_path);
}

SymbolLibrary::~SymbolLibrary() = default;

bool SymbolLibrary::load(const std::string &library_path) {
  pImpl->library_path_ = library_path;
  return true;
}

bool SymbolLibrary::save(const std::string &library_path) {
  pImpl->library_path_ = library_path;
  return true;
}

void SymbolLibrary::addSymbol(const SymbolDefinition &symbol) {
  pImpl->symbols_[symbol.name] = symbol;
}

void SymbolLibrary::removeSymbol(const std::string &name) {
  pImpl->symbols_.erase(name);
}

std::optional<SymbolDefinition>
SymbolLibrary::getSymbol(const std::string &name) const {
  auto it = pImpl->symbols_.find(name);
  if (it != pImpl->symbols_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<std::string> SymbolLibrary::listSymbols() const {
  std::vector<std::string> names;
  for (const auto &sym : pImpl->symbols_) {
    names.push_back(sym.first);
  }
  return names;
}

std::vector<std::string> SymbolLibrary::listLibraries() const {
  std::vector<std::string> libs;
  for (const auto &sym : pImpl->symbols_) {
    libs.push_back(sym.second.library);
  }
  return libs;
}

bool SymbolLibrary::hasSymbol(const std::string &name) const {
  return pImpl->symbols_.find(name) != pImpl->symbols_.end();
}

} // namespace deepiri