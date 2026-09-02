# Transistors with R / C / L

## BJT + Cap

| Combination | Behavior | Applied use |
|-------------|----------|-------------|
| C series with Base | Blocks DC, passes AC | AC coupling (mics, pedals) |
| C \|\| B–E | Shunts HF to GND | EMI / RFI suppress |
| C \|\| base resistor | Speed-up: dump charge in/out | Fast switching (old SMPS/CRT) |
| C \|\| emitter R | AC bypass → max AC gain | Audio preamps |
| C \|\| C–E | Soft snubber | Small inductive loads |

## MOSFET + Cap

| Combination | Behavior | Applied use |
|-------------|----------|-------------|
| C series with Gate | AC-coupled gate, delayed Vgs | Class-D style coupling |
| C \|\| G–S | Soft-start / absorb leakage | Inrush control, float protect |
| C \|\| D–G (Miller) | Slows dV/dt | Spike reduction (heat ↑) |
| C \|\| D–S | Resonant / ZVS help | Wireless charge, soft switch |

## BJT + Inductor

| Combination | Behavior | Applied use |
|-------------|----------|-------------|
| L series Collector | Stores energy; flyback spike off | Boost / ignition |
| L series Emitter | Current limit via feedback | Short protection |
| L \|\| B–E | RF choke on base | Radio front-ends |
| L + diode (flyback) | Safe recirculation | **Every inductive load** |

## MOSFET + Inductor

| Combination | Behavior | Applied use |
|-------------|----------|-------------|
| L series Drain | Buck/boost energy store | CPU VRMs, chargers |
| L series Source | Current sense / limit | Short protection |
| L \|\| D–S | Class-E / resonant | Qi, RF TX |
| L series Gate (bead) | Kill gate bounce | Motor controllers |
| MOSFET+L+C (buck) | Efficient step-down | 12V→5V/1.2V |
| LLC resonant | Near-zero Z at \(f_0\) | 80+ Titanium PSUs |

## Hybrid / topology

| Combo | Behavior | Use |
|-------|----------|-----|
| Darlington (BJT+BJT) | \(\beta\) multiplies | Tiny drive → big load |
| Cascode (BJT+MOSFET series) | Fast switch + high V | HV / RF / inverters |
| Sziklai / complementary | Best of both | Linear supplies |
| MOSFET body diode | Built-in D–S diode | Sync buck dead-time |

**BJT vs MOSFET reminder:** BJT = current into base; MOSFET = voltage on gate capacitance. BJT needs flyback diode or dies; MOSFET body diode buys a little time but still needs proper freewheel path for motors/relays.
