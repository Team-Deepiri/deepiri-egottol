# Mixed Component Combinations

## Passive mixes

| Combo | Behavior | Applied use |
|-------|----------|-------------|
| **R+C series** | High-pass (blocks DC, passes AC) | Audio crossovers, AC coupling |
| **R\|\|C** | Low-pass (passes DC, shunts HF noise) | Decoupling next to ICs |
| **R+L series** | Low-pass / choke | Power supply chokes |
| **R\|\|L** | High-pass | Speaker crossover branches |
| **L+C series** | Lowest Z at resonance | Series resonant, antenna match |
| **L\|\|C** | Highest Z at resonance (tank) | Radio tuners, band-stop |
| **R+L+C series** | Band-pass / dead-short at \(f_0\) | Antenna matching |
| **R\|\|L\|\|C** | Band-stop / tuner + bandwidth set by R | Parallel RLC filters |

## Diode mixes

| Combo | Behavior | Applied use |
|-------|----------|-------------|
| **R + Diode series** | Limits diode/LED current | **Mandatory for LEDs** |
| **R \|\| Diode** | Clamp / bypass when forward | Protection clamps |
| **C + Diode series** | Charge pump / voltage multiply | Cockcroft–Walton, flash HV |
| **C \|\| Diode** | Soft recovery / snubber | Spike absorb at diode turn-off |
| **L + Diode series** | Peak detector style | RF peak detect |
| **L \|\| Diode (reverse)** | **Freewheeling / flyback** | Relay, solenoid, motor — **non-negotiable** |

## Classic multi-part

| Circuit | Parts | Use |
|---------|-------|-----|
| Buck | MOSFET + L series + C\|\|load + diode (or sync FET) | Efficient step-down |
| Boost | L series to switch node + diode + C\|\|out | Step-up |
| Flyback | Coupled L + switch + secondary diode | Isolated adapters |
| Voltage doubler | C–Diode–C | Flash / CRT era HV |
| Common-emitter amp | BJT + bias R + coupling C | Mic preamps |
| 555-style RC osc | R charges C, transistor dumps C | Clocks, blinkers, buzzers |
| Relay/solenoid driver | Transistor + coil + **flyback diode** | Any inductive actuator |

Simulate: `tests/fixtures/design/`.
