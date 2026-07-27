// Regression tests for the native EII (Electrical Impulse Inference) port:
// core/eii/{detectors,encoders,inference,actuators,pipeline}. Mirrors the
// plain-assert style of tests/core_regression_test.cpp and, where possible,
// the specific scenarios in tests/test_eii.py so the two ports stay aligned.
#include "../core/eii/actuators.h"
#include "../core/eii/detectors.h"
#include "../core/eii/encoders.h"
#include "../core/eii/inference.h"
#include "../core/eii/pipeline.h"
#include "../core/matrix.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

EIIConfig makeConfig() {
    EIIConfig cfg;
    cfg.dt = 1e-3;
    cfg.windowT = 0.05;
    cfg.embeddingDim = 2;
    cfg.numChannels = 2;
    cfg.detectorMode = DetectorMode::Threshold;
    cfg.encoderMode = EncoderMode::Rate;
    cfg.stimulusTime = 0.0;
    return cfg;
}

EIIState makeState() {
    EIIState s;
    s.t = 0.0;
    s.voltages = {0.0, 0.0};
    s.refractory = {0.0, 0.0};
    s.prevVoltages = {0.0, 0.0};
    s.prevEnvelope = {0.0, 0.0};
    s.prevDiffSignal = {0.0, 0.0};
    s.prevComparator = {0.0, 0.0};
    s.prevMemristorCurrent = {0.0, 0.0};
    s.currents = {0.0, 0.0};
    s.filterState = {0.0, 0.0};
    return s;
}

void test_threshold_detector_emits_on_crossing() {
    EIIConfig cfg = makeConfig();
    ImpulseDetector detector(cfg);
    EIIState state = makeState();
    state.voltages = {0.2, 0.8};

    auto events = detector.detect(state, cfg.dt);
    expect(events.size() == 1, "threshold detector should emit exactly one event");
    if (events.size() == 1) {
        expect(events[0].channel == 1, "threshold event should be on channel 1");
        expect(events[0].eventType == "threshold_cross", "event type should be threshold_cross");
        expect(std::abs(events[0].amplitude - 0.8) < 1e-12, "amplitude should equal the crossing voltage");
    }
}

void test_threshold_detector_respects_refractory() {
    EIIConfig cfg = makeConfig();
    ImpulseDetector detector(cfg);
    EIIState state = makeState();
    state.voltages = {0.9, 0.9};

    auto first = detector.detect(state, cfg.dt);
    state.t += cfg.dt;
    auto second = detector.detect(state, cfg.dt);

    expect(first.size() == 2, "first crossing on both channels should fire");
    expect(second.empty(), "immediate re-check should be suppressed by refractory period");
}

void test_comparator_detector_rising_edge_only() {
    EIIConfig cfg = makeConfig();
    cfg.detectorMode = DetectorMode::Comparator;
    ImpulseDetector detector(cfg);
    EIIState state = makeState();

    state.voltages = {0.2, 0.2};
    expect(detector.detect(state, cfg.dt).empty(), "comparator should not fire below threshold");

    state.voltages = {0.6, 0.2};
    auto events = detector.detect(state, cfg.dt);
    expect(events.size() == 1, "comparator should fire once on the rising edge");
    if (!events.empty()) {
        expect(events[0].channel == 0, "rising-edge event should be on channel 0");
    }
}

void test_rate_encoder_counts_spikes_in_window() {
    EIIConfig cfg = makeConfig();
    EncodingManifold encoder(cfg);
    std::vector<ImpulseEvent> events = {
        {0.01, 0, "spike", 1.0},
        {0.02, 0, "spike", 1.0},
        {0.03, 1, "spike", 1.0},
    };
    EIIState state = makeState();
    auto z = encoder.encode(state, events, 0.0);
    expect(z.size() == 2, "rate encoding should produce embedding_dim entries");
    expect(std::abs(z[0] - 2.0 / cfg.windowT) < 1e-9, "channel 0 rate should be 2 spikes / window");
    expect(std::abs(z[1] - 1.0 / cfg.windowT) < 1e-9, "channel 1 rate should be 1 spike / window");
}

void test_continuous_encoder_reads_probe_voltages() {
    EIIConfig cfg = makeConfig();
    cfg.encoderMode = EncoderMode::Continuous;
    cfg.embeddingDim = 3;
    EncodingManifold encoder(cfg);
    EIIState state = makeState();
    state.voltages = {1.1, 2.2, 3.3};
    auto z = encoder.encode(state, {}, 0.0);
    expect(z.size() == 3, "continuous encoding should have embedding_dim entries");
    expect(std::abs(z[0] - 1.1) < 1e-12 && std::abs(z[1] - 2.2) < 1e-12 && std::abs(z[2] - 3.3) < 1e-12,
           "continuous encoding should pass probe voltages through unchanged");
}

void test_filter_encoder_accumulates_decay() {
    EIIConfig cfg = makeConfig();
    cfg.encoderMode = EncoderMode::Filter;
    EncodingManifold encoder(cfg);
    EIIState state = makeState();
    std::vector<ImpulseEvent> events = {{0.001, 0, "spike", 0.5}};

    auto z1 = encoder.encode(state, events, 0.0);
    auto z2 = encoder.encode(state, {}, 0.0);

    expect(z1[0] > 0.0, "filter encoding should accumulate a positive value on impulse");
    expect(z2[0] < z1[0], "filter encoding should decay between windows with no new events");
}

void test_inference_digital_produces_normalized_distribution() {
    InferenceConfig icfg;
    icfg.backend = InferenceBackend::Digital;
    icfg.numClasses = 3;
    Matrix weights({{1.0, 0.0}, {0.0, 1.0}, {0.5, 0.5}});
    std::vector<double> bias = {0.0, 0.0, 0.0};
    Matrix conductance(3, 2, 0.0);
    InferenceEngine engine(icfg, weights, bias, conductance);

    auto [probs, confidence] = engine.infer({1.0, 0.0});
    expect(probs.size() == 3, "digital inference should produce num_classes outputs");
    double sum = probs[0] + probs[1] + probs[2];
    expect(std::abs(sum - 1.0) < 1e-9, "softmax output should sum to 1");
    expect(confidence >= 1.0 / 3.0 && confidence <= 1.0, "confidence should be a valid probability");
    expect(probs[0] > probs[1], "channel favored by weights should have higher probability");
}

void test_actuator_dac_clips_to_supply_rail() {
    ActuatorConfig acfg;
    acfg.mode = ActuatorMode::Dac;
    acfg.vDd = 3.3;
    FeedbackActuator actuator(acfg);
    EIIState state = makeState();

    auto control = actuator.actuate(state, {-0.5, 0.5, 1.5}, 0.9, {});
    expect(control.size() == 3, "DAC actuation should preserve vector length");
    expect(std::abs(control[0] - 0.0) < 1e-12, "negative prediction should clip to 0V");
    expect(std::abs(control[1] - 1.65) < 1e-9, "0.5 prediction should scale to half v_dd");
    expect(std::abs(control[2] - 3.3) < 1e-12, "prediction above 1 should clip to v_dd");
}

void test_pipeline_end_to_end_step_input() {
    EIIPipelineConfig cfg;
    cfg.eii.dt = 1e-3;
    cfg.eii.windowT = 5e-3;
    cfg.eii.embeddingDim = 2;
    cfg.eii.numChannels = 2;
    cfg.eii.detectorMode = DetectorMode::Threshold;
    cfg.eii.encoderMode = EncoderMode::Rate;
    cfg.inference.backend = InferenceBackend::Digital;
    cfg.inference.numClasses = 2;
    cfg.weights = Matrix({{1.0, 0.0}, {0.0, 1.0}});
    cfg.bias = {0.0, 0.0};
    cfg.conductance = Matrix(2, 2, 1e-3);
    cfg.actuator.mode = ActuatorMode::Dac;

    EIIPipeline pipeline(cfg);

    // Step input: both channels jump above the detector threshold and stay
    // there, so the Phi->Psi->Gamma loop should run at every window boundary
    // without crashing, and always produce sane, finite, non-empty output.
    bool sawInference = false;
    for (int step = 0; step < 20; ++step) {
        auto result = pipeline.step({0.8, 0.8}, {0.0, 0.0}, cfg.eii.dt);
        if (result.inferenceRan) {
            sawInference = true;
            expect(result.embedding.size() == 2, "embedding should have embedding_dim entries");
            expect(result.prediction.size() == 2, "prediction should have num_classes entries");
            expect(result.control.size() == 2, "control should have num_classes entries");
            expect(result.confidence >= 0.0 && result.confidence <= 1.0 + 1e-9,
                   "confidence should be a valid probability");
            for (double v : result.prediction) {
                expect(std::isfinite(v), "prediction values must be finite");
            }
            for (double v : result.control) {
                expect(std::isfinite(v), "control values must be finite");
            }
        }
    }
    expect(sawInference, "pipeline should run inference at least once over 20 steps with a 5-step window");

    // A step input on both channels should fire the threshold detector on the
    // very first step (0.8 >= 0.5 threshold).
    expect(!pipeline.history().empty(), "pipeline should record step history");
    expect(!pipeline.history().front().events.empty(), "first step should register threshold-cross events");
}

}  // namespace

int main() {
    test_threshold_detector_emits_on_crossing();
    test_threshold_detector_respects_refractory();
    test_comparator_detector_rising_edge_only();
    test_rate_encoder_counts_spikes_in_window();
    test_continuous_encoder_reads_probe_voltages();
    test_filter_encoder_accumulates_decay();
    test_inference_digital_produces_normalized_distribution();
    test_actuator_dac_clips_to_supply_rail();
    test_pipeline_end_to_end_step_input();

    if (failures == 0) {
        std::printf("All EII pipeline regression tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
