# Series vs Parallel — Component Basics

| Component | In Series | In Parallel |
|-----------|-----------|-------------|
| **Resistor (R)** | Adds: \(R_t = R_1+R_2\). Current same. | Reciprocal: \(1/R_t = 1/R_1+1/R_2\). Voltage same. |
| **Capacitor (C)** | Reciprocal: \(1/C_t = 1/C_1+1/C_2\) (total ↓) | Adds: \(C_t = C_1+C_2\) (total ↑) |
| **Inductor (L)** | Adds: \(L_t = L_1+L_2\) | Reciprocal: \(1/L_t = 1/L_1+1/L_2\) |
| **Diode** | Vf drops add (0.7+0.7=1.4). Short fails both. | Current capacity adds; one short kills the other — use ballast R. |
| **BJT** | Darlington: \(\beta_1\times\beta_2\) ultra-high gain | Heat sharing — need emitter ballast R against thermal runaway |
| **MOSFET** | Rare (they fight) — use cascode carefully | High current: gates / drains / sources tied; positive tempco shares current |
| **Crystal** | **Never** — won't oscillate | **Never** — fight frequencies. Use a **MUX** between oscillators |
| **Relay coil** | Voltage drops add (high V supply → low V coil) | Current draw doubles (two isolated loads, one signal) |
| **Solenoid** | Slower response, lower force | Faster response, higher force |

## Golden duals

- **Series** divides voltage, keeps current the same.
- **Parallel** divides current, keeps voltage the same.
- **C in parallel** → smooths voltage.
- **L in series** → smooths current.
