
#pragma once

#include <float.h>

#include "RHIDefinitions.h"

namespace Engine {
    namespace RHI {

        // ERHIAccessFlags operators
        inline ERHIAccessFlags operator|(ERHIAccessFlags a, ERHIAccessFlags b) {
            return static_cast<ERHIAccessFlags>(static_cast<uint32>(a) |
                                                static_cast<uint32>(b));
        }

        inline ERHIAccessFlags operator&(ERHIAccessFlags a, ERHIAccessFlags b) {
            return static_cast<ERHIAccessFlags>(static_cast<uint32>(a) &
                                                static_cast<uint32>(b));
        }

        inline ERHIAccessFlags& operator|=(ERHIAccessFlags& a,
                                           ERHIAccessFlags b) {
            return a = a | b;
        }

        // ERHIResourceFlags operators
        inline ERHIResourceFlags operator|(ERHIResourceFlags a,
                                           ERHIResourceFlags b) {
            return static_cast<ERHIResourceFlags>(static_cast<uint32>(a) |
                                                  static_cast<uint32>(b));
        }

        inline ERHIResourceFlags operator&(ERHIResourceFlags a,
                                           ERHIResourceFlags b) {
            return static_cast<ERHIResourceFlags>(static_cast<uint32>(a) &
                                                  static_cast<uint32>(b));
        }

        inline ERHIResourceFlags& operator|=(ERHIResourceFlags& a,
                                             ERHIResourceFlags b) {
            return a = a | b;
        }

    }  // namespace RHI
}  // namespace Engine
