
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include "Assert.h"
#include "Serialization/Serializer.h"

namespace Engine {
    namespace Test {

        // Example game object for testing serialization
        class GameObject : public ISerializable {
          public:
            GameObject() = default;
            GameObject(const std::string &name,
                       const std::string &type,
                       int health)
                : m_name(name), m_type(type), m_health(health) {}

            // Implement ISerializable interface
            void SerializeToJson(nlohmann::json &json) const override {
                json["name"] = m_name;
                json["type"] = m_type;
                json["health"] = m_health;
                json["position"] = {{"x", m_position[0]},
                                    {"y", m_position[1]},
                                    {"z", m_position[2]}};
            }

            void DeserializeFromJson(const nlohmann::json &json) override {
                m_name = json["name"].get<std::string>();
                m_type = json["type"].get<std::string>();
                m_health = json["health"].get<int>();
                m_position[0] = json["position"]["x"].get<float>();
                m_position[1] = json["position"]["y"].get<float>();
                m_position[2] = json["position"]["z"].get<float>();
            }

            void SerializeToBinary(std::vector<uint8_t> &data) const override {
                SerializationUtils::WriteStringToBinary(data, m_name);
                SerializationUtils::WriteStringToBinary(data, m_type);
                SerializationUtils::WriteToBinary(data, m_health);
                SerializationUtils::WriteToBinary(data, m_position[0]);
                SerializationUtils::WriteToBinary(data, m_position[1]);
                SerializationUtils::WriteToBinary(data, m_position[2]);
            }

            size_t DeserializeFromBinary(const std::vector<uint8_t> &data,
                                         size_t offset) override {
                auto [name, offset1] =
                    SerializationUtils::ReadStringFromBinary(data, offset);
                auto [type, offset2] =
                    SerializationUtils::ReadStringFromBinary(data, offset1);
                auto [health, offset3] =
                    SerializationUtils::ReadFromBinary<int>(data, offset2);
                auto [x, offset4] =
                    SerializationUtils::ReadFromBinary<float>(data, offset3);
                auto [y, offset5] =
                    SerializationUtils::ReadFromBinary<float>(data, offset4);
                auto [z, offset6] =
                    SerializationUtils::ReadFromBinary<float>(data, offset5);

                m_name = name;
                m_type = type;
                m_health = health;
                m_position[0] = x;
                m_position[1] = y;
                m_position[2] = z;

                return offset6;
            }

            // Getters for testing
            const std::string &GetName() const { return m_name; }
            const std::string &GetType() const { return m_type; }
            int GetHealth() const { return m_health; }
            float GetPositionX() const { return m_position[0]; }
            float GetPositionY() const { return m_position[1]; }
            float GetPositionZ() const { return m_position[2]; }

            // Setters for testing
            void SetPosition(float x, float y, float z) {
                m_position[0] = x;
                m_position[1] = y;
                m_position[2] = z;
            }

          private:
            std::string m_name;
            std::string m_type;
            int m_health = 100;
            float m_position[3] = {0.0f, 0.0f, 0.0f};
        };

        void RunSerializationTests() {
            // Test 1: JSON Serialization
            {
                GameObject original("Player1", "Warrior", 100);
                original.SetPosition(1.5f, 2.5f, 3.5f);

                // Create JSON serializer
                auto serializer = SerializerFactory::CreateSerializer(
                    SerializationFormat::JSON);
                ASSERT(serializer != nullptr,
                       "Failed to create JSON serializer");

                // Serialize object
                std::string jsonData = serializer->Serialize(original);
                std::cout << "JSON Serialization Result:\n"
                          << jsonData << std::endl;

                // Deserialize to new object
                GameObject deserialized;
                serializer->Deserialize(jsonData, deserialized);

                // Verify deserialized data
                ASSERT(deserialized.GetName() == "Player1",
                       "Name not correctly deserialized");
                ASSERT(deserialized.GetType() == "Warrior",
                       "Type not correctly deserialized");
                ASSERT(deserialized.GetHealth() == 100,
                       "Health not correctly deserialized");
                ASSERT(deserialized.GetPositionX() == 1.5f,
                       "Position X not correctly deserialized");
                ASSERT(deserialized.GetPositionY() == 2.5f,
                       "Position Y not correctly deserialized");
                ASSERT(deserialized.GetPositionZ() == 3.5f,
                       "Position Z not correctly deserialized");
            }

            // Test 2: Binary Serialization
            {
                GameObject original("Player2", "Mage", 80);
                original.SetPosition(4.5f, 5.5f, 6.5f);

                // Create binary serializer
                auto serializer = SerializerFactory::CreateSerializer(
                    SerializationFormat::Binary);
                ASSERT(serializer != nullptr,
                       "Failed to create binary serializer");

                // Serialize object
                std::string binaryData = serializer->Serialize(original);

                // Print binary data size
                std::cout << "Binary Serialization Size: " << binaryData.size()
                          << " bytes" << std::endl;

                // Deserialize to new object
                GameObject deserialized;
                serializer->Deserialize(binaryData, deserialized);

                // Verify deserialized data
                ASSERT(deserialized.GetName() == "Player2",
                       "Name not correctly deserialized from binary");
                ASSERT(deserialized.GetType() == "Mage",
                       "Type not correctly deserialized from binary");
                ASSERT(deserialized.GetHealth() == 80,
                       "Health not correctly deserialized from binary");
                ASSERT(deserialized.GetPositionX() == 4.5f,
                       "Position X not correctly deserialized from binary");
                ASSERT(deserialized.GetPositionY() == 5.5f,
                       "Position Y not correctly deserialized from binary");
                ASSERT(deserialized.GetPositionZ() == 6.5f,
                       "Position Z not correctly deserialized from binary");
            }

            // Test 3: Serialization Performance
            {
                const int numObjects = 10000;
                std::vector<GameObject> objects;
                objects.reserve(numObjects);

                // Create test objects
                for (int i = 0; i < numObjects; ++i) {
                    std::stringstream ss;
                    ss << "Player" << i;
                    objects.emplace_back(ss.str(), "TestType", i % 100);
                    objects.back().SetPosition(static_cast<float>(i) * 0.1f,
                                               static_cast<float>(i) * 0.2f,
                                               static_cast<float>(i) * 0.3f);
                }

                // Test JSON serialization performance
                {
                    auto jsonSerializer = SerializerFactory::CreateSerializer(
                        SerializationFormat::JSON);
                    auto startTime = std::chrono::high_resolution_clock::now();

                    for (const auto &obj : objects) {
                        std::string jsonData = jsonSerializer->Serialize(obj);
                    }

                    auto duration =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::high_resolution_clock::now() -
                            startTime);
                    std::cout << "JSON Serialization Time: " << duration.count()
                              << "ms for " << numObjects << " objects"
                              << std::endl;
                }

                // Test Binary serialization performance
                {
                    auto binarySerializer = SerializerFactory::CreateSerializer(
                        SerializationFormat::Binary);
                    auto startTime = std::chrono::high_resolution_clock::now();

                    for (const auto &obj : objects) {
                        std::string binaryData =
                            binarySerializer->Serialize(obj);
                    }

                    auto duration =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::high_resolution_clock::now() -
                            startTime);
                    std::cout
                        << "Binary Serialization Time: " << duration.count()
                        << "ms for " << numObjects << " objects" << std::endl;
                }
            }

            std::cout << "Serialization tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine
