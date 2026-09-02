# Electrical Design Knowledge (Egottol EE)

Production design knowledge embedded in **deepiri-egottol**: series/parallel rules, component combinations, symptom→fix maps, motors, PCB floorplanning, and Copilot lookup.

| Doc | Contents |
|-----|----------|
| [series-parallel.md](series-parallel.md) | R/C/L/diode/BJT/MOSFET/crystal/relay/solenoid series vs parallel |
| [combinations.md](combinations.md) | RC/RL/LC, diode mixes, RLC, voltage multipliers, flyback |
| [transistors.md](transistors.md) | BJT/MOSFET + R/C/L; Darlington, cascode, hybrid |
| [symptom-solver.md](symptom-solver.md) | Symptom → combination → applied use (designer flowchart) |
| [motors.md](motors.md) | DC / stepper / servo drive & protection |
| [pcb-design.md](pcb-design.md) | PDN, grounding, SI, floorplanning, bring-up |
| [golden-rules.md](golden-rules.md) | Non-negotiables (flyback diode, decoupling, zones) |

**In product:** Copilot tool `lookup_ee_design` searches `egottol/knowledge/ee_symptoms.json`.  
**Sim fixtures:** `tests/fixtures/design/*.cir` for classic topologies.
