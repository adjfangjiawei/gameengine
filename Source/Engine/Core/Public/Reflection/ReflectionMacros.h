#pragma once

#include <functional>
#include <type_traits>

#include "Core/Reflection/TypeInfo.h"
#include "Core/Reflection/TypeRegistrar.h"

// Macro to begin reflection registration for a class
#define BEGIN_REFLECTION(className)        \
    void className::RegisterReflection() { \
        [[maybe_unused]] auto& type =      \
            ::Engine::TypeRegistrar::RegisterType<className>(#className);

// Macro to register a property
#define REFLECT_PROPERTY(className, propertyName) \
    ::Engine::TypeRegistrar::RegisterProperty(    \
        #className, #propertyName, &className::propertyName);

// Macro to register a method
#define REFLECT_METHOD(className, methodName)                           \
    {                                                                   \
        using MethodType = decltype(&className::methodName);            \
        ::Engine::TypeRegistrar::RegisterMethod<className, MethodType>( \
            #className, #methodName, &className::methodName);           \
    }

// Macro to end reflection registration
#define END_REFLECTION() }

// Macro to declare a class as reflectable
#define REFLECTABLE_CLASS(className)                                 \
  public:                                                            \
    static void RegisterReflection();                                \
    static const ::Engine::TypeInfo* StaticTypeInfo() {              \
        return ::Engine::TypeRegistry::Get().GetType(#className);    \
    }                                                                \
    virtual const ::Engine::TypeInfo* GetTypeInfo() const override { \
        return StaticTypeInfo();                                     \
    }                                                                \
                                                                     \
  private:

// Macro to implement reflection registration
#define IMPLEMENT_REFLECTION(className)                                 \
    namespace {                                                         \
        struct className##Registrar {                                   \
            className##Registrar() { className::RegisterReflection(); } \
        };                                                              \
        static className##Registrar s_##className##Registrar{};         \
    }

// Macro to register an enum
#define BEGIN_ENUM_REFLECTION(enumName)                           \
    namespace {                                                   \
        struct enumName##Registrar {                              \
            enumName##Registrar() {                               \
                auto type = std::make_unique<::Engine::TypeInfo>( \
                    #enumName, sizeof(enumName));                 \
                auto& registry = ::Engine::TypeRegistry::Get();

// Macro to register an enum value
#define REFLECT_ENUM_VALUE(enumName, valueName)               \
    type->AddProperty(std::make_unique<Engine::PropertyInfo>( \
        #valueName,                                           \
        nullptr,                                              \
        [](void*) -> void* {                                  \
            static enumName value = enumName::valueName;      \
            return &value;                                    \
        },                                                    \
        [](void*, const void*) {}));

// Macro to end enum reflection
#define END_ENUM_REFLECTION(enumName)                   \
    registry.RegisterType(std::move(type));             \
    }                                                   \
    }                                                   \
    ;                                                   \
    static enumName##Registrar s_##enumName##Registrar; \
    }

// Macro to register a constructor
#define REFLECT_CONSTRUCTOR(className, ...)                  \
    type.AddMethod(std::make_unique<Engine::MethodInfo>(     \
        "Constructor",                                       \
        &type,                                               \
        std::vector<Engine::ParameterInfo>{__VA_ARGS__},     \
        [](void*, const std::vector<void*>& args) -> void* { \
            return new className();                          \
        }));

// Macro to register a property with custom getter/setter
#define REFLECT_PROPERTY_EX(className, propertyName, getter, setter)    \
    Engine::TypeRegistrar::RegisterProperty(                            \
        #className,                                                     \
        #propertyName,                                                  \
        [](void* instance) -> void* {                                   \
            return &(static_cast<className*>(instance)->getter());      \
        },                                                              \
        [](void* instance, const void* value) {                         \
            static_cast<className*>(instance)->setter(                  \
                *static_cast<const decltype(className::propertyName)*>( \
                    value));                                            \
        });

// Macro to register a read-only property
#define REFLECT_PROPERTY_READONLY(className, propertyName)             \
    Engine::TypeRegistrar::RegisterProperty(                           \
        #className,                                                    \
        #propertyName,                                                 \
        [](void* instance) -> void* {                                  \
            return &(static_cast<className*>(instance)->propertyName); \
        },                                                             \
        nullptr);

// Macro to register array property
#define REFLECT_ARRAY_PROPERTY(className, propertyName, elementType)      \
    {                                                                     \
        auto arrayType = std::make_unique<Engine::TypeInfo>(              \
            "Array<" #elementType ">", sizeof(std::vector<elementType>)); \
        Engine::TypeRegistry::Get().RegisterType(std::move(arrayType));   \
        REFLECT_PROPERTY(className, propertyName)                         \
    }

// Macro to register a method with return type and parameters
#define REFLECT_METHOD_EX(className, methodName, returnType, ...)     \
    {                                                                 \
        using MethodType = returnType (className::*)(__VA_ARGS__);    \
        Engine::TypeRegistrar::RegisterMethod<className, MethodType>( \
            #className, #methodName, &className::methodName);         \
    }

// Macro to register a static method
#define REFLECT_STATIC_METHOD(className, methodName)     \
    type.AddMethod(std::make_unique<Engine::MethodInfo>( \
        #methodName,                                     \
        nullptr,                                         \
        std::vector<Engine::ParameterInfo>{},            \
        [](void*, const std::vector<void*>&) -> void* {  \
            className::methodName();                     \
            return nullptr;                              \
        }));

// Macro to register base class
#define REFLECT_BASE_CLASS(className, baseClassName)         \
    type.AddProperty(std::make_unique<Engine::PropertyInfo>( \
        "BaseClass",                                         \
        Engine::TypeRegistry::Get().GetType(#baseClassName), \
        nullptr,                                             \
        nullptr));

// Macro to register interface
#define REFLECT_INTERFACE(className, interfaceName)          \
    type.AddProperty(std::make_unique<Engine::PropertyInfo>( \
        "Interface",                                         \
        Engine::TypeRegistry::Get().GetType(#interfaceName), \
        nullptr,                                             \
        nullptr));

// Macro to register attributes
#define REFLECT_ATTRIBUTE(className, attributeName, attributeValue) \
    type.AddProperty(std::make_unique<Engine::PropertyInfo>(        \
        "Attribute_" #attributeName,                                \
        nullptr,                                                    \
        [](void*) -> void* {                                        \
            static auto value = attributeValue;                     \
            return &value;                                          \
        },                                                          \
        nullptr));
