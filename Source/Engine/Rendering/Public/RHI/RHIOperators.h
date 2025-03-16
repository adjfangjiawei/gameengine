#pragma once

#include <float.h>
#include <vulkan/vulkan.h>

#include <cstring>

#include "RHIDefinitions.h"

namespace Engine {
    namespace RHI {

        // ERHIAccessFlags operators
        inline ERHIAccessFlags operator|(ERHIAccessFlags a, ERHIAccessFlags b) {
            // 先将枚举值转换为底层类型
            const uint32 val_a = static_cast<uint32>(a);
            const uint32 val_b = static_cast<uint32>(b);
            // 执行位运算
            const uint32 result = val_a | val_b;
            // 验证结果是否为有效的枚举值组合
            // ERHIAccessFlags只使用低4位，每一位都有特定含义
            // 0xF = 1111b，覆盖所有四个可能的标志位
            const uint32 validMask = 0xF;
            // 确保结果只包含有效的标志位
            const uint32 maskedResult = result & validMask;
            return static_cast<ERHIAccessFlags>(maskedResult);
        }

        inline ERHIAccessFlags operator&(ERHIAccessFlags a, ERHIAccessFlags b) {
            // 对于&操作，结果总是小于等于操作数，所以不需要额外的范围检查
            const uint32 val_a = static_cast<uint32>(a);
            const uint32 val_b = static_cast<uint32>(b);
            const uint32 result = val_a & val_b;
            // 由于AND操作的结果必定小于等于操作数，所以结果一定在有效范围内
            return static_cast<ERHIAccessFlags>(result);
        }

        inline ERHIAccessFlags& operator|=(ERHIAccessFlags& a,
                                           ERHIAccessFlags b) {
            // 使用已经定义的|运算符来确保一致性
            a = a | b;
            return a;
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
