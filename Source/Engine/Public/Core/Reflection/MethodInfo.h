#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Engine {

    class MethodInfo {
      public:
        using InvokeFunction =
            std::function<void*(void*, const std::vector<void*>&)>;

        MethodInfo(const std::string& name, InvokeFunction invoker)
            : m_name(name), m_invoker(std::move(invoker)) {}

        const std::string& GetName() const { return m_name; }

        template <typename ReturnType>
        ReturnType* Invoke(void* instance,
                           const std::vector<void*>& args) const {
            void* result = m_invoker(instance, args);
            if constexpr (std::is_same_v<ReturnType, void>) {
                return nullptr;
            } else {
                return static_cast<ReturnType*>(result);
            }
        }

      private:
        std::string m_name;
        InvokeFunction m_invoker;
    };

}  // namespace Engine
