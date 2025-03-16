
#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <vector>

#include "Core/CoreTypes.h"

namespace Engine {

    /**
     * @brief Supported serialization formats
     */
    enum class SerializationFormat {
        JSON,
        Binary
        // Add more formats as needed
    };

    /**
     * @brief Interface for serializable objects
     */
    class ISerializable {
      public:
        virtual ~ISerializable() = default;

        /**
         * @brief Serialize the object to JSON
         * @param json The JSON object to serialize to
         */
        virtual void SerializeToJson(nlohmann::json &json) const = 0;

        /**
         * @brief Deserialize the object from JSON
         * @param json The JSON object to deserialize from
         */
        virtual void DeserializeFromJson(const nlohmann::json &json) = 0;

        /**
         * @brief Serialize the object to binary
         * @param data The vector to store binary data
         */
        virtual void SerializeToBinary(std::vector<uint8_t> &data) const = 0;

        /**
         * @brief Deserialize the object from binary
         * @param data The binary data to deserialize from
         * @param offset The current offset in the binary data
         * @return New offset after deserialization
         */
        virtual size_t DeserializeFromBinary(const std::vector<uint8_t> &data,
                                             size_t offset) = 0;
    };

    /**
     * @brief Base serializer interface
     */
    class ISerializer {
      public:
        virtual ~ISerializer() = default;

        /**
         * @brief Serialize an object to string
         * @param object The object to serialize
         * @return Serialized string
         */
        virtual std::string Serialize(const ISerializable &object) const = 0;

        /**
         * @brief Deserialize an object from string
         * @param data The string to deserialize from
         * @param object The object to deserialize into
         */
        virtual void Deserialize(const std::string &data,
                                 ISerializable &object) const = 0;
    };

    /**
     * @brief JSON serializer implementation
     */
    class JsonSerializer : public ISerializer {
      public:
        std::string Serialize(const ISerializable &object) const override {
            nlohmann::json json;
            object.SerializeToJson(json);
            return json.dump(4);  // Pretty print with 4 spaces
        }

        void Deserialize(const std::string &data,
                         ISerializable &object) const override {
            nlohmann::json json = nlohmann::json::parse(data);
            object.DeserializeFromJson(json);
        }
    };

    /**
     * @brief Binary serializer implementation
     */
    class BinarySerializer : public ISerializer {
      public:
        std::string Serialize(const ISerializable &object) const override {
            std::vector<uint8_t> data;
            object.SerializeToBinary(data);
            return std::string(reinterpret_cast<char *>(data.data()),
                               data.size());
        }

        void Deserialize(const std::string &data,
                         ISerializable &object) const override {
            std::vector<uint8_t> binary(data.begin(), data.end());
            object.DeserializeFromBinary(binary, 0);
        }
    };

    /**
     * @brief Helper class for serialization operations
     */
    class SerializationUtils {
      public:
        /**
         * @brief Write a value to binary data
         * @tparam T The type of value to write
         * @param data The vector to write to
         * @param value The value to write
         */
        template <typename T>
        static void WriteToBinary(std::vector<uint8_t> &data, const T &value) {
            static_assert(
                std::is_trivially_copyable<T>::value,
                "Type must be trivially copyable for binary serialization");

            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&value);
            data.insert(data.end(), bytes, bytes + sizeof(T));
        }

        /**
         * @brief Read a value from binary data
         * @tparam T The type of value to read
         * @param data The vector to read from
         * @param offset The current offset in the data
         * @return The read value and new offset
         */
        template <typename T>
        static std::pair<T, size_t> ReadFromBinary(
            const std::vector<uint8_t> &data, size_t offset) {
            static_assert(
                std::is_trivially_copyable<T>::value,
                "Type must be trivially copyable for binary serialization");

            T value;
            std::memcpy(&value, data.data() + offset, sizeof(T));
            return {value, offset + sizeof(T)};
        }

        /**
         * @brief Write a string to binary data
         * @param data The vector to write to
         * @param str The string to write
         */
        static void WriteStringToBinary(std::vector<uint8_t> &data,
                                        const std::string &str) {
            // Write string length
            WriteToBinary(data, static_cast<uint32_t>(str.length()));
            // Write string data
            data.insert(data.end(), str.begin(), str.end());
        }

        /**
         * @brief Read a string from binary data
         * @param data The vector to read from
         * @param offset The current offset in the data
         * @return The read string and new offset
         */
        static std::pair<std::string, size_t> ReadStringFromBinary(
            const std::vector<uint8_t> &data, size_t offset) {
            // Read string length
            auto [length, newOffset] = ReadFromBinary<uint32_t>(data, offset);

            // Read string data
            std::string str(
                reinterpret_cast<const char *>(data.data() + newOffset),
                length);

            return {str, newOffset + length};
        }
    };

    /**
     * @brief Factory for creating serializers
     */
    class SerializerFactory {
      public:
        /**
         * @brief Create a serializer for the specified format
         * @param format The serialization format
         * @return Unique pointer to the created serializer
         */
        static std::unique_ptr<ISerializer> CreateSerializer(
            SerializationFormat format) {
            switch (format) {
                case SerializationFormat::JSON:
                    return std::make_unique<JsonSerializer>();
                case SerializationFormat::Binary:
                    return std::make_unique<BinarySerializer>();
                default:
                    return nullptr;
            }
        }
    };

}  // namespace Engine
