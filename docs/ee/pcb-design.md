# PCB Design & Floorplanning (Egottol EE)

## Design flow (don't skip)

Requirements → Architecture → Block diagram / power budget → Topology → Parts → Hierarchical schematic → Review → Footprints → **Floorplan** → Route (high-speed first) → DRC/ERC → SI/PI/thermal → Fab → Bring-up → Production.

*"We'll fix it in layout"* is expensive. Fix architecture → schematic → placement first.

## Floorplan zones

| Zone | Put here | Keep away from |
|------|----------|----------------|
| **Dirty** | Power entry, bucks, motor drivers, relays | Analog front-ends |
| **Digital brain** | MCU, crystal, flash | Straddles dirty/clean as bridge |
| **Clean** | Op-amps, sensors, ADCs | Switching loops |

### Power waterfall (physical order)

`Outside → Protection (TVS/ESD) → Bulk C → Regulation → Local decouple → IC`

Protection faces the threat **before** bulk ceramics (ESD into an unprotected MLCC kills dielectric).

### Analog cleanroom order

`Sensor → RC LPF → Amp → Anti-alias → ADC pin`

Filter near the conversion — long post-amp traces are antennas.

### Proximity rule

| Distance | Parts |
|----------|-------|
| 0–2 mm | TVS/ESD, 0.1µF HF decouple — pad-adjacent |
| 2–10 mm | Feedback dividers, inductors, crystals |
| 10+ mm | Bulk C, connectors, LEDs, big switches |

## PDN & grounding (short)

- Target impedance: \(Z_{target} = \Delta V_{max}/\Delta I_{max}\).
- Decouple hierarchy: bulk → mid µF → 0.1µF → pF at pins.
- Prefer continuous ground plane; don't split under high-speed.
- Mixed-signal: one plane + **physical** separation often beats naive star ground at HF.

## SI / EMI

- Fast edges → transmission lines; terminate when length ≈ rise-time distance.
- Minimize loop area (hot loops at bucks, return under signals).
- 45° bends; matched length on buses / differential pairs.

## Bring-up

Visual → ohms power-to-GND → current-limited first power → rails on scope → clocks → program → peripherals → full function.

## Lab discipline (from #circuits)

- No scrap metal / wire bits left on boards.
- Enough insulation clearance on power rails.
- Inspect fillets, bridges, voids, flux under scope.
