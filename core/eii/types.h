#pragma once

#include <map>
#include <string>
#include <vector>

namespace deepiri {

enum class DetectorMode { Threshold, Differentiator, Comparator, RfEnvelope, MemristorSwitch };
enum class EncoderMode { Rate, Latency, Filter, Population, Continuous };
enum class InferenceBackend { Analog, Digital, EnergyBased };
enum class DigitalHead { Linear, Softmax };
enum class ActuatorMode { Dac, Stdp, Digital, Optical };

struct ImpulseEvent {
    double t = 0.0;
    int channel = 0;
    std::string eventType;
    double amplitude = 0.0;
};

// Mirrors egottol/engines/eii/types.py::EIIConfig. detectors.py and encoders.py
// read fields directly off this flat config (not the nested pydantic configs in
// egottol/models/eii.py) — see pipeline.h for why that distinction matters.
struct EIIConfig {
    double dt = 1e-4;
    double windowT = 10e-3;
    int embeddingDim = 8;
    int numChannels = 4;
    DetectorMode detectorMode = DetectorMode::Threshold;
    EncoderMode encoderMode = EncoderMode::Filter;
    InferenceBackend inferenceBackend = InferenceBackend::Digital;
    DigitalHead digitalHead = DigitalHead::Linear;
    ActuatorMode actuatorMode = ActuatorMode::Dac;
    double stimulusTime = 0.0;
    double readNoiseStd = 0.0;
};

// Mirrors egottol/engines/eii/types.py::EIIState. Optional numpy arrays
// (conductances, optical_phases, embedding, prediction) become vectors that are
// empty when "unset".
struct EIIState {
    double t = 0.0;
    std::vector<double> voltages;
    std::vector<double> currents;
    std::vector<double> conductances;
    std::vector<double> opticalPhases;
    std::map<std::string, double> logicRegisters;
    std::vector<double> refractory;
    std::vector<double> prevVoltages;
    std::vector<double> prevEnvelope;
    std::vector<double> prevDiffSignal;
    std::vector<double> prevComparator;
    std::vector<double> prevMemristorCurrent;
    std::vector<ImpulseEvent> eventHistory;
    std::vector<double> filterState;
    std::vector<double> embedding;
    std::vector<double> prediction;
    double confidence = 0.0;
    std::vector<double> control;
    std::vector<ImpulseEvent> windowEvents;
};

}  // namespace deepiri
