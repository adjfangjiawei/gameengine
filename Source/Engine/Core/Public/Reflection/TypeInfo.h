#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "MethodInfo.h"
#include "PropertyInfo.h"

namespace Engine {

    class TypeInfo {
      public:
        TypeInfo(const std::string& name, size_t size)
            : m_name(name), m_size(size) {}

        const std::string& GetName() const { return m_name; }
        size_t GetSize() const { return m_size; }

        void AddProperty(std::unique_ptr<PropertyInfo> property) const {
            if (property) {
                const_cast<TypeInfo*>(this)->m_properties[property->GetName()] =
                    std::move(property);
            }
        }

        void AddMethod(std::unique_ptr<MethodInfo> method) const {
            if (method) {
                const_cast<TypeInfo*>(this)->m_methods[method->GetName()] =
                    std::move(method);
            }
        }

        const PropertyInfo* GetProperty(const std::string& name) const {
            auto it = m_properties.find(name);
            return it != m_properties.end() ? it->second.get() : nullptr;
        }

        const MethodInfo* GetMethod(const std::string& name) const {
            auto it = m_methods.find(name);
            return it != m_methods.end() ? it->second.get() : nullptr;
        }

      private:
        std::string m_name;
        size_t m_size;
        std::unordered_map<std::string, std::unique_ptr<PropertyInfo>>
            m_properties;
        std::unordered_map<std::string, std::unique_ptr<MethodInfo>> m_methods;
    };

    class TypeRegistry {
      public:
        static TypeRegistry& Get() {
            static TypeRegistry instance;
            return instance;
        }

        void RegisterType(std::unique_ptr<TypeInfo> type) {
            if (type) {
                m_types[type->GetName()] = std::move(type);
            }
        }

        const TypeInfo* GetType(const std::string& name) const {
            auto it = m_types.find(name);
            return it != m_types.end() ? it->second.get() : nullptr;
        }

      private:
        TypeRegistry() = default;
        std::unordered_map<std::string, std::unique_ptr<TypeInfo>> m_types;
    };

}  // namespace Engine
