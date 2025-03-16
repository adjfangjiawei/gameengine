
#pragma once

#include <error.h>

#include <array>
#include <cmath>
#include <stdexcept>

#include "Math/Vector.h"

namespace Engine {
    namespace Math {

        /**
         * @brief Template class for MxN matrices
         * @tparam T The type of matrix elements
         * @tparam M Number of rows
         * @tparam N Number of columns
         */
        template <typename T, size_t M, size_t N>
        class Matrix {
            static_assert(std::is_arithmetic<T>::value,
                          "Matrix type must be arithmetic");
            static_assert(M > 0 && N > 0,
                          "Matrix dimensions must be greater than 0");

          public:
            /**
             * @brief Default constructor, initializes to identity if square,
             * zero otherwise
             */
            Matrix() {
                if constexpr (M == N) {
                    // Initialize as identity matrix
                    for (size_t i = 0; i < M; ++i) {
                        for (size_t j = 0; j < N; ++j) {
                            m_data[i][j] = (i == j) ? T(1) : T(0);
                        }
                    }
                } else {
                    // Initialize as zero matrix
                    for (size_t i = 0; i < M; ++i) {
                        for (size_t j = 0; j < N; ++j) {
                            m_data[i][j] = T(0);
                        }
                    }
                }
            }

            /**
             * @brief Constructor from array
             * @param data 2D array of elements
             */
            explicit Matrix(const std::array<std::array<T, N>, M> &data)
                : m_data(data) {}

            // Element access
            T &operator()(size_t row, size_t col) { return m_data[row][col]; }
            const T &operator()(size_t row, size_t col) const {
                return m_data[row][col];
            }

            // Matrix operations
            Matrix operator+(const Matrix &other) const {
                Matrix result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < N; ++j) {
                        result(i, j) = m_data[i][j] + other(i, j);
                    }
                }
                return result;
            }

            Matrix operator-(const Matrix &other) const {
                Matrix result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < N; ++j) {
                        result(i, j) = m_data[i][j] - other(i, j);
                    }
                }
                return result;
            }

            Matrix operator*(T scalar) const {
                Matrix result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < N; ++j) {
                        result(i, j) = m_data[i][j] * scalar;
                    }
                }
                return result;
            }

            // Matrix multiplication
            template <size_t P>
            Matrix<T, M, P> operator*(const Matrix<T, N, P> &other) const {
                Matrix<T, M, P> result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < P; ++j) {
                        T sum = T(0);
                        for (size_t k = 0; k < N; ++k) {
                            sum += m_data[i][k] * other(k, j);
                        }
                        result(i, j) = sum;
                    }
                }
                return result;
            }

            // Vector multiplication
            Vector<T, M> operator*(const Vector<T, N> &vec) const {
                Vector<T, M> result;
                for (size_t i = 0; i < M; ++i) {
                    T sum = T(0);
                    for (size_t j = 0; j < N; ++j) {
                        sum += m_data[i][j] * vec[j];
                    }
                    result[i] = sum;
                }
                return result;
            }

            // Transpose
            Matrix<T, N, M> Transpose() const {
                Matrix<T, N, M> result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < N; ++j) {
                        result(j, i) = m_data[i][j];
                    }
                }
                return result;
            }

            // Determinant (only for square matrices)
            template <typename U = T>
            typename std::enable_if<M == N, U>::type Determinant() const {
                if constexpr (M == 1) {
                    return m_data[0][0];
                } else if constexpr (M == 2) {
                    return m_data[0][0] * m_data[1][1] -
                           m_data[0][1] * m_data[1][0];
                } else if constexpr (M == 3) {
                    return m_data[0][0] * (m_data[1][1] * m_data[2][2] -
                                           m_data[1][2] * m_data[2][1]) -
                           m_data[0][1] * (m_data[1][0] * m_data[2][2] -
                                           m_data[1][2] * m_data[2][0]) +
                           m_data[0][2] * (m_data[1][0] * m_data[2][1] -
                                           m_data[1][1] * m_data[2][0]);
                } else {
                    // For larger matrices, use cofactor expansion along first
                    // row
                    T det = T(0);
                    for (size_t j = 0; j < N; ++j) {
                        det += m_data[0][j] * Cofactor(0, j);
                    }
                    return det;
                }
            }

            // Inverse (only for square matrices)
            template <typename U = T>
            typename std::enable_if<M == N, Matrix>::type Inverse() const {
                T det = Determinant();
                if (std::abs(det) < std::numeric_limits<T>::epsilon()) {
                    throw std::runtime_error("Matrix is not invertible");
                }

                Matrix result;
                for (size_t i = 0; i < M; ++i) {
                    for (size_t j = 0; j < N; ++j) {
                        result(j, i) = Cofactor(i, j) / det;
                    }
                }
                return result;
            }

          private:
            // Helper function for determinant calculation
            T Cofactor(size_t row, size_t col) const {
                Matrix<T, M - 1, N - 1> minor;
                size_t r = 0;
                for (size_t i = 0; i < M; ++i) {
                    if (i == row) continue;
                    size_t c = 0;
                    for (size_t j = 0; j < N; ++j) {
                        if (j == col) continue;
                        minor(r, c) = m_data[i][j];
                        ++c;
                    }
                    ++r;
                }
                return ((row + col) % 2 == 0 ? 1 : -1) * minor.Determinant();
            }

          private:
            std::array<std::array<T, N>, M> m_data;
        };

        // Common matrix type aliases
        using Matrix2f = Matrix<float, 2, 2>;
        using Matrix3f = Matrix<float, 3, 3>;
        using Matrix4f = Matrix<float, 4, 4>;
        using Matrix2d = Matrix<double, 2, 2>;
        using Matrix3d = Matrix<double, 3, 3>;
        using Matrix4d = Matrix<double, 4, 4>;

        // Scalar multiplication from the left
        template <typename T, size_t M, size_t N>
        Matrix<T, M, N> operator*(T scalar, const Matrix<T, M, N> &matrix) {
            return matrix * scalar;
        }

    }  // namespace Math
}  // namespace Engine
