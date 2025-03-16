
#include "Core/UUID/UUID.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>

#include "Core/Assert.h"

namespace Engine {

    UUID::UUID() { std::fill(m_data.begin(), m_data.end(), 0); }

    UUID::UUID(const std::string &str) {
        if (!ParseString(str)) {
            // If parsing fails, create nil UUID
            std::fill(m_data.begin(), m_data.end(), 0);
        }
    }

    UUID::UUID(const std::array<uint8_t, 16> &bytes) : m_data(bytes) {}

    UUID UUID::Generate() { return UUIDGenerator::Get().Generate(); }

    UUID UUID::Nil() { return UUID(); }

    std::string UUID::ToString() const {
        std::stringstream ss;

        // Format: 8-4-4-4-12
        for (size_t i = 0; i < 16; ++i) {
            ss << ByteToHex(m_data[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                ss << "-";
            }
        }

        return ss.str();
    }

    bool UUID::IsNil() const {
        return std::all_of(m_data.begin(), m_data.end(), [](uint8_t byte) {
            return byte == 0;
        });
    }

    uint8_t UUID::GetVersion() const { return (m_data[6] >> 4) & 0x0F; }

    uint8_t UUID::GetVariant() const { return (m_data[8] >> 6) & 0x03; }

    bool UUID::operator==(const UUID &other) const {
        return m_data == other.m_data;
    }

    bool UUID::operator!=(const UUID &other) const { return !(*this == other); }

    bool UUID::operator<(const UUID &other) const {
        return m_data < other.m_data;
    }

    bool UUID::operator>(const UUID &other) const { return other < *this; }

    bool UUID::operator<=(const UUID &other) const { return !(other < *this); }

    bool UUID::operator>=(const UUID &other) const { return !(*this < other); }

    bool UUID::ParseString(const std::string &str) {
        // Remove hyphens and spaces
        std::string cleanStr;
        cleanStr.reserve(32);

        for (char c : str) {
            if (c != '-' && !std::isspace(c)) {
                cleanStr += c;
            }
        }

        // Check length
        if (cleanStr.length() != 32) {
            return false;
        }

        // Parse hex digits
        try {
            for (size_t i = 0; i < 16; ++i) {
                m_data[i] = HexToByte(cleanStr.substr(i * 2, 2));
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    std::string UUID::ByteToHex(uint8_t byte) {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte);
        return ss.str();
    }

    uint8_t UUID::HexToByte(const std::string &hex) {
        return static_cast<uint8_t>(std::stoi(hex, nullptr, 16));
    }

    // UUIDGenerator implementation
    UUIDGenerator::UUIDGenerator() : m_sequenceCounter(0) {
        // Seed the RNG with current time and high-resolution clock
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto nanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
                .count();

        std::random_device rd;
        std::seed_seq seq{
            rd(),
            rd(),
            rd(),
            rd(),
            static_cast<unsigned int>(nanos & 0xFFFFFFFF),
            static_cast<unsigned int>((nanos >> 32) & 0xFFFFFFFF)};
        m_rng.seed(seq);

        // Generate random node ID
        GenerateRandomBytes(m_nodeId.data(), m_nodeId.size());
    }

    UUIDGenerator &UUIDGenerator::Get() {
        static UUIDGenerator instance;
        return instance;
    }

    UUID UUIDGenerator::Generate() {
        std::array<uint8_t, 16> bytes;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            GenerateRandomBytes(bytes.data(), bytes.size());
        }

        // Set version (4) and variant (2)
        bytes[6] = (bytes[6] & 0x0F) | 0x40;  // Version 4
        bytes[8] = (bytes[8] & 0x3F) | 0x80;  // Variant 2

        return UUID(bytes);
    }

    void UUIDGenerator::Seed(uint64_t seed) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rng.seed(seed);
    }

    UUID UUIDGenerator::GenerateSequential() {
        std::array<uint8_t, 16> bytes;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Use timestamp for first 8 bytes
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            auto nanos =
                std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
                    .count();

            for (int i = 0; i < 8; ++i) {
                bytes[i] = static_cast<uint8_t>((nanos >> (i * 8)) & 0xFF);
            }

            // Use sequence counter for next 4 bytes
            uint32_t counter = static_cast<uint32_t>(m_sequenceCounter++);
            for (int i = 0; i < 4; ++i) {
                bytes[8 + i] =
                    static_cast<uint8_t>((counter >> (i * 8)) & 0xFF);
            }

            // Use node ID for last 4 bytes
            std::copy(
                m_nodeId.begin(), m_nodeId.begin() + 4, bytes.begin() + 12);
        }

        // Set version (4) and variant (2)
        bytes[6] = (bytes[6] & 0x0F) | 0x40;  // Version 4
        bytes[8] = (bytes[8] & 0x3F) | 0x80;  // Variant 2

        return UUID(bytes);
    }

    UUID UUIDGenerator::GenerateNameBased(const UUID &namespace_uuid,
                                          const std::string &name) {
        // Implementation of UUID version 5 (SHA-1 based)
        // Note: This is a simplified version. A full implementation would use
        // SHA-1
        std::array<uint8_t, 16> bytes = namespace_uuid.GetBytes();

        // Simple hash of the name
        uint32_t hash = 0;
        for (char c : name) {
            hash = hash * 31 + c;
        }

        // Incorporate hash into the UUID
        for (int i = 0; i < 4; ++i) {
            bytes[i] ^= static_cast<uint8_t>((hash >> (i * 8)) & 0xFF);
        }

        // Set version (5) and variant (2)
        bytes[6] = (bytes[6] & 0x0F) | 0x50;  // Version 5
        bytes[8] = (bytes[8] & 0x3F) | 0x80;  // Variant 2

        return UUID(bytes);
    }

    void UUIDGenerator::GenerateRandomBytes(uint8_t *buffer, size_t size) {
        std::uniform_int_distribution<uint32_t> dist(0, 255);

        for (size_t i = 0; i < size; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(m_rng));
        }
    }

}  // namespace Engine
