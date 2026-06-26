# Analog-AI Benchmarks

Reproducible workloads for egottol's analog compute and AI inference stack.

| Benchmark | File | What it exercises |
|-----------|------|-------------------|
| Thermistor classify | `../eii/thermistor_classify.json` | Sensor → impulse → EII digital readout |
| Hopfield recall | `hopfield_recall.json` | Associative memory + hopfield inference backend |
| Crossbar XOR | `xor_crossbar.py` | Memristor crossbar programmed for XOR |

## Run headless

```bash
poetry run python -c "
from egottol.engines.orchestrator import MultiDomainOrchestrator
from egottol.models.base import Circuit
import json
c = Circuit.model_validate(json.load(open('benchmarks/analog-ai/hopfield_recall.json')))
o = MultiDomainOrchestrator(c)
print(o.run_eii_window(0.01))
"
```

## CI

All benchmarks are validated in `tests/test_analog_ai_integration.py`.
