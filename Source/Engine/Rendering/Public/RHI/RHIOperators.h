#pragma once

#include <float.h>
#include <vulkan/vulkan.h>

#include <cstring>

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

// Vulkan structure comparison operators
inline bool operator==(const VkDescriptorSetLayoutBinding& lhs,
                       const VkDescriptorSetLayoutBinding& rhs) {
    return lhs.binding == rhs.binding &&
           lhs.descriptorType == rhs.descriptorType &&
           lhs.descriptorCount == rhs.descriptorCount &&
           lhs.stageFlags == rhs.stageFlags &&
           lhs.pImmutableSamplers == rhs.pImmutableSamplers;
}

inline bool operator==(const VkPipelineShaderStageCreateInfo& lhs,
                       const VkPipelineShaderStageCreateInfo& rhs) {
    return lhs.flags == rhs.flags && lhs.stage == rhs.stage &&
           lhs.module == rhs.module &&
           (std::strcmp(lhs.pName, rhs.pName) == 0) &&
           lhs.pSpecializationInfo == rhs.pSpecializationInfo;
}

inline bool operator==(const VkPushConstantRange& lhs,
                       const VkPushConstantRange& rhs) {
    return lhs.stageFlags == rhs.stageFlags && lhs.offset == rhs.offset &&
           lhs.size == rhs.size;
}

inline bool operator==(const VkVertexInputAttributeDescription& lhs,
                       const VkVertexInputAttributeDescription& rhs) {
    return lhs.location == rhs.location && lhs.binding == rhs.binding &&
           lhs.format == rhs.format && lhs.offset == rhs.offset;
}

inline bool operator==(const VkVertexInputBindingDescription& lhs,
                       const VkVertexInputBindingDescription& rhs) {
    return lhs.binding == rhs.binding && lhs.stride == rhs.stride &&
           lhs.inputRate == rhs.inputRate;
}
