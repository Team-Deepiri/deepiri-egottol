#include "matrix.h"
#include <iostream>
#include <iomanip>

namespace deepiri {

Matrix::Matrix(size_t rows, size_t cols, double fill)
    : rows_(rows), cols_(cols), data_(rows * cols, fill) {}

Matrix::Matrix(const std::vector<std::vector<double>>& data)
    : rows_(data.size()), cols_(data.empty() ? 0 : data[0].size()) {
    for (const auto& row : data) {
        if (row.size() != cols_) {
            throw std::invalid_argument("Inconsistent row sizes in Matrix initialization");
        }
        data_.insert(data_.end(), row.begin(), row.end());
    }
}

Matrix Matrix::transpose() const {
    Matrix result(cols_, rows_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result.at(j, i) = at(i, j);
        }
    }
    return result;
}

Matrix Matrix::operator+(const Matrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for addition");
    }
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

Matrix Matrix::operator-(const Matrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for subtraction");
    }
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
    if (cols_ != other.rows_) {
        throw std::invalid_argument("Matrix dimensions incompatible for multiplication");
    }
    Matrix result(rows_, other.cols_, 0.0);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < other.cols_; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < cols_; ++k) {
                sum += at(i, k) * other.at(k, j);
            }
            result.at(i, j) = sum;
        }
    }
    return result;
}

Matrix Matrix::operator*(double scalar) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

Matrix Matrix::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Division by zero");
    }
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] / scalar;
    }
    return result;
}

Matrix& Matrix::operator+=(const Matrix& other) {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for addition");
    }
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] += other.data_[i];
    }
    return *this;
}

Matrix& Matrix::operator-=(const Matrix& other) {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for subtraction");
    }
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] -= other.data_[i];
    }
    return *this;
}

Matrix& Matrix::operator*=(double scalar) {
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] *= scalar;
    }
    return *this;
}

Matrix& Matrix::operator/=(double scalar) {
    if (scalar == 0.0) {
        throw std::invalid_argument("Division by zero");
    }
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] /= scalar;
    }
    return *this;
}

std::vector<double> Matrix::operator*(const std::vector<double>& vec) const {
    if (cols_ != vec.size()) {
        throw std::invalid_argument("Matrix column count must match vector size");
    }
    std::vector<double> result(rows_, 0.0);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result[i] += at(i, j) * vec[j];
        }
    }
    return result;
}

void Matrix::fill(double value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Matrix::setRow(size_t row, const std::vector<double>& values) {
    if (row >= rows_ || values.size() != cols_) {
        throw std::invalid_argument("Invalid row or values size");
    }
    for (size_t j = 0; j < cols_; ++j) {
        at(row, j) = values[j];
    }
}

void Matrix::setCol(size_t col, const std::vector<double>& values) {
    if (col >= cols_ || values.size() != rows_) {
        throw std::invalid_argument("Invalid column or values size");
    }
    for (size_t i = 0; i < rows_; ++i) {
        at(i, col) = values[i];
    }
}

std::vector<double> Matrix::getRow(size_t row) const {
    if (row >= rows_) {
        throw std::invalid_argument("Invalid row index");
    }
    std::vector<double> result(cols_);
    for (size_t j = 0; j < cols_; ++j) {
        result[j] = at(row, j);
    }
    return result;
}

std::vector<double> Matrix::getCol(size_t col) const {
    if (col >= cols_) {
        throw std::invalid_argument("Invalid column index");
    }
    std::vector<double> result(rows_);
    for (size_t i = 0; i < rows_; ++i) {
        result[i] = at(i, col);
    }
    return result;
}

Matrix Matrix::submatrix(size_t rowStart, size_t colStart, size_t rowCount, size_t colCount) const {
    if (rowStart + rowCount > rows_ || colStart + colCount > cols_) {
        throw std::invalid_argument("Submatrix dimensions out of bounds");
    }
    Matrix result(rowCount, colCount);
    for (size_t i = 0; i < rowCount; ++i) {
        for (size_t j = 0; j < colCount; ++j) {
            result.at(i, j) = at(rowStart + i, colStart + j);
        }
    }
    return result;
}

std::vector<double> Matrix::diagonal() const {
    size_t minDim = std::min(rows_, cols_);
    std::vector<double> result(minDim);
    for (size_t i = 0; i < minDim; ++i) {
        result[i] = at(i, i);
    }
    return result;
}

std::vector<double> Matrix::solveGaussian(const std::vector<double>& b) const {
    if (rows_ != cols_) {
        throw std::invalid_argument("Matrix must be square for Gaussian elimination");
    }
    if (b.size() != rows_) {
        throw std::invalid_argument("Vector size must match matrix dimensions");
    }

    size_t n = rows_;
    std::vector<double> a(data_);
    std::vector<double> bb(b);
    std::vector<size_t> pivotIndices(n);

    for (size_t i = 0; i < n; ++i) {
        size_t maxRow = i;
        double maxVal = std::abs(a[i * n + i]);
        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(a[k * n + i]) > maxVal) {
                maxRow = k;
                maxVal = std::abs(a[k * n + i]);
            }
        }

        if (maxVal < 1e-12) {
            throw std::runtime_error("Matrix is singular or nearly singular");
        }

        if (maxRow != i) {
            std::swap(a[i * n + i], a[maxRow * n + i]);
            for (size_t j = i + 1; j < n; ++j) {
                std::swap(a[i * n + j], a[maxRow * n + j]);
            }
            std::swap(bb[i], bb[maxRow]);
        }

        pivotIndices[i] = maxRow;

        for (size_t k = i + 1; k < n; ++k) {
            double factor = a[k * n + i] / a[i * n + i];
            bb[k] -= factor * bb[i];
            for (size_t j = i; j < n; ++j) {
                a[k * n + j] -= factor * a[i * n + j];
            }
        }
    }

    std::vector<double> x(n, 0.0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        const size_t row = static_cast<size_t>(i);
        double sum = bb[row];
        for (size_t j = row + 1; j < n; ++j) {
            sum -= a[row * n + j] * x[j];
        }
        x[row] = sum / a[row * n + row];
    }

    return x;
}

Matrix Matrix::luDecompose() const {
    if (rows_ != cols_) {
        throw std::invalid_argument("Matrix must be square for LU decomposition");
    }

    size_t n = rows_;
    Matrix LU(*this);

    for (size_t i = 0; i < n; ++i) {
        for (size_t k = i; k < n; ++k) {
            double sum = 0.0;
            for (size_t j = 0; j < i; ++j) {
                sum += LU.at(i, j) * LU.at(j, k);
            }
            LU.at(i, k) = at(i, k) - sum;
        }

        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(LU.at(i, i)) < 1e-12) {
                throw std::runtime_error("Zero pivot in LU decomposition");
            }
            double sum = 0.0;
            for (size_t j = 0; j < i; ++j) {
                sum += LU.at(k, j) * LU.at(j, i);
            }
            LU.at(k, i) = (at(k, i) - sum) / LU.at(i, i);
        }
    }

    return LU;
}

std::vector<double> Matrix::solveLU(const std::vector<double>& b) const {
    Matrix LU = luDecompose();
    return luSolve(LU, b);
}

std::vector<double> Matrix::luSolve(const Matrix& LU, const std::vector<double>& b) const {
    size_t n = rows_;
    if (n != b.size()) {
        throw std::invalid_argument("Vector size must match matrix dimensions");
    }

    std::vector<double> y(n), x(n);
    for (size_t i = 0; i < n; ++i) {
        y[i] = b[i];
        for (size_t j = 0; j < i; ++j) {
            y[i] -= LU.at(i, j) * y[j];
        }
    }

    for (int i = n - 1; i >= 0; --i) {
        x[i] = y[i];
        for (size_t j = i + 1; j < n; ++j) {
            x[i] -= LU.at(i, j) * x[j];
        }
        if (std::abs(LU.at(i, i)) < 1e-12) {
            throw std::runtime_error("Zero diagonal in LU matrix");
        }
        x[i] /= LU.at(i, i);
    }

    return x;
}

double Matrix::determinant() const {
    if (rows_ != cols_) {
        throw std::invalid_argument("Matrix must be square for determinant");
    }

    Matrix LU = luDecompose();
    double det = 1.0;
    for (size_t i = 0; i < rows_; ++i) {
        det *= LU.at(i, i);
    }
    return det;
}

Matrix Matrix::invert() const {
    return inverse();
}

Matrix Matrix::inverse() const {
    if (rows_ != cols_) {
        throw std::invalid_argument("Matrix must be square for inversion");
    }

    size_t n = rows_;
    Matrix result(n, n);

    for (size_t j = 0; j < n; ++j) {
        std::vector<double> e(n, 0.0);
        e[j] = 1.0;
        std::vector<double> x = solveLU(e);
        for (size_t i = 0; i < n; ++i) {
            result.at(i, j) = x[i];
        }
    }

    return result;
}

void Matrix::print() const {
    std::cout << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            std::cout << std::setw(12) << at(i, j);
        }
        std::cout << std::endl;
    }
}

}