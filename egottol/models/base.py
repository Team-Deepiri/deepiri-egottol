from enum import Enum
from typing import List, Optional, Dict, Any, Union
from pydantic import BaseModel, Field

class ComponentType(str, Enum):
    PASSIVE = "passive"
    ACTIVE = "active"
    SOURCE = "source"
    LOGIC = "logic"
    IC_BLOCK = "ic_block"
    POWER = "power"
    MATH = "math"
    RF = "rf"
    EXPERIMENTAL = "experimental"
    UQE_BRIDGE = "uqe_bridge"
    GPU_COMPUTE = "gpu_compute"
    QUANTUM = "quantum"
    SENSOR = "sensor"
    ELECTROMECHANICAL = "electromechanical"
    CONNECTOR = "connector"

class Port(BaseModel):
    name: str
    direction: str  # "in", "out", "inout"
    value: float = 0.0

class Component(BaseModel):
    id: str
    name: str
    type: ComponentType
    ports: List[Port] = []
    parameters: Dict[str, Any] = {}
    metadata: Dict[str, Any] = {}

class Wire(BaseModel):
    id: str
    from_component: str
    from_port: str
    to_component: str
    to_port: str

class Circuit(BaseModel):
    id: str
    name: str
    components: List[Component] = []
    wires: List[Wire] = []

# --- Standard Library ---
class Resistor(Component):
    type: ComponentType = ComponentType.PASSIVE
    name: str = "Resistor"

class Capacitor(Component):
    type: ComponentType = ComponentType.PASSIVE
    name: str = "Capacitor"

class Diode(Component):
    type: ComponentType = ComponentType.ACTIVE
    name: str = "Diode"

class MOSFET(Component):
    type: ComponentType = ComponentType.ACTIVE
    name: str = "MOSFET"

class LogicGate(Component):
    type: ComponentType = ComponentType.LOGIC

# --- Airspace & RF ---
class ADSBNode(Component):
    type: ComponentType = ComponentType.RF
    name: str = "ADS-B_Transponder"

class SDRNode(Component):
    type: ComponentType = ComponentType.RF
    name: str = "SDR_Decoder"

# --- Experimental ---
class NeuralSignalProcessor(Component):
    type: ComponentType = ComponentType.EXPERIMENTAL
    name: str = "NSP_AI"

class BehavioralSuperNode(Component):
    type: ComponentType = ComponentType.MATH
    name: str = "Behavioral_Node"

# --- Quantum ---
class QuantumGate(Component):
    type: ComponentType = ComponentType.QUANTUM

# --- Sensors ---
class Sensor(Component):
    type: ComponentType = ComponentType.SENSOR

# --- Electromechanical ---
class Electromechanical(Component):
    type: ComponentType = ComponentType.ELECTROMECHANICAL

# --- Connectors ---
class Connector(Component):
    type: ComponentType = ComponentType.CONNECTOR
