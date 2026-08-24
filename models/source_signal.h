#pragma once

#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace deepiri {

// Shared independent-source waveforms (SPICE V/I source functions).
enum class SourceWaveform { DC, AC, PULSE, SIN, EXP, PWL };

struct SourceSignal {
    SourceWaveform kind = SourceWaveform::DC;
    double dc = 0.0;
    double acMag = 0.0;
    double acPhaseDeg = 0.0;

    // PULSE(v1 v2 td tr tf pw per)
    double pulseV1 = 0, pulseV2 = 1, pulseTd = 0, pulseTr = 1e-9, pulseTf = 1e-9;
    double pulsePw = 1e-3, pulsePer = 2e-3;

    // SIN(vo va freq td theta phase)
    double sinVo = 0, sinVa = 1, sinFreq = 1e3, sinTd = 0, sinTheta = 0, sinPhase = 0;

    // EXP(v1 v2 td1 tau1 td2 tau2)
    double expV1 = 0, expV2 = 1, expTd1 = 0, expTau1 = 1e-9, expTd2 = 1e-9, expTau2 = 1e-9;

    // PWL(t1 v1 t2 v2 …)
    std::vector<double> pwlT;
    std::vector<double> pwlV;

    double eval(double t) const {
        switch (kind) {
            case SourceWaveform::DC:
                return dc;
            case SourceWaveform::AC:
                return acMag * std::cos(2.0 * M_PI * t * 1e9 + acPhaseDeg * M_PI / 180.0);
            case SourceWaveform::PULSE: {
                if (t < pulseTd) return pulseV1;
                double tl = t - pulseTd;
                if (pulsePer > 0) tl = std::fmod(tl, pulsePer);
                if (tl < pulseTr) {
                    return pulseV1 + (pulseV2 - pulseV1) * tl / std::max(pulseTr, 1e-30);
                }
                if (tl < pulseTr + pulsePw) return pulseV2;
                if (tl < pulseTr + pulsePw + pulseTf) {
                    return pulseV2 + (pulseV1 - pulseV2) *
                           (tl - pulseTr - pulsePw) / std::max(pulseTf, 1e-30);
                }
                return pulseV1;
            }
            case SourceWaveform::SIN: {
                if (t < sinTd) {
                    return sinVo + sinVa * std::sin(sinPhase * M_PI / 180.0);
                }
                double tt = t - sinTd;
                double damp = (sinTheta > 0) ? std::exp(-tt * sinTheta) : 1.0;
                double arg = 2.0 * M_PI * sinFreq * tt + sinPhase * M_PI / 180.0;
                return sinVo + sinVa * damp * std::sin(arg);
            }
            case SourceWaveform::EXP: {
                if (t < expTd1) return expV1;
                if (t < expTd2) {
                    return expV1 + (expV2 - expV1) *
                           (1.0 - std::exp(-(t - expTd1) / std::max(expTau1, 1e-30)));
                }
                double rise = expV1 + (expV2 - expV1) *
                              (1.0 - std::exp(-(expTd2 - expTd1) / std::max(expTau1, 1e-30)));
                return rise + (expV1 - rise) *
                       (1.0 - std::exp(-(t - expTd2) / std::max(expTau2, 1e-30)));
            }
            case SourceWaveform::PWL: {
                if (pwlT.empty()) return dc;
                if (t <= pwlT.front()) return pwlV.front();
                if (t >= pwlT.back()) return pwlV.back();
                for (size_t i = 1; i < pwlT.size(); ++i) {
                    if (t <= pwlT[i]) {
                        double t0 = pwlT[i - 1], t1 = pwlT[i];
                        double v0 = pwlV[i - 1], v1 = pwlV[i];
                        double a = (t - t0) / std::max(t1 - t0, 1e-30);
                        return v0 + a * (v1 - v0);
                    }
                }
                return pwlV.back();
            }
        }
        return dc;
    }
};

// Apply waveform flags from a parsed netlist element (named_parameters + parameters).
inline void applyWaveformFromParams(
    SourceSignal& s,
    const std::map<std::string, double>& named,
    const std::vector<double>& params
) {
    auto has = [&](const char* k) { return named.count(k) != 0; };
    auto p = [&](size_t i, double fb) { return i < params.size() ? params[i] : fb; };

    if (has("pulse")) {
        s.kind = SourceWaveform::PULSE;
        s.pulseV1 = p(0, 0);
        s.pulseV2 = p(1, 1);
        s.pulseTd = p(2, 0);
        s.pulseTr = p(3, 1e-9);
        s.pulseTf = p(4, 1e-9);
        s.pulsePw = p(5, 1e-3);
        s.pulsePer = p(6, 2e-3);
        s.dc = s.pulseV1;
    } else if (has("sin")) {
        s.kind = SourceWaveform::SIN;
        s.sinVo = p(0, 0);
        s.sinVa = p(1, 1);
        s.sinFreq = p(2, 1e3);
        s.sinTd = p(3, 0);
        s.sinTheta = p(4, 0);
        s.sinPhase = p(5, 0);
        s.dc = s.sinVo;
    } else if (has("exp")) {
        s.kind = SourceWaveform::EXP;
        s.expV1 = p(0, 0);
        s.expV2 = p(1, 1);
        s.expTd1 = p(2, 0);
        s.expTau1 = p(3, 1e-9);
        s.expTd2 = p(4, p(2, 0) + 1e-9);
        s.expTau2 = p(5, 1e-9);
        s.dc = s.expV1;
    } else if (has("pwl")) {
        s.kind = SourceWaveform::PWL;
        s.pwlT.clear();
        s.pwlV.clear();
        for (size_t i = 0; i + 1 < params.size(); i += 2) {
            s.pwlT.push_back(params[i]);
            s.pwlV.push_back(params[i + 1]);
        }
        if (!s.pwlV.empty()) s.dc = s.pwlV.front();
    } else if (has("ac")) {
        s.kind = SourceWaveform::DC;
        s.dc = p(0, s.dc);
        if (params.size() >= 2) {
            s.acMag = params[1];
            s.acPhaseDeg = p(2, 0);
        } else if (params.size() == 1) {
            // AC 1 alone after DC keyword path may put mag in params[0]
            s.acMag = params[0];
        }
    } else if (!params.empty()) {
        s.kind = SourceWaveform::DC;
        s.dc = params[0];
    }
}

}  // namespace
