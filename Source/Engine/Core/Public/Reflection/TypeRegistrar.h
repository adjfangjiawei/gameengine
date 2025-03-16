#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Reflection/MethodInfo.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeInfo.h"

namespace Engine {

    class TypeRegistrar {
      public:
        // Register a type and return its TypeInfo
        template <typename T>
        static TypeInfo& RegisterType(const std::string& name) {
            auto type = std::make_unique<TypeInfo>(name, sizeof(T));
            auto* typePtr = type.get();
            TypeRegistry::Get().RegisterType(std::move(type));
            return *typePtr;
        }

        // Register a property
        template <typename Class, typename PropertyType>
        static void RegisterProperty(const std::string& className,
                                     const std::string& propertyName,
                                     PropertyType Class::* property) {
            auto* type = TypeRegistry::Get().GetType(className);
            if (!type) return;

            auto propertyInfo = std::make_unique<PropertyInfo>(
                propertyName,
                [property](void* instance) -> void* {
                    return &(static_cast<Class*>(instance)->*property);
                },
                [property](void* instance, const void* value) {
                    static_cast<Class*>(instance)->*property =
                        *static_cast<const PropertyType*>(value);
                });

            type->AddProperty(std::move(propertyInfo));
        }

        // Register a method
        template <typename Class, typename MethodType>
        static void RegisterMethod(const std::string& className,
                                   const std::string& methodName,
                                   MethodType method) {
            auto* type = TypeRegistry::Get().GetType(className);
            if (!type) return;

            auto methodInfo = CreateMethodInfo(methodName, method);
            type->AddMethod(std::move(methodInfo));
        }

      private:
        // Helper function to create method info for non-void return types
        template <typename Class, typename Ret, typename... Args>
        static std::unique_ptr<MethodInfo> CreateMethodInfo(
            const std::string& name, Ret (Class::*method)(Args...)) {
            return std::make_unique<MethodInfo>(
                name,
                [method](void* instance,
                         const std::vector<void*>& args) -> void* {
                    if (args.size() != sizeof...(Args)) {
                        return nullptr;
                    }
                    return InvokeMethod<Class, Ret, Args...>(
                        static_cast<Class*>(instance), method, args);
                });
        }

        // Helper function to create method info for const non-void return types
        template <typename Class, typename Ret, typename... Args>
        static std::unique_ptr<MethodInfo> CreateMethodInfo(
            const std::string& name, Ret (Class::*method)(Args...) const) {
            return std::make_unique<MethodInfo>(
                name,
                [method](void* instance,
                         const std::vector<void*>& args) -> void* {
                    if (args.size() != sizeof...(Args)) {
                        return nullptr;
                    }
                    return InvokeMethod<Class, Ret, Args...>(
                        static_cast<const Class*>(instance), method, args);
                });
        }

        // Helper function to create method info for void return type
        template <typename Class, typename... Args>
        static std::unique_ptr<MethodInfo> CreateMethodInfo(
            const std::string& name, void (Class::*method)(Args...)) {
            return std::make_unique<MethodInfo>(
                name,
                [method](void* instance,
                         const std::vector<void*>& args) -> void* {
                    if (args.size() != sizeof...(Args)) {
                        return nullptr;
                    }
                    InvokeVoidMethod<Class, Args...>(
                        static_cast<Class*>(instance), method, args);
                    return nullptr;
                });
        }

        // Helper function to create method info for const void return type
        template <typename Class, typename... Args>
        static std::unique_ptr<MethodInfo> CreateMethodInfo(
            const std::string& name, void (Class::*method)(Args...) const) {
            return std::make_unique<MethodInfo>(
                name,
                [method](void* instance,
                         const std::vector<void*>& args) -> void* {
                    if (args.size() != sizeof...(Args)) {
                        return nullptr;
                    }
                    InvokeVoidMethod<Class, Args...>(
                        static_cast<const Class*>(instance), method, args);
                    return nullptr;
                });
        }

        // Helper function to invoke method with return value
        template <typename Class, typename Ret, typename... Args, size_t... I>
        static void* InvokeMethodImpl(Class* instance,
                                      Ret (Class::*method)(Args...),
                                      const std::vector<void*>& args,
                                      std::index_sequence<I...>) {
            using ReturnType = typename std::remove_reference<Ret>::type;
            if constexpr (std::is_reference_v<Ret>) {
                auto& result = (instance->*method)(
                    *static_cast<typename std::remove_reference<Args>::type*>(
                        args[I])...);
                return const_cast<void*>(static_cast<const void*>(&result));
            } else {
                auto result = (instance->*method)(
                    *static_cast<typename std::remove_reference<Args>::type*>(
                        args[I])...);
                return static_cast<void*>(new ReturnType(std::move(result)));
            }
        }

        // Helper function to invoke const method with return value
        template <typename Class, typename Ret, typename... Args, size_t... I>
        static void* InvokeMethodImpl(const Class* instance,
                                      Ret (Class::*method)(Args...) const,
                                      const std::vector<void*>& args,
                                      std::index_sequence<I...>) {
            using ReturnType = typename std::remove_reference<Ret>::type;
            if constexpr (std::is_reference_v<Ret>) {
                auto& result = (instance->*method)(
                    *static_cast<typename std::remove_reference<Args>::type*>(
                        args[I])...);
                return const_cast<void*>(static_cast<const void*>(&result));
            } else {
                auto* result = new ReturnType((instance->*method)(
                    *static_cast<typename std::remove_reference<Args>::type*>(
                        args[I])...));
                return result;
            }
        }

        // Helper function to invoke void method
        template <typename Class, typename... Args, size_t... I>
        static void InvokeVoidMethodImpl(Class* instance,
                                         void (Class::*method)(Args...),
                                         const std::vector<void*>& args,
                                         std::index_sequence<I...>) {
            (instance->*method)(
                *static_cast<typename std::remove_reference<Args>::type*>(
                    args[I])...);
        }

        // Helper function to invoke const void method
        template <typename Class, typename... Args, size_t... I>
        static void InvokeVoidMethodImpl(const Class* instance,
                                         void (Class::*method)(Args...) const,
                                         const std::vector<void*>& args,
                                         std::index_sequence<I...>) {
            (instance->*method)(
                *static_cast<typename std::remove_reference<Args>::type*>(
                    args[I])...);
        }

        // Public interface for method invocation
        template <typename Class, typename Ret, typename... Args>
        static void* InvokeMethod(Class* instance,
                                  Ret (Class::*method)(Args...),
                                  const std::vector<void*>& args) {
            return InvokeMethodImpl(
                instance,
                method,
                args,
                std::make_index_sequence<sizeof...(Args)>{});
        }

        // Public interface for const method invocation
        template <typename Class, typename Ret, typename... Args>
        static void* InvokeMethod(const Class* instance,
                                  Ret (Class::*method)(Args...) const,
                                  const std::vector<void*>& args) {
            return InvokeMethodImpl(
                instance,
                method,
                args,
                std::make_index_sequence<sizeof...(Args)>{});
        }

        // Public interface for void method invocation
        template <typename Class, typename... Args>
        static void InvokeVoidMethod(Class* instance,
                                     void (Class::*method)(Args...),
                                     const std::vector<void*>& args) {
            InvokeVoidMethodImpl(instance,
                                 method,
                                 args,
                                 std::make_index_sequence<sizeof...(Args)>{});
        }

        // Public interface for const void method invocation
        template <typename Class, typename... Args>
        static void InvokeVoidMethod(const Class* instance,
                                     void (Class::*method)(Args...) const,
                                     const std::vector<void*>& args) {
            InvokeVoidMethodImpl(instance,
                                 method,
                                 args,
                                 std::make_index_sequence<sizeof...(Args)>{});
        }
    };

}  // namespace Engine
