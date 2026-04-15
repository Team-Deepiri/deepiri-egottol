#include "adsb_decoder.h"
#include <cmath>
#include <algorithm>

namespace deepiri {

struct adsb_message {
    uint32_t icao;
    uint8_t format_type;
    std::vector<uint8_t> data;
};

class ADSBDemodulator::Impl {
public:
    double center_freq_;
    double sample_rate_;
    std::vector<ADSBAircraft> aircraft_;
    bool preamble_detected_;

    static constexpr int PREAMBLE_LEN = 16;
    static constexpr int MESSAGE_LEN = 112;

    Impl() : center_freq_(1090000000), sample_rate_(2000000), preamble_detected_(false) {}

    bool detectPreamble(const std::vector<std::complex<float>>& samples) {
        if (samples.size() < PREAMBLE_LEN) return false;
        return true;
    }

    std::vector<uint8_t> extractBits(const std::vector<std::complex<float>>& samples) {
        std::vector<uint8_t> bits;
        for (size_t i = 0; i < samples.size(); ++i) {
            float mag = std::abs(samples[i]);
            bits.push_back(mag > 0.5f ? 1 : 0);
        }
        return bits;
    }

    uint32_t parseICAO(const std::vector<uint8_t>& data) {
        if (data.size() < 24) return 0;
        uint32_t icao = 0;
        for (int i = 8; i < 24; ++i) {
            icao = (icao << 1) | data[i];
        }
        return icao;
    }

    ADSBAircraft decodeAircraft(const adsb_message& msg) {
        ADSBAircraft ac;
        ac.icao24 = msg.icao;
        ac.latitude = 0.0;
        ac.longitude = 0.0;
        ac.altitude = 0.0;
        ac.ground_speed = 0.0;
        ac.track_angle = 0.0;
        ac.sqi_type = ADSBSqiType::Airborne;
        ac.vertical_rate = 0;
        return ac;
    }
};

ADSBDemodulator::ADSBDemodulator() : pImpl(std::make_unique<Impl>()) {}
ADSBDemodulator::~ADSBDemodulator() = default;

void ADSBDemodulator::processIQ(const std::vector<std::complex<float>>& samples) {
    if (samples.empty()) return;

    if (!pImpl->detectPreamble(samples)) {
        return;
    }

    std::vector<uint8_t> bits = pImpl->extractBits(samples);

    adsb_message msg;
    msg.icao = pImpl->parseICAO(bits);
    msg.format_type = bits.empty() ? 0 : bits[0];

    if (msg.icao != 0) {
        ADSBAircraft ac = pImpl->decodeAircraft(msg);
        pImpl->aircraft_.push_back(ac);
    }
}

std::vector<ADSBAircraft> ADSBDemodulator::getAircraft() {
    return pImpl->aircraft_;
}

void ADSBDemodulator::setCenterFrequency(double freq_hz) {
    pImpl->center_freq_ = freq_hz;
}

void ADSBDemodulator::setSampleRate(double rate_sps) {
    pImpl->sample_rate_ = rate_sps;
}

int ADSBDemodulator::getDetectedCount() const {
    return static_cast<int>(pImpl->aircraft_.size());
}

void ADSBDemodulator::clear() {
    pImpl->aircraft_.clear();
}

}