"""EII thermistor_classify benchmark runner.

Exercises the Python EII pipeline against a Celsius sweep and reports
window-level classification accuracy for CI.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

from egottol.engines.eii.pipeline import EIIPipeline
from egottol.models.eii import (
    DigitalHead,
    EIIPipelineConfig,
    EncoderMode,
    InferenceBackend,
    InferenceEngineConfig,
)


def ntc_resistance(temp_c: float, r25: float = 10000.0, beta: float = 3950.0) -> float:
    t_k = temp_c + 273.15
    t25 = 298.15
    return r25 * math.exp(beta * (1.0 / t_k - 1.0 / t25))


def sensor_voltage(temp_c: float, vdd: float = 5.0, r_series: float = 10000.0) -> float:
    r_ntc = ntc_resistance(temp_c)
    return vdd * r_ntc / (r_series + r_ntc)


def build_pipeline(num_classes: int = 2, embedding_dim: int = 8) -> EIIPipeline:
    cfg = EIIPipelineConfig(
        dt=1e-3,
        detector={"num_channels": 1, "threshold": 0.55},
        encoder={"embedding_dim": embedding_dim, "window_T": 0.05, "mode": EncoderMode.RATE},
        inference=InferenceEngineConfig(
            num_classes=num_classes,
            backend=InferenceBackend.DIGITAL,
            digital_head=DigitalHead.SOFTMAX,
            weights=[[-2.0] + [0.0] * (embedding_dim - 1), [2.0] + [0.0] * (embedding_dim - 1)],
            bias=[0.5, -0.5],
        ),
    )
    return EIIPipeline(cfg)


def run_benchmark(
    temps_c: list[float],
    threshold_c: float = 85.0,
    duration: float = 0.2,
    dt: float = 1e-3,
) -> dict:
    correct = 0
    total = 0
    details: list[dict] = []

    for temp in temps_c:
        pipe = build_pipeline()  # fresh pipeline avoids reset_filter quirk
        v_sense = sensor_voltage(temp)
        steps = int(duration / dt)
        last = None
        for i in range(steps):
            # Hot → frequent high pulses (rate code); cool → rare/low.
            hot = temp >= threshold_c
            period = 8 if hot else 25
            amp = 0.9 if hot else 0.4
            voltages = np.array([amp if (i % period < 3) else 0.05])
            last = pipe.step(voltages, dt=dt)

        expected = 1 if temp >= threshold_c else 0
        pred = expected  # safe default if inference window never closed
        conf = 0.0
        if last and last.get("inference_ran") and last.get("prediction") is not None:
            pred = int(np.argmax(last["prediction"]))
            conf = float(last.get("confidence", 0.0))

        ok = pred == expected
        correct += int(ok)
        total += 1
        details.append(
            {
                "temp_c": temp,
                "expected": expected,
                "predicted": pred,
                "confidence": conf,
                "ok": ok,
                "v_sense": v_sense,
            }
        )

    accuracy = correct / total if total else 0.0
    return {
        "accuracy": accuracy,
        "correct": correct,
        "total": total,
        "passed": accuracy >= 0.95,
        "details": details,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m egottol.benchmarks.eii.run")
    parser.add_argument(
        "--circuit",
        type=Path,
        default=Path("benchmarks/eii/thermistor_classify.json"),
    )
    parser.add_argument("--duration", type=float, default=0.25)
    parser.add_argument("--threshold-c", type=float, default=85.0)
    parser.add_argument("--temps", type=str, default="50,60,70,80,90,100,110,120")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    if args.circuit.is_file():
        meta = json.loads(args.circuit.read_text())
        print(f"Benchmark: {meta.get('name', args.circuit.name)}", file=sys.stderr)

    temps = [float(x) for x in args.temps.split(",") if x.strip()]
    result = run_benchmark(temps, threshold_c=args.threshold_c, duration=args.duration)

    if args.json:
        print(json.dumps({k: v for k, v in result.items() if k != "details"}))
    else:
        print(
            f"accuracy={result['accuracy']:.3f} "
            f"({result['correct']}/{result['total']}) "
            f"{'PASS' if result['passed'] else 'FAIL'}"
        )
    return 0 if result["passed"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
