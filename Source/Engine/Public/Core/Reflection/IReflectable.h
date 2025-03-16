#pragma once

#include "TypeInfo.h"

namespace Engine {

    class IReflectable {
      public:
        virtual ~IReflectable() = default;
        virtual const TypeInfo* GetTypeInfo() const = 0;
    };

}  // namespace Engine
