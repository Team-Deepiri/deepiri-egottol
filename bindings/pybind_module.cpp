#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "matrix.h"

namespace py = pybind11;

namespace {

py::array_t<double> solve_linear(py::array_t<double> a_arr, py::array_t<double> b_arr) {
    auto a_buf = a_arr.request();
    auto b_buf = b_arr.request();

    if (a_buf.ndim != 2 || a_buf.shape[0] != a_buf.shape[1]) {
        throw std::invalid_argument("A must be a square 2-D array");
    }
    if (b_buf.ndim != 1) {
        throw std::invalid_argument("b must be a 1-D array");
    }

    const py::ssize_t n = a_buf.shape[0];
    if (b_buf.shape[0] != n) {
        throw std::invalid_argument("b length must match A dimension");
    }

    deepiri::Matrix matrix(static_cast<size_t>(n), static_cast<size_t>(n));
    const auto* a_ptr = static_cast<const double*>(a_buf.ptr);
    for (py::ssize_t i = 0; i < n; ++i) {
        for (py::ssize_t j = 0; j < n; ++j) {
            matrix.at(static_cast<size_t>(i), static_cast<size_t>(j)) =
                a_ptr[i * n + j];
        }
    }

    std::vector<double> rhs(static_cast<size_t>(n));
    const auto* b_ptr = static_cast<const double*>(b_buf.ptr);
    for (py::ssize_t i = 0; i < n; ++i) {
        rhs[static_cast<size_t>(i)] = b_ptr[i];
    }

    std::vector<double> solution;
    try {
        solution = matrix.solveGaussian(rhs);
    } catch (const std::exception& exc) {
        throw std::runtime_error(
            std::string("egottol._native: solve failed: ") + exc.what());
    }

    py::array_t<double> out(n);
    auto out_buf = out.request();
    auto* out_ptr = static_cast<double*>(out_buf.ptr);
    for (py::ssize_t i = 0; i < n; ++i) {
        out_ptr[i] = solution[static_cast<size_t>(i)];
    }
    return out;
}

}  // namespace

PYBIND11_MODULE(_native, m) {
    m.doc() = "Egottol native C++ core (MNA linear solve via deepiri::Matrix)";

    m.def("core_version", []() { return std::string("1.0.0"); });
    m.def("available", []() { return true; });
    m.def(
        "solve_linear",
        &solve_linear,
        py::arg("a"),
        py::arg("b"),
        "Solve Ax=b using the C++ Gaussian elimination solver."
    );
}
