from typing import Dict, List, Any, Optional
from pydantic import BaseModel
from egottol.models.base import ComponentType, Port

class ComponentDefinition(BaseModel):
    name: str
    category: ComponentType
    symbol: str  # SVG or Path for UI
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
    
    # --- ACTIVE ---
    "NPN": ComponentDefinition(
        name="NPN BJT", category=ComponentType.ACTIVE, symbol="Q_NPN",
        ports=[Port(name="C", direction="inout"), Port(name="B", direction="inout"), Port(name="E", direction="inout")],
        parameters={"model": "2N2222"}
    ),
    "NMOS": ComponentDefinition(
        name="NMOS MOSFET", category=ComponentType.ACTIVE, symbol="M_NMOS",
        ports=[Port(name="D", direction="inout"), Port(name="G", direction="inout"), Port(name="S", direction="inout")],
        parameters={"Vto": 2.0, "Kp": 1e-3}
    ),

    # --- LOGIC (VHDL COMPATIBLE) ---
    "AND": ComponentDefinition(
        name="AND Gate", category=ComponentType.LOGIC, symbol="GATE_AND",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 10}
    ),
    "XOR": ComponentDefinition(
        name="XOR Gate", category=ComponentType.LOGIC, symbol="GATE_XOR",
        ports=[Port(name="A", direction="in"), Port(name="B", direction="in"), Port(name="Q", direction="out")],
        parameters={"delay_ns": 12}
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

# Add massive vendor models (placeholder for 10k+ parts)
for i in range(100):
    COMPONENT_LIBRARY[f"OPAMP_{i}"] = ComponentDefinition(
        name=f"OpAmp Model {i}", category=ComponentType.IC_BLOCK, symbol="OPAMP",
        ports=[Port(name="+", direction="in"), Port(name="-", direction="in"), Port(name="OUT", direction="out")],
        parameters={"gain": 1e5, "v_offset": 0.001}
    )
