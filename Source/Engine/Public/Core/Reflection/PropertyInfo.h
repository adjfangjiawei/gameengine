#pragma once

#include <functional>
#include <string>

namespace Engine {

    class TypeInfo;

    class PropertyInfo {
      public:
        using GetterFunction = std::function<void*(void*)>;
        using SetterFunction = std::function<void(void*, const void*)>;

        PropertyInfo(const std::string& name,
                     GetterFunction getter,
                     SetterFunction setter)
            : m_name(name),
              m_type(nullptr),
              m_getter(std::move(getter)),
              m_setter(std::move(setter)) {}

        PropertyInfo(const std::string& name,
                     const TypeInfo* type,
                     GetterFunction getter,
                     SetterFunction setter)
            : m_name(name),
              m_type(type),
              m_getter(std::move(getter)),
              m_setter(std::move(setter)) {}

        const std::string& GetName() const { return m_name; }
        const TypeInfo* GetType() const { return m_type; }

        template <typename T>
        T* Get(void* instance) const {
            if (!m_getter) return nullptr;
            return static_cast<T*>(m_getter(instance));
        }

        void Set(void* instance, const void* value) const {
            if (m_setter) {
                m_setter(instance, value);
            }
        }

        bool IsReadOnly() const { return !m_setter; }

      private:
        std::string m_name;
        const TypeInfo* m_type;
        GetterFunction m_getter;
        SetterFunction m_setter;
    };

}  // namespace Engine
