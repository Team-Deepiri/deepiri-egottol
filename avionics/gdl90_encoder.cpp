#include "gdl90_encoder.h"
#include <cstring>
#include <sstream>

namespace deepiri {

class GDL90Encoder::Impl {
public:
    uint32_t icao_address_;
    std::string callsign_;
    uint8_t heartbeat_counter_;

    static constexpr uint8_t MSG_ID_HEARTBEAT = 0x00;
    static constexpr uint8_t MSG_ID_OWNSHIP_REPORT = 0x01;
    static constexpr uint8_t MSG_ID_OWN_POSITION = 0x02;
    static constexpr uint8_t MSG_ID_ENCODED_ES = 0x05;
    static constexpr uint8_t MSG_ID_ATLAS = 0x07;

    Impl() : icao_address_(0), callsign_(""), heartbeat_counter_(0) {}

    uint8_t calculateCRC(const std::vector<uint8_t>& data) {
        uint8_t crc = 0;
        for (uint8_t b : data) {
            crc ^= b;
        }
        return crc;
    }

    std::vector<uint8_t> encodeSignedInt24(int32_t value) {
        std::vector<uint8_t> result(3);
        result[0] = (value >> 16) & 0xFF;
        result[1] = (value >> 8) & 0xFF;
        result[2] = value & 0xFF;
        return result;
    }

    std::vector<uint8_t> encodeUnsignedInt24(uint32_t value) {
        std::vector<uint8_t> result(3);
        result[0] = (value >> 16) & 0xFF;
        result[1] = (value >> 8) & 0xFF;
        result[2] = value & 0xFF;
        return result;
    }

    std::vector<uint8_t> encodeInt15(int16_t value) {
        std::vector<uint8_t> result(2);
        result[0] = (value >> 8) & 0x7F;
        result[1] = value & 0xFF;
        return result;
    }

    double latLonToGDL90(double value) {
        return value * (1 << 14) / 180.0;
    }
};

GDL90Encoder::GDL90Encoder() : pImpl(std::make_unique<Impl>()) {}
GDL90Encoder::~GDL90Encoder() = default;

std::vector<uint8_t> GDL90Encoder::encodeMessage(const GDL90Message& msg) {
    std::vector<uint8_t> encoded;
    encoded.push_back(0x7E);
    encoded.push_back(msg.message_id);
    encoded.insert(encoded.end(), msg.payload.begin(), msg.payload.end());
    encoded.push_back(pImpl->calculateCRC(encoded));
    encoded.push_back(0x7E);
    return encoded;
}

std::vector<uint8_t> GDL90Encoder::encodeOwnshipReport(const OwnshipReport& report) {
    std::vector<uint8_t> payload;

    payload.push_back(Impl::MSG_ID_OWNSHIP_REPORT);

    auto icao_bytes = pImpl->encodeUnsignedInt24(report.icao_address);
    payload.insert(payload.end(), icao_bytes.begin(), icao_bytes.end());

    auto lat_enc = pImpl->encodeSignedInt24(static_cast<int32_t>(report.latitude * (1 << 14) / 90.0));
    payload.insert(payload.end(), lat_enc.begin(), lat_enc.end());

    auto lon_enc = pImpl->encodeSignedInt24(static_cast<int32_t>(report.longitude * (1 << 14) / 90.0));
    payload.insert(payload.end(), lon_enc.begin(), lon_enc.end());

    auto alt_enc = pImpl->encodeUnsignedInt24(static_cast<uint32_t>(report.altitude * 25.0));
    payload.insert(payload.end(), alt_enc.begin(), alt_enc.end());

    uint8_t movement = static_cast<uint8_t>(report.ground_speed / 4);
    payload.push_back(movement);

    uint8_t track = static_cast<uint8_t>(report.track_angle / (360.0 / 128));
    payload.push_back(track);

    int16_t vert_rate = static_cast<int16_t>(report.vertical_rate / 64);
    auto vr_enc = pImpl->encodeInt15(vert_rate);
    payload.insert(payload.end(), vr_enc.begin(), vr_enc.end());

    GDL90Message msg;
    msg.message_id = Impl::MSG_ID_OWNSHIP_REPORT;
    msg.payload = payload;

    return encodeMessage(msg);
}

std::vector<uint8_t> GDL90Encoder::encodeHeartbeat() {
    std::vector<uint8_t> payload;
    payload.push_back(Impl::MSG_ID_HEARTBEAT);
    payload.push_back(0x00);
    payload.push_back(0x00);

    GDL90Message msg;
    msg.message_id = Impl::MSG_ID_HEARTBEAT;
    msg.payload = payload;

    return encodeMessage(msg);
}

void GDL90Encoder::setCallsign(const std::string& callsign) {
    pImpl->callsign_ = callsign;
}

void GDL90Encoder::setICAOAddress(uint32_t icao) {
    pImpl->icao_address_ = icao;
}

bool GDL90Encoder::sendToNetwork(const std::string& host, uint16_t port) {
    return false;
}

}