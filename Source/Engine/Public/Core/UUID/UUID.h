
#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>

#include "Core/CoreTypes.h"

namespace Engine {

    /**
     * @brief UUID (Universally Unique Identifier) class
     *
     * Implements UUID version 4 (random) according to RFC 4122.
     * Format: xxxxxxxx-xxxx-4xxx-[8|9|a|b]xxx-xxxxxxxxxxxx
     */
    class UUID {
      public:
        /**
         * @brief Default constructor creates a nil UUID
         */
        UUID();

        /**
         * @brief Create UUID from string representation
         * @param str String representation of UUID
         */
        explicit UUID(const std::string &str);

        /**
         * @brief Create UUID from raw bytes
         * @param bytes Array of 16 bytes
         */
        explicit UUID(const std::array<uint8_t, 16> &bytes);

        /**
         * @brief Generate a new random UUID (v4)
         * @return New UUID instance
         */
        static UUID Generate();

        /**
         * @brief Get nil UUID (all zeros)
         * @return Nil UUID instance
         */
        static UUID Nil();

        /**
         * @brief Convert UUID to string
         * @return String representation of UUID
         */
        std::string ToString() const;

        /**
         * @brief Get raw bytes of UUID
         * @return Array of 16 bytes
         */
        const std::array<uint8_t, 16> &GetBytes() const { return m_data; }

        /**
         * @brief Check if UUID is nil
         * @return True if UUID is nil
         */
        bool IsNil() const;

        /**
         * @brief Get the version of this UUID
         * @return UUID version number
         */
        uint8_t GetVersion() const;

        /**
         * @brief Get the variant of this UUID
         * @return UUID variant number
         */
        uint8_t GetVariant() const;

        // Comparison operators
        bool operator==(const UUID &other) const;
        bool operator!=(const UUID &other) const;
        bool operator<(const UUID &other) const;
        bool operator>(const UUID &other) const;
        bool operator<=(const UUID &other) const;
        bool operator>=(const UUID &other) const;

      private:
        std::array<uint8_t, 16> m_data;

        /**
         * @brief Parse UUID from string
         * @param str String representation of UUID
         * @return True if parsing was successful
         */
        bool ParseString(const std::string &str);

        /**
         * @brief Convert byte to hex string
         * @param byte Byte to convert
         * @return Two-character hex string
         */
        static std::string ByteToHex(uint8_t byte);

        /**
         * @brief Convert hex string to byte
         * @param hex Two-character hex string
         * @return Converted byte value
         */
        static uint8_t HexToByte(const std::string &hex);
    };

    /**
     * @brief UUID generator class
     *
     * Provides thread-safe UUID generation with optional seeding
     * and different generation strategies.
     */
    class UUIDGenerator {
      public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the UUID generator
         */
        static UUIDGenerator &Get();

        /**
         * @brief Generate a new UUID
         * @return New UUID instance
         */
        UUID Generate();

        /**
         * @brief Seed the random number generator
         * @param seed Seed value
         */
        void Seed(uint64_t seed);

        /**
         * @brief Generate a sequential UUID
         *
         * This method generates UUIDs that are guaranteed to be
         * unique within the current process and are sequential.
         *
         * @return New sequential UUID
         */
        UUID GenerateSequential();

        /**
         * @brief Generate a name-based UUID (v5)
         * @param namespace_uuid Namespace UUID
         * @param name Name string
         * @return New name-based UUID
         */
        UUID GenerateNameBased(const UUID &namespace_uuid,
                               const std::string &name);

      private:
        UUIDGenerator();
        ~UUIDGenerator() = default;

        UUIDGenerator(const UUIDGenerator &) = delete;
        UUIDGenerator &operator=(const UUIDGenerator &) = delete;
        UUIDGenerator(UUIDGenerator &&) = delete;
        UUIDGenerator &operator=(UUIDGenerator &&) = delete;

        /**
         * @brief Generate random bytes
         * @param buffer Buffer to fill with random bytes
         * @param size Number of bytes to generate
         */
        void GenerateRandomBytes(uint8_t *buffer, size_t size);

      private:
        std::mutex m_mutex;
        std::mt19937_64 m_rng;
        uint64_t m_sequenceCounter;
        std::array<uint8_t, 16> m_nodeId;
    };

}  // namespace Engine

// Custom hash function for UUID
namespace std {
    template <>
    struct hash<Engine::UUID> {
        size_t operator()(const Engine::UUID &uuid) const {
            const auto &bytes = uuid.GetBytes();
            size_t hash = 0;
            for (size_t i = 0; i < bytes.size(); ++i) {
                hash = hash * 31 + bytes[i];
            }
            return hash;
        }
    };
}  // namespace std
