from typing import Dict, List, Any, Optional
from pydantic import BaseModel
from egottol.models.base import ComponentType, Port


class ComponentDefinition(BaseModel):
    name: str
    category: ComponentType
    symbol: str  # symbol key for UI renderer
    ports: List[Port]
    parameters: Dict[str, Any]


COMPONENT_LIBRARY: Dict[str, ComponentDefinition] = {
    # --- PASSIVE ---
    "RES": ComponentDefinition(
        name="Resistor", category=ComponentType.PASSIVE, symbol="R",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"R": 1000.0, "tolerance": 0.05}
    ),
    "CAP": ComponentDefinition(
        name="Capacitor", category=ComponentType.PASSIVE, symbol="C",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"C": 1e-6, "esr": 0.01}
    ),
    "IND": ComponentDefinition(
        name="Inductor", category=ComponentType.PASSIVE, symbol="L",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"L": 1e-3}
    ),
    "DIODE": ComponentDefinition(
        name="Diode", category=ComponentType.PASSIVE, symbol="DIODE",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"Is": 1e-14, "n": 1.0}
    ),
    "ZENER": ComponentDefinition(
        name="Zener Diode", category=ComponentType.PASSIVE, symbol="ZENER",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"Vz": 5.1, "Iz": 20e-3}
    ),
    "LED": ComponentDefinition(
        name="LED", category=ComponentType.PASSIVE, symbol="LED",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"color": "red", "Vf": 2.0}
    ),
    "SW": ComponentDefinition(
        name="Switch", category=ComponentType.PASSIVE, symbol="SW",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"state": "open"}
    ),
    "XFMR": ComponentDefinition(
        name="Transformer", category=ComponentType.PASSIVE, symbol="XFMR",
        ports=[
            Port(name="P1", direction="inout"), Port(name="P2", direction="inout"),
            Port(name="S1", direction="inout"), Port(name="S2", direction="inout"),
        ],
        parameters={"turns_ratio": 1.0}
    ),
    "POT": ComponentDefinition(
        name="Potentiometer", category=ComponentType.PASSIVE, symbol="POT",
        ports=[
            Port(name="1", direction="inout"),
            Port(name="W", direction="inout"),
            Port(name="2", direction="inout"),
        ],
        parameters={"R": 10000, "wiper": 0.5}
    ),

    # --- PASSIVE EXPANSION ---
    "VARISTOR": ComponentDefinition(
        name="Varistor", category=ComponentType.PASSIVE, symbol="VARISTOR",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"V_clamp": 18.0, "energy_J": 1.0}
    ),
    "THERM_NTC": ComponentDefinition(
        name="NTC Thermistor", category=ComponentType.PASSIVE, symbol="THERM_NTC",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"R_25": 10000.0, "B": 3950.0}
    ),
    "THERM_PTC": ComponentDefinition(
        name="PTC Thermistor", category=ComponentType.PASSIVE, symbol="THERM_PTC",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"R_25": 100.0, "trip_current_A": 0.5}
    ),
    "TRIMMER": ComponentDefinition(
        name="Trimmer Pot", category=ComponentType.PASSIVE, symbol="TRIMMER",
        ports=[Port(name="1", direction="inout"), Port(name="W", direction="inout"), Port(name="2", direction="inout")],
        parameters={"R": 10000.0, "turns": 25}
    ),
    "CAP_ELEC": ComponentDefinition(
        name="Electrolytic Cap", category=ComponentType.PASSIVE, symbol="CAP_ELEC",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"C": 100e-6, "V_rated": 25.0, "esr": 0.5}
    ),
    "CAP_CER": ComponentDefinition(
        name="Ceramic Cap", category=ComponentType.PASSIVE, symbol="CAP_CER",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"C": 100e-9, "V_rated": 50.0, "dielectric": "X7R"}
    ),
    "CAP_FILM": ComponentDefinition(
        name="Film Cap", category=ComponentType.PASSIVE, symbol="CAP_FILM",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"C": 1e-6, "V_rated": 100.0, "tolerance": 0.01}
    ),
    "CAP_TANT": ComponentDefinition(
        name="Tantalum Cap", category=ComponentType.PASSIVE, symbol="CAP_TANT",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"C": 10e-6, "V_rated": 16.0, "esr": 1.0}
    ),
    "CAP_TRIM": ComponentDefinition(
        name="Trimmer Cap", category=ComponentType.PASSIVE, symbol="CAP_TRIM",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"C_min": 5e-12, "C_max": 50e-12}
    ),
    "IND_FERRITE": ComponentDefinition(
        name="Ferrite Bead", category=ComponentType.PASSIVE, symbol="IND_FERRITE",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"Z_100MHz": 600.0, "I_max": 2.0}
    ),
    "IND_VAR": ComponentDefinition(
        name="Variable Inductor", category=ComponentType.PASSIVE, symbol="IND_VAR",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"L_min": 1e-6, "L_max": 100e-6}
    ),
    "FUSE": ComponentDefinition(
        name="Fuse", category=ComponentType.PASSIVE, symbol="FUSE",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"I_rated": 1.0, "V_rated": 250.0, "type": "fast"}
    ),
    "FUSE_PTC": ComponentDefinition(
        name="PTC Resettable Fuse", category=ComponentType.PASSIVE, symbol="FUSE_PTC",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"I_hold": 0.5, "I_trip": 1.0, "R_min": 0.1}
    ),
    "CRYSTAL": ComponentDefinition(
        name="Crystal Oscillator", category=ComponentType.PASSIVE, symbol="CRYSTAL",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"freq": 16e6, "CL": 18e-12, "ESR": 40.0}
    ),
    "RESONATOR": ComponentDefinition(
        name="Ceramic Resonator", category=ComponentType.PASSIVE, symbol="RESONATOR",
        ports=[Port(name="1", direction="inout"), Port(name="GND", direction="inout"), Port(name="2", direction="inout")],
        parameters={"freq": 8e6, "CL": 30e-12}
    ),
    "BUZZER": ComponentDefinition(
        name="Piezo Buzzer", category=ComponentType.ELECTROMECHANICAL, symbol="BUZZER",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"freq": 4000, "SPL_dB": 85}
    ),
    "RELAY_SPST": ComponentDefinition(
        name="Relay SPST", category=ComponentType.ELECTROMECHANICAL, symbol="RELAY_SPST",
        ports=[
            Port(name="COIL+", direction="inout"), Port(name="COIL−", direction="inout"),
            Port(name="COM", direction="inout"), Port(name="NO", direction="inout"),
        ],
        parameters={"V_coil": 5.0, "R_coil": 70.0, "I_contact": 2.0}
    ),
    "RELAY_DPDT": ComponentDefinition(
        name="Relay DPDT", category=ComponentType.ELECTROMECHANICAL, symbol="RELAY_DPDT",
        ports=[
            Port(name="COIL+", direction="inout"), Port(name="COIL−", direction="inout"),
            Port(name="C1", direction="inout"), Port(name="NO1", direction="inout"), Port(name="NC1", direction="inout"),
            Port(name="C2", direction="inout"), Port(name="NO2", direction="inout"), Port(name="NC2", direction="inout"),
        ],
        parameters={"V_coil": 12.0, "R_coil": 120.0, "I_contact": 5.0}
    ),
    "PHOTODIODE": ComponentDefinition(
        name="Photodiode", category=ComponentType.PASSIVE, symbol="PHOTODIODE",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"lambda_peak": 850e-9, "responsivity": 0.5}
    ),
    "PHOTOTRANS": ComponentDefinition(
        name="Phototransistor", category=ComponentType.ACTIVE, symbol="PHOTOTRANS",
        ports=[Port(name="C", direction="inout"), Port(name="E", direction="inout")],
        parameters={"V_ceo": 30.0, "I_c_max": 0.02}
    ),
    "OPTOCOUPLER": ComponentDefinition(
        name="Optocoupler", category=ComponentType.IC_BLOCK, symbol="OPTOCOUPLER",
        ports=[
            Port(name="AN", direction="inout"), Port(name="CA", direction="inout"),
            Port(name="C", direction="inout"), Port(name="E", direction="inout"),
        ],
        parameters={"CTR": 1.0, "V_iso": 5000}
    ),
    "BRIDGE_RECT": ComponentDefinition(
        name="Bridge Rectifier", category=ComponentType.PASSIVE, symbol="BRIDGE_RECT",
        ports=[
            Port(name="~1", direction="inout"), Port(name="~2", direction="inout"),
            Port(name="+", direction="inout"), Port(name="−", direction="inout"),
        ],
        parameters={"V_rrm": 400.0, "I_o": 1.5}
    ),
    "TVS": ComponentDefinition(
        name="TVS Diode", category=ComponentType.PASSIVE, symbol="TVS",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"V_br": 6.8, "I_pp": 50.0}
    ),
    "SCHOTTKY": ComponentDefinition(
        name="Schottky Diode", category=ComponentType.PASSIVE, symbol="SCHOTTKY",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"V_f": 0.3, "V_rrm": 40.0, "I_o": 1.0}
    ),
    "DIODE_TUNNEL": ComponentDefinition(
        name="Tunnel Diode", category=ComponentType.PASSIVE, symbol="DIODE_TUNNEL",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"I_p": 1e-3, "V_p": 0.065, "C_j": 10e-12}
    ),

    # --- ACTIVE ---
    "NPN": ComponentDefinition(
        name="NPN BJT", category=ComponentType.ACTIVE, symbol="Q_NPN",
        ports=[Port(name="C", direction="inout"), Port(name="B", direction="inout"), Port(name="E", direction="inout")],
        parameters={"model": "2N2222"}
    ),
    "PNP": ComponentDefinition(
        name="PNP BJT", category=ComponentType.ACTIVE, symbol="Q_PNP",
        ports=[Port(name="C", direction="inout"), Port(name="B", direction="inout"), Port(name="E", direction="inout")],
        parameters={"model": "2N3906"}
    ),
    "NMOS": ComponentDefinition(
        name="NMOS MOSFET", category=ComponentType.ACTIVE, symbol="M_NMOS",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vto": 2.0, "Kp": 1e-3}
    ),
    "PMOS": ComponentDefinition(
        name="PMOS MOSFET", category=ComponentType.ACTIVE, symbol="M_PMOS",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vto": -2.0, "Kp": 1e-3}
    ),

    # --- ACTIVE EXPANSION ---
    "DARLINGTON": ComponentDefinition(
        name="Darlington NPN", category=ComponentType.ACTIVE, symbol="DARLINGTON",
        ports=[Port(name="C", direction="inout"), Port(name="B", direction="inout"), Port(name="E", direction="inout")],
        parameters={"hFE": 10000, "V_ceo": 60.0}
    ),
    "JFET_N": ComponentDefinition(
        name="N-JFET", category=ComponentType.ACTIVE, symbol="JFET_N",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vp": -4.0, "Idss": 10e-3}
    ),
    "JFET_P": ComponentDefinition(
        name="P-JFET", category=ComponentType.ACTIVE, symbol="JFET_P",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vp": 4.0, "Idss": 10e-3}
    ),
    "SCR": ComponentDefinition(
        name="SCR Thyristor", category=ComponentType.ACTIVE, symbol="SCR",
        ports=[Port(name="A", direction="inout"), Port(name="G", direction="inout"), Port(name="K", direction="inout")],
        parameters={"V_drm": 600.0, "I_t": 25.0}
    ),
    "TRIAC": ComponentDefinition(
        name="TRIAC", category=ComponentType.ACTIVE, symbol="TRIAC",
        ports=[Port(name="MT1", direction="inout"), Port(name="G", direction="inout"), Port(name="MT2", direction="inout")],
        parameters={"V_drm": 400.0, "I_t": 16.0}
    ),
    "DIAC": ComponentDefinition(
        name="DIAC", category=ComponentType.ACTIVE, symbol="DIAC",
        ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
        parameters={"V_bo": 32.0, "I_bo": 100e-6}
    ),
    "IGBT": ComponentDefinition(
        name="IGBT N-Channel", category=ComponentType.ACTIVE, symbol="IGBT",
        ports=[Port(name="C", direction="inout"), Port(name="G", direction="inout"), Port(name="E", direction="inout")],
        parameters={"V_ces": 600.0, "V_ge_th": 5.0, "I_c": 50.0}
    ),
    "PHOTOTRIAC": ComponentDefinition(
        name="Phototriac", category=ComponentType.ACTIVE, symbol="PHOTOTRIAC",
        ports=[Port(name="AN", direction="inout"), Port(name="CA", direction="inout"), Port(name="MT1", direction="inout"), Port(name="MT2", direction="inout")],
        parameters={"V_drm": 400.0, "I_t": 1.0}
    ),
    "VARACTOR": ComponentDefinition(
        name="Varactor Diode", category=ComponentType.PASSIVE, symbol="VARACTOR",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"C_j0": 20e-12, "V_br": 30.0}
    ),
    "LED_RGB": ComponentDefinition(
        name="RGB LED", category=ComponentType.PASSIVE, symbol="LED_RGB",
        ports=[Port(name="R", direction="inout"), Port(name="G", direction="inout"), Port(name="B", direction="inout"), Port(name="C", direction="inout")],
        parameters={"Vf_red": 2.0, "Vf_grn": 3.0, "Vf_blu": 3.0}
    ),
    "LASER_DIODE": ComponentDefinition(
        name="Laser Diode", category=ComponentType.PASSIVE, symbol="LASER_DIODE",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout"), Port(name="PD", direction="inout")],
        parameters={"lambda": 780e-9, "P_o": 5e-3}
    ),
    "DIODE_SHOCKLEY": ComponentDefinition(
        name="Shockley Diode", category=ComponentType.ACTIVE, symbol="DIODE",
        ports=[Port(name="A", direction="inout"), Port(name="K", direction="inout")],
        parameters={"V_bo": 20.0}
    ),
    "HBT": ComponentDefinition(
        name="HBT NPN", category=ComponentType.ACTIVE, symbol="Q_NPN",
        ports=[Port(name="C", direction="inout"), Port(name="B", direction="inout"), Port(name="E", direction="inout")],
        parameters={"hFE": 500, "fT": 50e9}
    ),
    "HEMT": ComponentDefinition(
        name="HEMT GaN", category=ComponentType.ACTIVE, symbol="M_NMOS",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vto": -3.0, "Idss": 5.0, "fT": 100e9}
    ),
    "MESFET": ComponentDefinition(
        name="MESFET N", category=ComponentType.ACTIVE, symbol="M_NMOS",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vp": -2.0, "Idss": 0.5}
    ),
    "SOLAR_CELL": ComponentDefinition(
        name="Solar Cell", category=ComponentType.PASSIVE, symbol="SOLAR_CELL",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"V_oc": 0.6, "I_sc": 0.1, "eff": 0.20}
    ),
    "LED_7SEG": ComponentDefinition(
        name="7-Segment LED", category=ComponentType.PASSIVE, symbol="LED_7SEG",
        ports=[
            Port(name="A", direction="inout"), Port(name="B", direction="inout"),
            Port(name="C", direction="inout"), Port(name="D", direction="inout"),
            Port(name="E", direction="inout"), Port(name="F", direction="inout"),
            Port(name="G", direction="inout"), Port(name="DP", direction="inout"),
            Port(name="COM", direction="inout"),
        ],
        parameters={"type": "common_cathode", "Vf": 2.0}
    ),
    "LED_MATRIX": ComponentDefinition(
        name="LED Matrix 8x8", category=ComponentType.PASSIVE, symbol="DEFAULT",
        ports=[Port(name="ROW", direction="inout"), Port(name="COL", direction="inout")],
        parameters={"rows": 8, "cols": 8}
    ),
    "DIODE_BRIDGE_MOD": ComponentDefinition(
        name="Diode Bridge Module", category=ComponentType.ACTIVE, symbol="BRIDGE_RECT",
        ports=[
            Port(name="~1", direction="inout"), Port(name="~2", direction="inout"),
            Port(name="+", direction="inout"), Port(name="−", direction="inout"),
        ],
        parameters={"V_rrm": 600.0, "I_o": 35.0}
    ),
    "THERMOCOUPLE": ComponentDefinition(
        name="Thermocouple", category=ComponentType.SENSOR, symbol="THERMOCOUPLE",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"type": "K", "seebeck": 41e-6}
    ),
    "PELTIER": ComponentDefinition(
        name="Peltier Element", category=ComponentType.ACTIVE, symbol="PELTIER",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"I_max": 6.0, "V_max": 15.4, "Q_max": 50.0}
    ),
    "MEMS_ACCEL": ComponentDefinition(
        name="MEMS Accelerometer", category=ComponentType.SENSOR, symbol="DEFAULT",
        ports=[Port(name="VDD", direction="inout"), Port(name="GND", direction="inout"), Port(name="OUT", direction="out")],
        parameters={"range_g": 16, "sensitivity": 0.004}
    ),
    "MEMS_GYRO": ComponentDefinition(
        name="MEMS Gyroscope", category=ComponentType.SENSOR, symbol="DEFAULT",
        ports=[Port(name="VDD", direction="inout"), Port(name="GND", direction="inout"), Port(name="OUT", direction="out")],
        parameters={"range_dps": 2000, "sensitivity": 0.07}
    ),
    "PHOTO_VOLTAIC": ComponentDefinition(
        name="Photovoltaic Module", category=ComponentType.PASSIVE, symbol="SOLAR_CELL",
        ports=[Port(name="+", direction="inout"), Port(name="−", direction="inout")],
        parameters={"V_oc": 12.0, "I_sc": 2.0, "P_max": 20.0}
    ),
    "CURRENT_MIRROR": ComponentDefinition(
        name="Current Mirror", category=ComponentType.ACTIVE, symbol="DEFAULT",
        ports=[Port(name="I_in", direction="in"), Port(name="I_out", direction="out"), Port(name="VCC", direction="inout"), Port(name="GND", direction="inout")],
        parameters={"ratio": 1.0, "I_ref": 1e-3}
    ),

    # --- LOGIC ---
    "AND": ComponentDefinition(
        name="AND Gate", category=ComponentType.LOGIC, symbol="GATE_AND",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 10}
    ),
    "OR": ComponentDefinition(
        name="OR Gate", category=ComponentType.LOGIC, symbol="GATE_OR",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 10}
    ),
    "XOR": ComponentDefinition(
        name="XOR Gate", category=ComponentType.LOGIC, symbol="GATE_XOR",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 12}
    ),
    "NAND": ComponentDefinition(
        name="NAND Gate", category=ComponentType.LOGIC, symbol="GATE_NAND",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 10}
    ),
    "NOR": ComponentDefinition(
        name="NOR Gate", category=ComponentType.LOGIC, symbol="GATE_NOR",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 10}
    ),
    "NOT": ComponentDefinition(
        name="NOT Gate", category=ComponentType.LOGIC, symbol="GATE_NOT",
        ports=[Port(name="A", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 5}
    ),
    "DFF": ComponentDefinition(
        name="D Flip-Flop", category=ComponentType.LOGIC, symbol="DFF",
        ports=[
            Port(name="D", direction="in"), Port(name="CLK", direction="in"),
            Port(name="Q", direction="out"), Port(name="QB", direction="out"),
        ],
        parameters={"delay_ns": 5}
    ),
    "MUX2": ComponentDefinition(
        name="2:1 Mux", category=ComponentType.LOGIC, symbol="MUX2",
        ports=[
            Port(name="A", direction="in"), Port(name="B", direction="in"),
            Port(name="SEL", direction="in"), Port(name="Q", direction="out"),
        ],
        parameters={"delay_ns": 10}
    ),

    # --- SOURCES & POWER ---
    "VSRC": ComponentDefinition(
        name="Voltage Source", category=ComponentType.SOURCE, symbol="VSRC",
        ports=[Port(name="+", direction="inout"), Port(name="-", direction="inout")],
        parameters={"V": 5.0, "freq": 0.0}
    ),
    "ISRC": ComponentDefinition(
        name="Current Source", category=ComponentType.SOURCE, symbol="ISRC",
        ports=[Port(name="+", direction="inout"), Port(name="-", direction="inout")],
        parameters={"I": 0.001}
    ),
    "GND": ComponentDefinition(
        name="Ground", category=ComponentType.POWER, symbol="GND",
        ports=[Port(name="G", direction="inout")],
        parameters={}
    ),
    "VCC": ComponentDefinition(
        name="VCC Rail", category=ComponentType.POWER, symbol="VCC",
        ports=[Port(name="V", direction="inout")],
        parameters={"V": 5.0}
    ),

    # --- IC BLOCKS ---
    "LM741": ComponentDefinition(
        name="Op-Amp LM741", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[
            Port(name="+", direction="in"), Port(name="-", direction="in"),
            Port(name="OUT", direction="out"),
            Port(name="V+", direction="inout"), Port(name="V-", direction="inout"),
        ],
        parameters={"gain": 200000, "slew_rate": 0.5e6}
    ),
    "LM358": ComponentDefinition(
        name="Op-Amp LM358", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[
            Port(name="+", direction="in"), Port(name="-", direction="in"),
            Port(name="OUT", direction="out"),
        ],
        parameters={"gain": 100000, "slew_rate": 0.6e6}
    ),
    "TL071": ComponentDefinition(
        name="Op-Amp TL071", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[
            Port(name="+", direction="in"), Port(name="-", direction="in"),
            Port(name="OUT", direction="out"),
        ],
        parameters={"gain": 200000, "slew_rate": 13e6}
    ),
    "555": ComponentDefinition(
        name="555 Timer", category=ComponentType.IC_BLOCK, symbol="IC_555",
        ports=[
            Port(name="VCC", direction="inout"), Port(name="GND", direction="inout"),
            Port(name="OUT", direction="out"), Port(name="TRIG", direction="in"),
            Port(name="THRES", direction="in"), Port(name="RESET", direction="in"),
            Port(name="CV", direction="inout"), Port(name="DISCH", direction="inout"),
        ],
        parameters={"mode": "astable"}
    ),
    "7805": ComponentDefinition(
        name="5V Regulator", category=ComponentType.IC_BLOCK, symbol="IC_REG",
        ports=[
            Port(name="IN", direction="inout"), Port(name="GND", direction="inout"),
            Port(name="OUT", direction="inout"),
        ],
        parameters={"V_out": 5.0, "I_max": 1.5}
    ),
    "LM317": ComponentDefinition(
        name="Adj Regulator LM317", category=ComponentType.IC_BLOCK, symbol="IC_REG",
        ports=[
            Port(name="IN", direction="inout"), Port(name="ADJ", direction="inout"),
            Port(name="OUT", direction="inout"),
        ],
        parameters={"V_out": 1.25}
    ),

    # Named OPAMP variants
    "OPAMP_UA741": ComponentDefinition(
        name="OpAmp uA741", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[Port(name="+", direction="in"), Port(name="-", direction="in"), Port(name="OUT", direction="out")],
        parameters={"gain": 200000, "v_offset": 0.001}
    ),
    "OPAMP_LF356": ComponentDefinition(
        name="OpAmp LF356", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[Port(name="+", direction="in"), Port(name="-", direction="in"), Port(name="OUT", direction="out")],
        parameters={"gain": 200000, "v_offset": 0.002}
    ),
    "OPAMP_AD8055": ComponentDefinition(
        name="OpAmp AD8055", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[Port(name="+", direction="in"), Port(name="-", direction="in"), Port(name="OUT", direction="out")],
        parameters={"gain": 200000, "slew_rate": 1400e6}
    ),

    # --- EXPERIMENTAL / AVIONICS ---
    "ADSB_TX": ComponentDefinition(
        name="ADS-B Transponder", category=ComponentType.RF, symbol="RF_TX",
        ports=[Port(name="ANT", direction="out"), Port(name="DATA", direction="in")],
        parameters={"icao": "0xABCDEF", "power_dbm": 20}
    ),
    "NSP_AI": ComponentDefinition(
        name="Neural Signal Processor", category=ComponentType.EXPERIMENTAL, symbol="AI_BLOCK",
        ports=[Port(name="IN", direction="in"), Port(name="OUT", direction="out")],
        parameters={"model": "signal_cleaner_v1.pth"}
    ),
}
