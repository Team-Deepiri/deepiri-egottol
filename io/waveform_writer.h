#pragma once

#include <string>
#include <vector>
#include <memory>

namespace deepiri {

struct WaveformData {
    std::string name;
    std::string unit;
    std::vector<double> time_points;
    std::vector<double> values;
};

enum class WaveformFormat {
    CSV,
    VCD,
    TSV
};

class WaveformWriter {
public:
    WaveformWriter();
    ~WaveformWriter();

    bool write(const std::string& filename, const std::vector<WaveformData>& waveforms);
    bool writeCSV(const std::string& filename, const std::vector<WaveformData>& waveforms);
    bool writeVCD(const std::string& filename, const std::vector<WaveformData>& waveforms);

    void setFormat(WaveformFormat format);
    WaveformFormat getFormat() const;

    std::string toString(const std::vector<WaveformData>& waveforms);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}