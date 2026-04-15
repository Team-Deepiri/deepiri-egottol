#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace deepiri {

struct GDL90Message {
    uint8_t message_id;
    std::vector<uint8_t> payload;
};

struct OwnshipReport {
    uint32_t icao_address;
    double latitude;
    double longitude;
    double altitude;
    double ground_speed;
    double track_angle;
    int vertical_rate;
    uint32_t timestamp;
};

class GDL90Encoder {
public:
    GDL90Encoder();
    ~GDL90Encoder();

    std::vector<uint8_t> encodeMessage(const GDL90Message& msg);
    std::vector<uint8_t> encodeOwnshipReport(const OwnshipReport& report);
    std::vector<uint8_t> encodeHeartbeat();

    void setCallsign(const std::string& callsign);
    void setICAOAddress(uint32_t icao);

    bool sendToNetwork(const std::string& host, uint16_t port);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}