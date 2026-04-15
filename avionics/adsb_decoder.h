#pragma once

#include <string>
#include <complex>
#include <vector>
#include <cstdint>
#include <memory>

namespace deepiri {

enum class ADSBSqiType {
    Surface,
    Airborne,
    Ground
};

struct ADSBAircraft {
    uint32_t icao24;
    std::string callsign;
    double latitude;
    double longitude;
    double altitude;
    double ground_speed;
    double track_angle;
    ADSBSqiType sqi_type;
    int vertical_rate;
    uint32_t timestamp;
};

class ADSBDemodulator {
public:
    ADSBDemodulator();
    ~ADSBDemodulator();

    void processIQ(const std::vector<std::complex<float>>& samples);
    std::vector<ADSBAircraft> getAircraft();

    void setCenterFrequency(double freq_hz);
    void setSampleRate(double rate_sps);

    int getDetectedCount() const;
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}