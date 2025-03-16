#include <iostream>

#include "Assert.h"
#include "Reflection/IReflectable.h"
#include "Reflection/ReflectionMacros.h"
#include "Reflection/TypeInfo.h"

namespace Engine {
    namespace Test {

        // Example enum for testing reflection
        enum class TestEnum { Value1, Value2, Value3 };

        // Reflect the enum
        BEGIN_ENUM_REFLECTION(TestEnum)
        REFLECT_ENUM_VALUE(TestEnum, Value1)
        REFLECT_ENUM_VALUE(TestEnum, Value2)
        REFLECT_ENUM_VALUE(TestEnum, Value3)
        END_ENUM_REFLECTION(TestEnum)

        // Example base class for testing inheritance reflection
        class TestBaseClass : public IReflectable {
            REFLECTABLE_CLASS(TestBaseClass)

          public:
            virtual ~TestBaseClass() = default;

            int baseValue = 0;
            virtual std::string GetClassName() const { return "TestBaseClass"; }
        };

        // Example component class for testing reflection
        class TestComponent : public TestBaseClass {
            REFLECTABLE_CLASS(TestComponent)

          public:
            // Properties for testing
            int intValue = 42;
            float floatValue = 3.14f;
            std::string stringValue = "Hello Reflection";
            std::vector<int> arrayValue = {1, 2, 3, 4, 5};
            TestEnum enumValue = TestEnum::Value1;

            // Methods for testing
            void SetIntValue(int value) { intValue = value; }
            int GetIntValue() const { return intValue; }

            void SetFloatValue(float value) { floatValue = value; }
            float GetFloatValue() const { return floatValue; }

            void SetStringValue(const std::string &value) {
                stringValue = value;
            }
            const std::string &GetStringValue() const { return stringValue; }

            std::string GetClassName() const override {
                return "TestComponent";
            }

            void TestMethod(int a, float b) {
                std::cout << "TestMethod called with " << a << " and " << b
                          << std::endl;
            }
        };

        // Implement reflection for TestBaseClass
        BEGIN_REFLECTION(TestBaseClass)
        REFLECT_PROPERTY(TestBaseClass, baseValue)
        REFLECT_METHOD(TestBaseClass, GetClassName)
        END_REFLECTION()

        // Implement reflection for TestComponent
        BEGIN_REFLECTION(TestComponent)
        REFLECT_BASE_CLASS(TestComponent, TestBaseClass)
        REFLECT_PROPERTY(TestComponent, intValue)
        REFLECT_PROPERTY(TestComponent, floatValue)
        REFLECT_PROPERTY(TestComponent, stringValue)
        REFLECT_ARRAY_PROPERTY(TestComponent, arrayValue, int)
        REFLECT_PROPERTY(TestComponent, enumValue)
        REFLECT_METHOD(TestComponent, SetIntValue)
        REFLECT_METHOD(TestComponent, GetIntValue)
        REFLECT_METHOD(TestComponent, SetFloatValue)
        REFLECT_METHOD(TestComponent, GetFloatValue)
        REFLECT_METHOD(TestComponent, SetStringValue)
        REFLECT_METHOD(TestComponent, GetStringValue)
        REFLECT_METHOD(TestComponent, TestMethod)
        END_REFLECTION()

        // Implement the reflection registration
        IMPLEMENT_REFLECTION(TestBaseClass)
        IMPLEMENT_REFLECTION(TestComponent)

        void RunReflectionTests() {
            std::cout << "\nRunning Reflection Tests:" << std::endl;

            // Test 1: Type Registration and Lookup
            {
                std::cout << "\nTesting Type Registration and Lookup:"
                          << std::endl;

                const TypeInfo *baseType =
                    TypeRegistry::Get().GetType("TestBaseClass");
                ASSERT(baseType != nullptr,
                       "Failed to find TestBaseClass type");
                ASSERT(baseType->GetName() == "TestBaseClass",
                       "Incorrect type name");

                const TypeInfo *componentType =
                    TypeRegistry::Get().GetType("TestComponent");
                ASSERT(componentType != nullptr,
                       "Failed to find TestComponent type");
                ASSERT(componentType->GetName() == "TestComponent",
                       "Incorrect type name");

                std::cout << "Type registration and lookup tests passed!"
                          << std::endl;
            }

            // Test 2: Property Reflection
            {
                std::cout << "\nTesting Property Reflection:" << std::endl;

                TestComponent component;
                const TypeInfo *type = component.GetTypeInfo();

                // Test integer property
                {
                    const PropertyInfo *intProp = type->GetProperty("intValue");
                    ASSERT(intProp != nullptr,
                           "Failed to find intValue property");

                    int newValue = 100;
                    intProp->Set(&component, &newValue);
                    int *value = intProp->Get<int>(&component);
                    ASSERT(*value == 100,
                           "Property set/get failed for intValue");
                }

                // Test float property
                {
                    const PropertyInfo *floatProp =
                        type->GetProperty("floatValue");
                    ASSERT(floatProp != nullptr,
                           "Failed to find floatValue property");

                    float newValue = 6.28f;
                    floatProp->Set(&component, &newValue);
                    float *value = floatProp->Get<float>(&component);
                    ASSERT(*value == 6.28f,
                           "Property set/get failed for floatValue");
                }

                // Test string property
                {
                    const PropertyInfo *stringProp =
                        type->GetProperty("stringValue");
                    ASSERT(stringProp != nullptr,
                           "Failed to find stringValue property");

                    std::string newValue = "Reflection Works!";
                    stringProp->Set(&component, &newValue);
                    std::string *value =
                        stringProp->Get<std::string>(&component);
                    ASSERT(*value == "Reflection Works!",
                           "Property set/get failed for stringValue");
                }

                std::cout << "Property reflection tests passed!" << std::endl;
            }

            // Test 3: Method Reflection
            {
                std::cout << "\nTesting Method Reflection:" << std::endl;

                TestComponent component;
                const TypeInfo *type = component.GetTypeInfo();

                // Test method lookup
                const MethodInfo *setIntMethod = type->GetMethod("SetIntValue");
                ASSERT(setIntMethod != nullptr,
                       "Failed to find SetIntValue method");

                const MethodInfo *getIntMethod = type->GetMethod("GetIntValue");
                ASSERT(getIntMethod != nullptr,
                       "Failed to find GetIntValue method");

                // Test method invocation
                std::vector<void *> args;
                int arg = 200;
                args.push_back(&arg);
                setIntMethod->Invoke<void>(&component, args);

                args.clear();
                int *result = getIntMethod->Invoke<int>(&component, args);
                ASSERT(*result == 200, "Method invocation failed");

                std::cout << "Method reflection tests passed!" << std::endl;
            }

            // Test 4: Inheritance Reflection
            {
                std::cout << "\nTesting Inheritance Reflection:" << std::endl;

                TestComponent component;
                const TypeInfo *type = component.GetTypeInfo();

                // Verify base class property access
                const PropertyInfo *baseProp = type->GetProperty("baseValue");
                ASSERT(baseProp != nullptr,
                       "Failed to find base class property");

                int newBaseValue = 50;
                baseProp->Set(&component, &newBaseValue);
                int *baseValue = baseProp->Get<int>(&component);
                ASSERT(*baseValue == 50, "Base class property set/get failed");

                std::cout << "Inheritance reflection tests passed!"
                          << std::endl;
            }

            // Test 5: Enum Reflection
            {
                std::cout << "\nTesting Enum Reflection:" << std::endl;

                const TypeInfo *enumType =
                    TypeRegistry::Get().GetType("TestEnum");
                ASSERT(enumType != nullptr, "Failed to find TestEnum type");

                // Verify enum values
                const PropertyInfo *value1 = enumType->GetProperty("Value1");
                ASSERT(value1 != nullptr, "Failed to find enum value Value1");

                const PropertyInfo *value2 = enumType->GetProperty("Value2");
                ASSERT(value2 != nullptr, "Failed to find enum value Value2");

                const PropertyInfo *value3 = enumType->GetProperty("Value3");
                ASSERT(value3 != nullptr, "Failed to find enum value Value3");

                std::cout << "Enum reflection tests passed!" << std::endl;
            }

            std::cout << "\nAll reflection tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine

// Add main function
int main() {
    try {
        Engine::Test::RunReflectionTests();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
