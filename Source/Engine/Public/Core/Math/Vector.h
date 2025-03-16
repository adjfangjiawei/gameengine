
#pragma once

#include <array>
#include <cmath>
#include <type_traits>

#include "Core/CoreTypes.h"

namespace Engine {
    namespace Math {

        /**
         * @brief Template class for N-dimensional vectors
         * @tparam T The type of vector components
         * @tparam N The number of dimensions
         */
        template <typename T, size_t N>
        class Vector {
            static_assert(std::is_arithmetic<T>::value,
                          "Vector type must be arithmetic");
            static_assert(N > 0, "Vector dimension must be greater than 0");

          public:
            /**
             * @brief Default constructor, initializes all components to zero
             */
            Vector() : m_data{} {}

            /**
             * @brief Constructor from array
             * @param data Array of components
             */
            explicit Vector(const std::array<T, N> &data) : m_data(data) {}

            /**
             * @brief Constructor from initializer list
             * @param list List of components
             */
            Vector(std::initializer_list<T> list) {
                size_t i = 0;
                for (const T &value : list) {
                    if (i >= N) break;
                    m_data[i++] = value;
                }
            }

            // Element access
            T &operator[](size_t index) { return m_data[index]; }
            const T &operator[](size_t index) const { return m_data[index]; }

            // Vector operations
            Vector operator+(const Vector &other) const {
                Vector result;
                for (size_t i = 0; i < N; ++i) {
                    result[i] = m_data[i] + other[i];
                }
                return result;
            }

            Vector operator-(const Vector &other) const {
                Vector result;
                for (size_t i = 0; i < N; ++i) {
                    result[i] = m_data[i] - other[i];
                }
                return result;
            }

            Vector operator*(T scalar) const {
                Vector result;
                for (size_t i = 0; i < N; ++i) {
                    result[i] = m_data[i] * scalar;
                }
                return result;
            }

            Vector operator/(T scalar) const {
                Vector result;
                for (size_t i = 0; i < N; ++i) {
                    result[i] = m_data[i] / scalar;
                }
                return result;
            }

            // Compound assignment operators
            Vector &operator+=(const Vector &other) {
                for (size_t i = 0; i < N; ++i) {
                    m_data[i] += other[i];
                }
                return *this;
            }

            Vector &operator-=(const Vector &other) {
                for (size_t i = 0; i < N; ++i) {
                    m_data[i] -= other[i];
                }
                return *this;
            }

            Vector &operator*=(T scalar) {
                for (size_t i = 0; i < N; ++i) {
                    m_data[i] *= scalar;
                }
                return *this;
            }

            Vector &operator/=(T scalar) {
                for (size_t i = 0; i < N; ++i) {
                    m_data[i] /= scalar;
                }
                return *this;
            }

            // Vector operations
            T Dot(const Vector &other) const {
                T result = T(0);
                for (size_t i = 0; i < N; ++i) {
                    result += m_data[i] * other[i];
                }
                return result;
            }

            T LengthSquared() const { return Dot(*this); }

            T Length() const { return std::sqrt(LengthSquared()); }

            Vector Normalized() const {
                T length = Length();
                if (length > T(0)) {
                    return *this / length;
                }
                return *this;
            }

            void Normalize() { *this = Normalized(); }

            // Static utility functions
            static T Distance(const Vector &a, const Vector &b) {
                return (b - a).Length();
            }

            static T DistanceSquared(const Vector &a, const Vector &b) {
                return (b - a).LengthSquared();
            }

            static Vector Lerp(const Vector &a, const Vector &b, T t) {
                return a + (b - a) * t;
            }

          private:
            std::array<T, N> m_data;
        };

        // Common vector type aliases
        using Vector2f = Vector<float, 2>;
        using Vector3f = Vector<float, 3>;
        using Vector4f = Vector<float, 4>;
        using Vector2d = Vector<double, 2>;
        using Vector3d = Vector<double, 3>;
        using Vector4d = Vector<double, 4>;
        using Vector2i = Vector<int32_t, 2>;
        using Vector3i = Vector<int32_t, 3>;
        using Vector4i = Vector<int32_t, 4>;

        // Specialized 3D vector operations
        template <typename T>
        Vector<T, 3> Cross(const Vector<T, 3> &a, const Vector<T, 3> &b) {
            return Vector<T, 3>{a[1] * b[2] - a[2] * b[1],
                                a[2] * b[0] - a[0] * b[2],
                                a[0] * b[1] - a[1] * b[0]};
        }

        // Scalar multiplication from the left
        template <typename T, size_t N>
        Vector<T, N> operator*(T scalar, const Vector<T, N> &vector) {
            return vector * scalar;
        }

    }  // namespace Math
}  // namespace Engine
