#pragma once

#include <complex>
#include <vector>
#include <stdexcept>
#include <cmath>

namespace deepiri {

inline bool isPowerOfTwo(size_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

inline size_t nextPowerOfTwo(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// Iterative radix-2 Cooley-Tukey, in-place bit-reversal permutation followed
// by butterfly passes. n must be a power of two.
inline std::vector<std::complex<double>> fft(std::vector<std::complex<double>> a) {
    const size_t n = a.size();
    if (n <= 1) return a;
    if (!isPowerOfTwo(n)) {
        throw std::invalid_argument("fft: size must be a power of two");
    }

    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2.0 * M_PI / static_cast<double>(len);
        const std::complex<double> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k) {
                std::complex<double> u = a[i + k];
                std::complex<double> v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    return a;
}

// ifft via conjugation trick: ifft(x) = conj(fft(conj(x))) / n, avoiding a
// second butterfly implementation with a flipped twiddle sign.
inline std::vector<std::complex<double>> ifft(std::vector<std::complex<double>> a) {
    const size_t n = a.size();
    for (auto& x : a) x = std::conj(x);
    std::vector<std::complex<double>> y = fft(std::move(a));
    for (auto& x : y) x = std::conj(x) / static_cast<double>(n);
    return y;
}

}  // namespace deepiri
