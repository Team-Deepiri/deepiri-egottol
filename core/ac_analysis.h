#pragma once

#include <complex>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class Device;

struct ACResult {
    std::vector<double> frequenciesHz;
    // magnitude[nodeIndex][freqIndex] / phaseDeg[nodeIndex][freqIndex],
    // where nodeIndex 0 corresponds to node id 1 in nodeMap (ground, id 0,
    // is not part of the reduced system — same convention as MNASolver).
    std::vector<std::vector<double>> magnitude;
    std::vector<std::vector<double>> phaseDeg;
    double referenceMagnitude = 1.0;
    bool success = false;
    std::string message;
};

// Small-signal AC/Bode analysis: linearizes at a DC operating point (via
// MNASolver, the same real-valued MNA used for DC) and sweeps frequency,
// solving a complex MNA system at each point for |H(jw)| and phase per node.
// Mirrors egottol/engines/analog/ac_analysis.py's ACAnalysisEngine.
class ACAnalysis {
public:
    ACResult sweep(
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap,
        double freqStartHz,
        double freqEndHz,
        int numPoints
    );

private:
    static std::vector<double> frequencySweep(double freqStartHz, double freqEndHz, int numPoints);

    static void stampAdmittance(
        std::vector<std::complex<double>>& y,
        size_t width,
        size_t numNodes,
        size_t np,
        size_t nn,
        std::complex<double> admittance
    );

    static std::vector<std::complex<double>> solveComplexGaussian(
        std::vector<std::complex<double>> a,
        std::vector<std::complex<double>> b,
        size_t n
    );
};

}
