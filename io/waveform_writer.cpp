#include "waveform_writer.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace deepiri {

class WaveformWriter::Impl {
public:
  WaveformFormat format_;

  Impl() : format_(WaveformFormat::CSV) {}

  std::string escapeVCDName(const std::string &name) {
    if (name.empty())
      return "[]";
    if (name[0] == '[')
      return name;
    return "[" + name + "]";
  }
};

WaveformWriter::WaveformWriter() : pImpl(std::make_unique<Impl>()) {}
WaveformWriter::~WaveformWriter() = default;

bool WaveformWriter::write(const std::string &filename,
                           const std::vector<WaveformData> &waveforms) {
  switch (pImpl->format_) {
  case WaveformFormat::CSV:
    return writeCSV(filename, waveforms);
  case WaveformFormat::VCD:
    return writeVCD(filename, waveforms);
  default:
    return writeCSV(filename, waveforms);
  }
}

bool WaveformWriter::writeCSV(const std::string &filename,
                              const std::vector<WaveformData> &waveforms) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  file << "time";
  for (const auto &wd : waveforms) {
    file << "," << wd.name;
  }
  file << "\n";

  size_t max_size = 0;
  for (const auto &wd : waveforms) {
    if (wd.time_points.size() > max_size) {
      max_size = wd.time_points.size();
    }
  }

  for (size_t i = 0; i < max_size; ++i) {
    if (i < waveforms[0].time_points.size()) {
      file << std::scientific << std::setprecision(6)
           << waveforms[0].time_points[i];
    } else {
      file << "0";
    }

    for (const auto &wd : waveforms) {
      if (i < wd.values.size()) {
        file << "," << std::scientific << std::setprecision(6) << wd.values[i];
      } else {
        file << ",0";
      }
    }
    file << "\n";
  }

  return true;
}

bool WaveformWriter::writeVCD(const std::string &filename,
                              const std::vector<WaveformData> &waveforms) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  file << "$timescale 1ns $end\n";
  file << "$scope module logic $end\n";

  char var_id = '!';
  for (size_t i = 0; i < waveforms.size(); ++i) {
    std::string var_name = pImpl->escapeVCDName(waveforms[i].name);
    file << "$var wire 1 " << var_id << var_name << " $end\n";
    var_id++;
  }

  file << "$upscope $end\n";
  file << "$enddefinitions $end\n";

  file << "$dumpvars\n";
  for (const auto &wd : waveforms) {
    bool val = (wd.values.size() > 0 && wd.values[0] != 0.0);
    file << (val ? "1" : "0") << "$end\n";
  }
  file << "$end\n";

  size_t max_size = 0;
  for (const auto &wd : waveforms) {
    if (wd.time_points.size() > max_size) {
      max_size = wd.time_points.size();
    }
  }

  char current_id = '!';
  for (size_t i = 0; i < max_size; ++i) {
    double current_time = (i < waveforms[0].time_points.size())
                              ? waveforms[0].time_points[i]
                              : 0.0;
    file << "#" << static_cast<long>(current_time * 1e9) << "\n";

    for (size_t j = 0; j < waveforms.size(); ++j) {
      if (i < waveforms[j].values.size()) {
        bool val = (waveforms[j].values[i] != 0.0);
        file << (val ? "1" : "0") << char('!' + j) << "\n";
      }
    }
  }

  return true;
}

void WaveformWriter::setFormat(WaveformFormat format) {
  pImpl->format_ = format;
}

WaveformFormat WaveformWriter::getFormat() const { return pImpl->format_; }

std::string
WaveformWriter::toString(const std::vector<WaveformData> &waveforms) {
  std::ostringstream oss;
  oss << "time";
  for (const auto &wd : waveforms) {
    oss << "," << wd.name;
  }
  oss << "\n";
  return oss.str();
}

} // namespace deepiri