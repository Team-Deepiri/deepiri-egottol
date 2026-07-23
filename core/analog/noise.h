#pragma once

#include <vector>
#include <random>
#include <cstddef>

namespace deepiri {

constexpr double kBoltzmann = 1.380649e-23;
constexpr double kElectronCharge = 1.602176634e-19;

std::vector<double> thermalNoise(double resistance, double temperature, double bandwidth,
                                  size_t nSamples, std::mt19937_64& rng);
std::vector<double> thermalNoise(double resistance = 1000.0, double temperature = 300.0,
                                  double bandwidth = 1.0, size_t nSamples = 1);

std::vector<double> flickerNoise(size_t nSamples, double dt, double cornerFreq,
                                  double amplitude, std::mt19937_64& rng);
std::vector<double> flickerNoise(size_t nSamples, double dt, double cornerFreq = 1.0,
                                  double amplitude = 1e-6);

std::vector<double> addNoiseToTrace(const std::vector<double>& trace, double thermalR,
                                     double temperature, double bandwidth, double flickerAmp,
                                     double dt, double flickerCorner, std::mt19937_64& rng);
std::vector<double> addNoiseToTrace(const std::vector<double>& trace, double thermalR = 0.0,
                                     double temperature = 300.0, double bandwidth = 1.0,
                                     double flickerAmp = 0.0, double dt = 1e-3,
                                     double flickerCorner = 1.0);

}
