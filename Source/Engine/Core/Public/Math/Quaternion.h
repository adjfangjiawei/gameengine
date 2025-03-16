
#pragma once

#include <cmath>

#include "Math/Matrix.h"
#include "Math/Vector.h"

namespace Engine {
    namespace Math {

        /**
         * @brief Quaternion class for representing rotations
         * @tparam T The underlying type (usually float or double)
         */
        template <typename T>
        class Quaternion {
            static_assert(std::is_floating_point<T>::value,
                          "Quaternion type must be floating point");

          public:
            /**
             * @brief Default constructor, creates identity quaternion
             */
            Quaternion() : m_x(0), m_y(0), m_z(0), m_w(1) {}

            /**
             * @brief Constructor from components
             */
            Quaternion(T x, T y, T z, T w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

            /**
             * @brief Constructor from axis and angle
             * @param axis Rotation axis (should be normalized)
             * @param angle Rotation angle in radians
             */
            Quaternion(const Vector<T, 3> &axis, T angle) {
                T halfAngle = angle * T(0.5);
                T sinHalfAngle = std::sin(halfAngle);

                m_x = axis[0] * sinHalfAngle;
                m_y = axis[1] * sinHalfAngle;
                m_z = axis[2] * sinHalfAngle;
                m_w = std::cos(halfAngle);
            }

            /**
             * @brief Constructor from Euler angles (in radians)
             * @param pitch Rotation around X axis
             * @param yaw Rotation around Y axis
             * @param roll Rotation around Z axis
             */
            static Quaternion FromEulerAngles(T pitch, T yaw, T roll) {
                T cy = std::cos(yaw * T(0.5));
                T sy = std::sin(yaw * T(0.5));
                T cp = std::cos(pitch * T(0.5));
                T sp = std::sin(pitch * T(0.5));
                T cr = std::cos(roll * T(0.5));
                T sr = std::sin(roll * T(0.5));

                return Quaternion(sr * cp * cy - cr * sp * sy,
                                  cr * sp * cy + sr * cp * sy,
                                  cr * cp * sy - sr * sp * cy,
                                  cr * cp * cy + sr * sp * sy);
            }

            // Getters
            T X() const { return m_x; }
            T Y() const { return m_y; }
            T Z() const { return m_z; }
            T W() const { return m_w; }

            // Basic operations
            Quaternion operator+(const Quaternion &other) const {
                return Quaternion(m_x + other.m_x,
                                  m_y + other.m_y,
                                  m_z + other.m_z,
                                  m_w + other.m_w);
            }

            Quaternion operator-(const Quaternion &other) const {
                return Quaternion(m_x - other.m_x,
                                  m_y - other.m_y,
                                  m_z - other.m_z,
                                  m_w - other.m_w);
            }

            Quaternion operator*(T scalar) const {
                return Quaternion(
                    m_x * scalar, m_y * scalar, m_z * scalar, m_w * scalar);
            }

            /**
             * @brief Quaternion multiplication (composition of rotations)
             */
            Quaternion operator*(const Quaternion &other) const {
                return Quaternion(m_w * other.m_x + m_x * other.m_w +
                                      m_y * other.m_z - m_z * other.m_y,
                                  m_w * other.m_y - m_x * other.m_z +
                                      m_y * other.m_w + m_z * other.m_x,
                                  m_w * other.m_z + m_x * other.m_y -
                                      m_y * other.m_x + m_z * other.m_w,
                                  m_w * other.m_w - m_x * other.m_x -
                                      m_y * other.m_y - m_z * other.m_z);
            }

            /**
             * @brief Get the conjugate of the quaternion
             */
            Quaternion Conjugate() const {
                return Quaternion(-m_x, -m_y, -m_z, m_w);
            }

            /**
             * @brief Get the magnitude (length) of the quaternion
             */
            T Magnitude() const {
                return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w);
            }

            /**
             * @brief Normalize the quaternion
             */
            void Normalize() {
                T mag = Magnitude();
                if (mag > T(0)) {
                    T invMag = T(1) / mag;
                    m_x *= invMag;
                    m_y *= invMag;
                    m_z *= invMag;
                    m_w *= invMag;
                }
            }

            /**
             * @brief Get a normalized copy of the quaternion
             */
            Quaternion Normalized() const {
                Quaternion q = *this;
                q.Normalize();
                return q;
            }

            /**
             * @brief Convert to rotation matrix
             */
            Matrix<T, 3, 3> ToRotationMatrix() const {
                Matrix<T, 3, 3> result;

                T xx = m_x * m_x;
                T xy = m_x * m_y;
                T xz = m_x * m_z;
                T xw = m_x * m_w;
                T yy = m_y * m_y;
                T yz = m_y * m_z;
                T yw = m_y * m_w;
                T zz = m_z * m_z;
                T zw = m_z * m_w;

                result(0, 0) = T(1) - T(2) * (yy + zz);
                result(0, 1) = T(2) * (xy - zw);
                result(0, 2) = T(2) * (xz + yw);

                result(1, 0) = T(2) * (xy + zw);
                result(1, 1) = T(1) - T(2) * (xx + zz);
                result(1, 2) = T(2) * (yz - xw);

                result(2, 0) = T(2) * (xz - yw);
                result(2, 1) = T(2) * (yz + xw);
                result(2, 2) = T(1) - T(2) * (xx + yy);

                return result;
            }

            /**
             * @brief Convert to Euler angles (in radians)
             * @return Vector3 containing (pitch, yaw, roll)
             */
            Vector<T, 3> ToEulerAngles() const {
                Vector<T, 3> angles;

                // pitch (x-axis rotation)
                T sinp = T(2) * (m_w * m_x - m_y * m_z);
                if (std::abs(sinp) >= T(1)) {
                    angles[0] = std::copysign(
                        M_PI / T(2), sinp);  // use 90 degrees if out of range
                } else {
                    angles[0] = std::asin(sinp);
                }

                // yaw (y-axis rotation)
                T siny_cosp = T(2) * (m_w * m_y + m_x * m_z);
                T cosy_cosp = T(1) - T(2) * (m_x * m_x + m_y * m_y);
                angles[1] = std::atan2(siny_cosp, cosy_cosp);

                // roll (z-axis rotation)
                T sinr_cosp = T(2) * (m_w * m_z + m_x * m_y);
                T cosr_cosp = T(1) - T(2) * (m_x * m_x + m_z * m_z);
                angles[2] = std::atan2(sinr_cosp, cosr_cosp);

                return angles;
            }

            /**
             * @brief Spherical linear interpolation between quaternions
             */
            static Quaternion Slerp(const Quaternion &q1,
                                    const Quaternion &q2,
                                    T t) {
                // Compute the cosine of the angle between the quaternions
                T cosHalfTheta = q1.m_x * q2.m_x + q1.m_y * q2.m_y +
                                 q1.m_z * q2.m_z + q1.m_w * q2.m_w;

                // If q1=q2 or q1=-q2 then theta = 0 and we can return q1
                if (std::abs(cosHalfTheta) >= T(1.0)) {
                    return q1;
                }

                // Calculate temporary values
                T halfTheta = std::acos(cosHalfTheta);
                T sinHalfTheta =
                    std::sqrt(T(1.0) - cosHalfTheta * cosHalfTheta);

                // If theta = 180 degrees then result is not fully defined
                // we could rotate around any axis normal to q1 or q2
                if (std::abs(sinHalfTheta) < T(0.001)) {
                    return Quaternion(q1.m_x * T(0.5) + q2.m_x * T(0.5),
                                      q1.m_y * T(0.5) + q2.m_y * T(0.5),
                                      q1.m_z * T(0.5) + q2.m_z * T(0.5),
                                      q1.m_w * T(0.5) + q2.m_w * T(0.5));
                }

                T ratioA = std::sin((T(1) - t) * halfTheta) / sinHalfTheta;
                T ratioB = std::sin(t * halfTheta) / sinHalfTheta;

                return Quaternion(q1.m_x * ratioA + q2.m_x * ratioB,
                                  q1.m_y * ratioA + q2.m_y * ratioB,
                                  q1.m_z * ratioA + q2.m_z * ratioB,
                                  q1.m_w * ratioA + q2.m_w * ratioB);
            }

          private:
            T m_x, m_y, m_z, m_w;
        };

        // Common quaternion types
        using Quaternionf = Quaternion<float>;
        using Quaterniond = Quaternion<double>;

    }  // namespace Math
}  // namespace Engine
