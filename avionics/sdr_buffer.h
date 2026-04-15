#pragma once

#include <vector>
#include <complex>
#include <memory>
#include <optional>
#include <complex>
#include <cstdint>
#include <memory>

namespace deepiri {

enum class SDRFormat {
    CF32,
    CS16,
    CU8,
    CSI16
};

struct SDRMetadata {
    double center_frequency;
    double sample_rate;
    int64_t timestamp;
    uint32_t sequence_number;
};

class SDRBuffer {
public:
    SDRBuffer();
    ~SDRBuffer();

    void setFormat(SDRFormat format);
    void setCapacity(size_t max_samples);

    bool write(const std::vector<std::complex<float>>& samples);
    bool read(size_t count, std::vector<std::complex<float>>& output);

    size_t available() const;
    size_t capacity() const;
    void clear();

    void setMetadata(const SDRMetadata& meta);
    SDRMetadata getMetadata() const;

    SDRFormat getFormat() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}