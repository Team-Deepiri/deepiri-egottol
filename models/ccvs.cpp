#include "ccvs.h"

namespace deepiri {

CCVS::CCVS(const std::string& name, double gain)
    : name_(name), gain_(gain) {}

std::vector<std::vector<double>> CCVS::getConductanceMatrix() const {
    return std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0));
}

}
