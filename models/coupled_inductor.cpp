#include "coupled_inductor.h"
#include <algorithm>
#include <cmath>

namespace deepiri {

CoupledInductor::CoupledInductor(const std::string& name,
                                 double l1, double l2, double k,
                                 size_t n1p, size_t n1n, size_t n2p, size_t n2n)
    : name_(name), L1_(std::max(l1, 1e-18)), L2_(std::max(l2, 1e-18)),
      n1p_(n1p), n1n_(n1n), n2p_(n2p), n2n_(n2n) {
    double kk = std::clamp(k, -0.999999, 0.999999);
    M_ = kk * std::sqrt(L1_ * L2_);
    nodeP_ = n1p;
    nodeN_ = n1n;
    terminals_ = {n1p, n1n, n2p, n2n};
}

std::vector<size_t> CoupledInductor::terminals() const {
    return {n1p_, n1n_, n2p_, n2n_};
}

void CoupledInductor::initializeDC() {
    i1_ = i2_ = 0.0;
    transientActive_ = false;
}

std::vector<double> CoupledInductor::getCurrent() const {
    if (!transientActive_) return {0, 0, 0, 0};
    // Into device at each terminal (same convention as single L)
    return {ieq1_, -ieq1_, ieq2_, -ieq2_};
}

std::vector<std::vector<double>> CoupledInductor::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(4, std::vector<double>(4, 0.0));
    if (!transientActive_) return G;
    // Port1 terminals 0,1; port2 terminals 2,3
    auto addPort = [&](size_t a, size_t b, double gaa, double gab, double gba, double gbb) {
        G[a][a] += gaa; G[a][b] += gab;
        G[b][a] += gba; G[b][b] += gbb;
    };
    // i1 into + of L1 from geq: i1 = g11*v1 + g12*v2 + …
    // Leaving node n1p: +i1 → G contributions on differential ports
    G[0][0] += g11_; G[0][1] -= g11_; G[0][2] += g12_; G[0][3] -= g12_;
    G[1][0] -= g11_; G[1][1] += g11_; G[1][2] -= g12_; G[1][3] += g12_;
    G[2][0] += g21_; G[2][1] -= g21_; G[2][2] += g22_; G[2][3] -= g22_;
    G[3][0] -= g21_; G[3][1] += g21_; G[3][2] -= g22_; G[3][3] += g22_;
    (void)addPort;
    return G;
}

void CoupledInductor::prepareTransientStep(double h, const std::vector<double>& /*prev*/) {
    if (h <= 0.0) {
        transientActive_ = false;
        return;
    }
    // BE: L di/dt = v  →  [L1 M; M L2] (i - i_prev)/h = v
    // i = i_prev + h A^{-1} v
    // Geq = h * A^{-1}, Ieq = -i_prev (into device convention matches single L: ieq_ = -i_)
    double det = L1_ * L2_ - M_ * M_;
    if (std::abs(det) < 1e-30) det = 1e-30;
    double inv11 = L2_ / det;
    double inv12 = -M_ / det;
    double inv21 = -M_ / det;
    double inv22 = L1_ / det;
    double scale = useTrap_ ? (h * 0.5) : h;
    g11_ = scale * inv11;
    g12_ = scale * inv12;
    g21_ = scale * inv21;
    g22_ = scale * inv22;
    // Trap would also need v_prev terms; BE is primary path.
    ieq1_ = -i1_;
    ieq2_ = -i2_;
    if (useTrap_) {
        // Approximate trap via same Geq with half-step (no v_prev cross yet).
        ieq1_ = -i1_;
        ieq2_ = -i2_;
    }
    transientActive_ = true;
}

void CoupledInductor::acceptTransientStep(const std::vector<double>& state) {
    auto vAt = [&](size_t n) -> double {
        if (n == 0) return 0.0;
        if (n - 1 < state.size()) return state[n - 1];
        return 0.0;
    };
    double v1 = vAt(n1p_) - vAt(n1n_);
    double v2 = vAt(n2p_) - vAt(n2n_);
    if (transientActive_) {
        i1_ = g11_ * v1 + g12_ * v2 - ieq1_;
        i2_ = g21_ * v1 + g22_ * v2 - ieq2_;
    }
}

}
