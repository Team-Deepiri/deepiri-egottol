#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class Device;

struct TransferFunctionResult {
    double gain = 0.0;           // Vout/Vin or Iout/Iin depending on sources
    double inputZ = 0.0;         // Ohms
    double outputZ = 0.0;        // Ohms
    bool success = false;
    std::string message;
};

struct NoiseResult {
    std::vector<double> frequenciesHz;
    std::vector<double> outputNoiseDensity;  // V/√Hz at output
    double totalRms = 0.0;                  // integrated if band given
    bool success = false;
    std::string message;
};

// .tf V(out) Vin  — small-signal DC transfer + Zin/Zout at the linearized OP.
TransferFunctionResult computeTransferFunction(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t outputNode,          // 1-based node id (0 = ground)
    const std::string& inputSourceName
);

// .noise V(out) Vin dec np fstart fstop — resistor thermal noise referred to out.
NoiseResult computeOutputNoise(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t outputNode,
    double freqStartHz,
    double freqEndHz,
    int numPoints,
    double temperatureK = 300.0
);

}
