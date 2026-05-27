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
