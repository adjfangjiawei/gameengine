#include <iomanip>
#include <iostream>

#include "Core/Assert.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector.h"

namespace Engine {
    namespace Test {

        // Helper function to compare floating point values
        template <typename T>
        bool IsEqual(T a, T b, T epsilon = T(0.0001)) {
            return std::abs(a - b) < epsilon;
        }

        void RunMathTests() {
            // Test 1: Vector Operations
            {
                std::cout << "\nTesting Vector Operations:" << std::endl;

                // Vector construction and basic operations
                Math::Vector3f v1{1.0f, 2.0f, 3.0f};
                Math::Vector3f v2{4.0f, 5.0f, 6.0f};

                // Addition
                auto sum = v1 + v2;
                ASSERT(IsEqual(sum[0], 5.0f), "Vector addition failed");
                ASSERT(IsEqual(sum[1], 7.0f), "Vector addition failed");
                ASSERT(IsEqual(sum[2], 9.0f), "Vector addition failed");

                // Dot product
                float dot = v1.Dot(v2);
                ASSERT(IsEqual(dot, 32.0f), "Vector dot product failed");

                // Cross product
                auto cross = Math::Cross(v1, v2);
                ASSERT(IsEqual(cross[0], -3.0f), "Vector cross product failed");
                ASSERT(IsEqual(cross[1], 6.0f), "Vector cross product failed");
                ASSERT(IsEqual(cross[2], -3.0f), "Vector cross product failed");

                // Length and normalization
                float length = v1.Length();
                ASSERT(IsEqual(length, std::sqrt(14.0f)),
                       "Vector length calculation failed");

                auto normalized = v1.Normalized();
                ASSERT(IsEqual(normalized.Length(), 1.0f),
                       "Vector normalization failed");

                std::cout << "Vector operations tests passed!" << std::endl;
            }

            // Test 2: Matrix Operations
            {
                std::cout << "\nTesting Matrix Operations:" << std::endl;

                // Create test matrices
                Math::Matrix3f m1;  // Identity matrix
                Math::Matrix3f m2(std::array<std::array<float, 3>, 3>{
                    std::array<float, 3>{1.0f, 2.0f, 3.0f},
                    std::array<float, 3>{4.0f, 5.0f, 6.0f},
                    std::array<float, 3>{7.0f, 8.0f, 9.0f}});

                // Matrix multiplication
                auto product = m1 * m2;
                ASSERT(product(0, 0) == m2(0, 0),
                       "Matrix multiplication failed");
                ASSERT(product(1, 1) == m2(1, 1),
                       "Matrix multiplication failed");
                ASSERT(product(2, 2) == m2(2, 2),
                       "Matrix multiplication failed");

                // Matrix determinant
                float det = m2.Determinant();
                ASSERT(IsEqual(det, 0.0f),
                       "Matrix determinant calculation failed");

                // Matrix transpose
                auto transposed = m2.Transpose();
                ASSERT(transposed(0, 1) == m2(1, 0), "Matrix transpose failed");
                ASSERT(transposed(1, 2) == m2(2, 1), "Matrix transpose failed");

                // Create invertible matrix
                Math::Matrix3f m3(std::array<std::array<float, 3>, 3>{
                    std::array<float, 3>{4.0f, 0.0f, 0.0f},
                    std::array<float, 3>{0.0f, 5.0f, 0.0f},
                    std::array<float, 3>{0.0f, 0.0f, 6.0f}});

                // Matrix inverse
                auto inverse = m3.Inverse();
                auto identity = m3 * inverse;
                ASSERT(IsEqual(identity(0, 0), 1.0f), "Matrix inverse failed");
                ASSERT(IsEqual(identity(1, 1), 1.0f), "Matrix inverse failed");
                ASSERT(IsEqual(identity(2, 2), 1.0f), "Matrix inverse failed");

                std::cout << "Matrix operations tests passed!" << std::endl;
            }

            // Test 3: Quaternion Operations
            {
                std::cout << "\nTesting Quaternion Operations:" << std::endl;

                // Create quaternions
                Math::Quaternionf q1;  // Identity quaternion
                Math::Quaternionf q2(
                    0.0f,
                    1.0f,
                    0.0f,
                    0.0f);  // 180-degree rotation around Y axis

                // Test quaternion multiplication
                auto product = q1 * q2;
                ASSERT(IsEqual(product.X(), q2.X()),
                       "Quaternion multiplication failed");
                ASSERT(IsEqual(product.Y(), q2.Y()),
                       "Quaternion multiplication failed");
                ASSERT(IsEqual(product.Z(), q2.Z()),
                       "Quaternion multiplication failed");
                ASSERT(IsEqual(product.W(), q2.W()),
                       "Quaternion multiplication failed");

                // Test conversion to rotation matrix
                auto rotMatrix = q2.ToRotationMatrix();
                ASSERT(IsEqual(rotMatrix(0, 0), -1.0f),
                       "Quaternion to matrix conversion failed");
                ASSERT(IsEqual(rotMatrix(1, 1), 1.0f),
                       "Quaternion to matrix conversion failed");
                ASSERT(IsEqual(rotMatrix(2, 2), -1.0f),
                       "Quaternion to matrix conversion failed");

                // Test Euler angle conversion
                float pitch = M_PI / 4.0f;  // 45 degrees
                float yaw = M_PI / 3.0f;    // 60 degrees
                float roll = M_PI / 6.0f;   // 30 degrees

                auto qFromEuler =
                    Math::Quaternionf::FromEulerAngles(pitch, yaw, roll);
                auto angles = qFromEuler.ToEulerAngles();

                ASSERT(IsEqual(angles[0], pitch),
                       "Euler angle conversion failed");
                ASSERT(IsEqual(angles[1], yaw),
                       "Euler angle conversion failed");
                ASSERT(IsEqual(angles[2], roll),
                       "Euler angle conversion failed");

                // Test quaternion interpolation
                Math::Quaternionf q3(0.0f, 0.0f, 0.0f, 1.0f);  // Identity
                Math::Quaternionf q4(
                    0.0f, 1.0f, 0.0f, 0.0f);  // 180-degree Y rotation

                auto qInterp = Math::Quaternionf::Slerp(q3, q4, 0.5f);
                ASSERT(IsEqual(qInterp.Magnitude(), 1.0f),
                       "Quaternion interpolation failed");

                std::cout << "Quaternion operations tests passed!" << std::endl;
            }

            // Test 4: Combined Operations
            {
                std::cout << "\nTesting Combined Operations:" << std::endl;

                // Create a rotation quaternion (90 degrees around Y axis)
                Math::Quaternionf rotation =
                    Math::Quaternionf::FromEulerAngles(0.0f, M_PI / 2.0f, 0.0f);

                // Create a point to rotate
                Math::Vector3f point{1.0f, 0.0f, 0.0f};

                // Convert quaternion to matrix
                Math::Matrix3f rotMatrix = rotation.ToRotationMatrix();

                // Rotate point using matrix
                Math::Vector3f rotatedPoint = rotMatrix * point;

                // The point (1,0,0) rotated 90 degrees around Y should be
                // approximately (0,0,-1)
                ASSERT(IsEqual(rotatedPoint[0], 0.0f),
                       "Combined rotation failed");
                ASSERT(IsEqual(rotatedPoint[1], 0.0f),
                       "Combined rotation failed");
                ASSERT(IsEqual(rotatedPoint[2], -1.0f),
                       "Combined rotation failed");

                std::cout << "Combined operations tests passed!" << std::endl;
            }

            std::cout << "\nAll math library tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine

// Add main function
int main() {
    try {
        Engine::Test::RunMathTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
