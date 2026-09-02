# Symptom → Solution Map

Scan **Symptom** → apply **Combination**. Full searchable index: `egottol/knowledge/ee_symptoms.json` (Copilot: `lookup_ee_design`).

## A — Passives / filters

| Symptom | Combination | Why |
|---------|-------------|-----|
| LED burned out | R + LED series | Limits current |
| Voltage too high for coil | R + coil series | Drops excess as heat |
| HF noise on rail | R\|\|C or C\|\|rail | Low-pass / shunt |
| DC offset into amp/speaker | C series (AC couple) | Blocks DC |
| Tweeter getting bass | R+C series HPF | Cuts bass |
| Need one radio station | L\|\|C tank | Peak Z at \(f_0\) |
| Need to reject one freq | L+C series trap | Min Z shorts \(f_0\) |

## B — Inductive kick / power

| Symptom | Combination | Why |
|---------|-------------|-----|
| Relay/solenoid/motor kills FET/BJT | **Diode \|\| coil (reverse)** | Flyback path — **always** |
| Need 5V→300V | L + switch + diode (boost) | Flyback energy to C |
| Need 12V→1.2V cool | Buck: FET+L+C+diode | Efficient step-down |
| Coil spike too high still | RC snubber \|\| switch | Soft dV/dt |

## C — Transistor speed / bias

| Symptom | Combination | Why |
|---------|-------------|-----|
| BJT amp too quiet | C\|\| emitter R | AC gain boost |
| BJT slow / hot switching | C\|\| base R (speed-up) | Force charge in/out |
| Mic DC messes bias | C series into base | AC couple |
| MOSFET floats on at boot | C\|\|G–S + pull-down R | Hold gate off |
| MOSFET rings / EMI | Miller C or gate bead | Kill dV/dt / bounce |
| Gate overvoltage | Zener \|\| G–S | Clamp oxide |

## D — Mechanical / MUX / crystal

| Symptom | Combination | Why |
|---------|-------------|-----|
| 3.3V MCU → 12V motor | Transistor + relay coil | Level / current amplify |
| Relay dims headlights | C\|\| coil | Hold-up inrush |
| Safety two-hand start | Relay contacts series | Both must close |
| Crystal won't start | R\|\| crystal (~1M) | Bias inverter linear |
| Clock drifts | Load C to GND each leg | Pull to rated \(f\) |
| Two clocks needed | Osc1 + Osc2 + **MUX** | Never parallel crystals |
| One ADC, many sensors | MUX + sample/hold C | Time-multiplex |

## E — Motors (short)

| Symptom | Combination | Why |
|---------|-------------|-----|
| MCU pin drives motor | **Wrong** — use FET/driver | Current / flyback |
| Motor EMI on radio | C\|\| motor terminals | Brush noise snubber |
| Bidirectional spin | H-bridge (4 FETs) | Reverse polarity |
| Stepper torque @ low speed | Coils **series** | Higher L/R |
| Stepper speed | Coils **parallel** | Faster di/dt (2× I) |
