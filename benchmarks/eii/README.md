# EII Benchmarks

Electrical Impulse Inference (EII) benchmarks exercise the full **Φ → Ψ → Γ** pipeline on representative circuits: impulse extraction, encoding, digital readout, and closed-loop actuation.

## Benchmarks

| Circuit | File | Task | Success metric |
|---------|------|------|----------------|
| Thermistor classify | `thermistor_classify.json` | NTC sensor → LIF spikes → rate encoding → binary overtemperature class → relay/buzzer | ≥ 95% window accuracy over 50 °C–120 °C sweep |
| SDR burst detect | `sdr_burst_detect.egt` *(planned)* | RF envelope impulses → filter embedding → burst vs noise class | F1 ≥ 0.90 on synthetic ADS-B bursts |

## Running

Headless EII benchmark (requires `egottol[gpu]` optional for large crossbar readouts):

```bash
poetry install -E gpu
poetry run python -m egottol.benchmarks.eii.run --circuit benchmarks/eii/thermistor_classify.json \
  --weights benchmarks/eii/thermistor_classify.egt-weights \
  --duration 1.0
```

Load a circuit JSON into the schematic editor and attach weights from a `.egt-weights` file (schema: `io/eii_weights.schema.json`).

## Circuit format

Benchmark circuits use the Pydantic `Circuit` model (`egottol.models.base`): `id`, `name`, `components[]`, `wires[]`. EII blocks (`IMPULSE_DETECTOR`, `INFERENCE_ENCODER`, `INFERENCE_ENGINE`, `FEEDBACK_ACTUATOR`) store pipeline parameters in `parameters` and `metadata.eii`.

## CI regression

Benchmarks run in CI with fixed seeds and tolerance bands on:

- Spike count per window
- Embedding norm `‖z‖₂`
- Classification accuracy / confidence
- Actuator command latency (windows until relay trips)

Add new benchmarks under this directory with matching `.egt-weights` readout files and a row in the table above.
