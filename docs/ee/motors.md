# Motors

A motor is an inductor with motion + **back-EMF**. Same flyback rules as relays/solenoids, plus commutation noise.

## DC brushed

| Need | Combination | Notes |
|------|-------------|-------|
| Drive from logic | MOSFET/BJT + motor series | Never MCU pin alone |
| Protect switch | Diode \|\| motor (reverse) | Freewheel |
| Kill brush EMI | Ceramic C \|\| terminals (~0.1µF) | Non-polar |
| Smooth PWM current | L series (choke) | Quieter, less ripple heat |
| Soft inrush | NTC or R + bypass relay | Limit startup surge |
| Forward/reverse | H-bridge | Four switches |

## Stepper

| Need | Coil wiring | Effect |
|------|-------------|--------|
| Holding / low-speed torque | **Series** | Higher R & L |
| High speed | **Parallel** | Lower L, needs ~2× driver current |

## Servo

Usually a self-contained driver + pot feedback — treat the power rails like any inductive/actuator bus: bulk C near the servo, star or plane return, never starve the MCU rail from motor di/dt.

## Efficiency tip

Resonant L–C with the motor's electrical frequency can recycle energy (advanced high-efficiency drives). For hobby boards, start with solid freewheel paths and local bulk capacitance.
