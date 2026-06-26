# EII Operator Specification

Electrical Impulse Inference (EII) maps continuous circuit physics to discrete information and back. Three operators form a closed loop over window `T`:

```
Physics → Φ → Ψ → Γ → Physics
```

State at time `t`: `x(t) = [V(t); I_b(t); G(t); φ_opt(t); s_logic(t)]`.

Impulses: `e_k = (t_k, channel_i, type, amplitude)` with `type ∈ {spike, threshold_cross, memristor_switch, rf_burst_peak}`.

---

## Φ — Encoding manifold

Maps impulses and probes to embedding `z ∈ ℝᵈ` over window `[t−T, t]`:

```
z = Φ(x(t), {e_k}, T)
```

| Mode | Definition |
|------|------------|
| Rate | `z_i = N_spikes_i / T` |
| Latency | `z_i = exp(−(t_first_i − t_stim) / τ_lat)` |
| Filter | `z_i = Σ_k α^(t−t_k) · A_k` or synaptic kernel `(S_i * h)(T)` |
| Population | `z = pool([z_1, …, z_N])` across channels |
| Continuous | `z = V_probe` (direct analog readout) |

Read noise (optional): `z ← z + ε`, `ε ~ N(0, Σ_read)`.

---

## Ψ — Inference engine

Consumes `z`, produces prediction `ŷ` and confidence `p`:

```
(ŷ, p) = Ψ(z; W, θ_Ψ)
```

| Backend | Map |
|---------|-----|
| Analog | `ŷ = Q_ADC(G · DAC⁻¹(z))` |
| Digital (linear) | `ŷ = softmax(W · z + b)` |
| Digital (MLP) | `ŷ = softmax(W₂ · σ(W₁ · z + b₁) + b₂)` |
| Energy-based | `ŷ* = argmin_ŷ [−ŷᵀ W z + ½ ŷᵀ L ŷ + λ_spike · ‖S‖₀]` |

Weights `W`, `b` load from `.egt-weights` (see `io/eii_weights.schema.json`).

---

## Γ — Feedback actuator

Maps inference output to circuit drive at `t + Δt`:

```
u(t + Δt) = Γ(ŷ, p, x(t))
```

| Mode | Action |
|------|--------|
| DAC | `V_row = V_dd · ŷ` |
| STDP | `G ← G + η · f(Δt) · (1 − p_correct)` |
| Digital | `s_logic ← f(ŷ)` (register / relay) |
| Optical | `φ_j ← φ_j + Δφ(ŷ_j)` |

---

## Discrete timestep

```
1. x(t+dt) ← physics step (MNA ∪ photonic ∪ digital)
2. E_t ← impulse detectors on x
3. if t mod T == 0:
     z ← Φ(x, E_{t−T:t})
     ŷ, p ← Ψ(z)
     u ← Γ(ŷ, p, x)
```

Implementation: `egottol/engines/eii/pipeline.py`.
