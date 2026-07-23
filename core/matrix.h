#ifndef DEEPIRI_MATRIX_H
#define DEEPIRI_MATRIX_H

#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace deepiri {

class Matrix {
public:
    Matrix() : rows_(0), cols_(0) {}
    Matrix(size_t rows, size_t cols, double fill = 0.0);
    Matrix(const std::vector<std::vector<double>>& data);

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    double& at(size_t i, size_t j) {
        if (i >= rows_ || j >= cols_) {
            throw std::out_of_range("Matrix::at index out of range");
        }
        return data_[i * cols_ + j];
    }
    const double& at(size_t i, size_t j) const {
        if (i >= rows_ || j >= cols_) {
            throw std::out_of_range("Matrix::at index out of range");
        }
        return data_[i * cols_ + j];
    }

    Matrix transpose() const;
    Matrix operator+(const Matrix& other) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator*(const Matrix& other) const;
    Matrix operator*(double scalar) const;
    Matrix operator/(double scalar) const;
    Matrix& operator+=(const Matrix& other);
    Matrix& operator-=(const Matrix& other);
    Matrix& operator*=(double scalar);
    Matrix& operator/=(double scalar);

    std::vector<double> operator*(const std::vector<double>& vec) const;
    std::vector<double> solveLU(const std::vector<double>& b) const;
    std::vector<double> solveGaussian(const std::vector<double>& b) const;

    Matrix luDecompose() const;
    std::vector<double> luSolve(const Matrix& LU, const std::vector<double>& b) const;
    Matrix invert() const;
    double determinant() const;
    Matrix inverse() const;
    void fill(double value);
    void setRow(size_t row, const std::vector<double>& values);
    void setCol(size_t col, const std::vector<double>& values);
    std::vector<double> getRow(size_t row) const;
    std::vector<double> getCol(size_t col) const;
    Matrix submatrix(size_t rowStart, size_t colStart, size_t rowCount, size_t colCount) const;
    std::vector<double> diagonal() const;

    void print() const;

private:
    size_t rows_, cols_;
    std::vector<double> data_;
    void luDecomposeImpl(Matrix& L, Matrix& U, std::vector<size_t>& pivots) const;
};

}

#endif