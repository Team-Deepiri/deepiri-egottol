# Golden Rules (Non-negotiable)

1. **Inductive load (motor, relay, solenoid, coil)** → flyback diode (or proper sync/snubber). No exceptions.
2. **LED** → series current-limit resistor.
3. **Every IC** → local HF decouple (typ. 0.1µF) with tiny loop to GND; add bulk nearby.
4. **Crystals** → never series/parallel with each other; use **MUX** to select oscillators; use load caps + bias R as datasheet says.
5. **Series** divides voltage / shared current; **Parallel** divides current / shared voltage.
6. **C\|\|node** smooths voltage; **L series** smooths current.
7. **Floorplan dirty / digital / clean** before routing.
8. **Protection before bulk** at the connector.
9. **MCU pin ≠ motor driver.**
10. **Datasheet absolute max ≠ recommended operating** — derate.

Copilot: ask *“LED burned”* / *“relay kills MOSFET”* → `lookup_ee_design`.
