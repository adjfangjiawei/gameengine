
#pragma once

#include <vulkan/vulkan.h>

#include <cstring>
#include <string>

#include "RHI/RHIDefinitions.h"

namespace Engine {
    namespace RHI {

        // 混合因子转换
        inline VkBlendFactor ConvertToVkBlendFactor(EBlendFactor factor) {
            switch (factor) {
                case EBlendFactor::Zero:
                    return VK_BLEND_FACTOR_ZERO;
                case EBlendFactor::One:
                    return VK_BLEND_FACTOR_ONE;
                case EBlendFactor::SrcColor:
                    return VK_BLEND_FACTOR_SRC_COLOR;
                case EBlendFactor::InvSrcColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case EBlendFactor::SrcAlpha:
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                case EBlendFactor::InvSrcAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case EBlendFactor::DestAlpha:
                    return VK_BLEND_FACTOR_DST_ALPHA;
                case EBlendFactor::InvDestAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case EBlendFactor::DestColor:
                    return VK_BLEND_FACTOR_DST_COLOR;
                case EBlendFactor::InvDestColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                default:
                    return VK_BLEND_FACTOR_ONE;
            }
        }

        // 混合操作转换
        inline VkBlendOp ConvertToVkBlendOp(EBlendOp op) {
            switch (op) {
                case EBlendOp::Add:
                    return VK_BLEND_OP_ADD;
                case EBlendOp::Subtract:
                    return VK_BLEND_OP_SUBTRACT;
                case EBlendOp::RevSubtract:
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case EBlendOp::Min:
                    return VK_BLEND_OP_MIN;
                case EBlendOp::Max:
                    return VK_BLEND_OP_MAX;
                default:
                    return VK_BLEND_OP_ADD;
            }
        }

        // 模板操作转换
        inline VkStencilOp ConvertToVkStencilOp(EStencilOp op) {
            switch (op) {
                case EStencilOp::Keep:
                    return VK_STENCIL_OP_KEEP;
                case EStencilOp::Zero:
                    return VK_STENCIL_OP_ZERO;
                case EStencilOp::Replace:
                    return VK_STENCIL_OP_REPLACE;
                case EStencilOp::IncrSat:
                    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case EStencilOp::DecrSat:
                    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case EStencilOp::Invert:
                    return VK_STENCIL_OP_INVERT;
                case EStencilOp::Incr:
                    return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case EStencilOp::Decr:
                    return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default:
                    return VK_STENCIL_OP_KEEP;
            }
        }

    }  // namespace RHI
}  // namespace Engine
// Vulkan structure comparison operators
// VkDescriptorSetLayoutBinding 比较操作符
inline bool operator==(const VkDescriptorSetLayoutBinding& a,
                       const VkDescriptorSetLayoutBinding& b) {
    return a.binding == b.binding && a.descriptorType == b.descriptorType &&
           a.descriptorCount == b.descriptorCount &&
           a.stageFlags == b.stageFlags;
}

inline bool operator<(const VkDescriptorSetLayoutBinding& a,
                      const VkDescriptorSetLayoutBinding& b) {
    if (a.binding != b.binding) return a.binding < b.binding;
    if (a.descriptorType != b.descriptorType)
        return a.descriptorType < b.descriptorType;
    if (a.descriptorCount != b.descriptorCount)
        return a.descriptorCount < b.descriptorCount;
    if (a.stageFlags != b.stageFlags) return a.stageFlags < b.stageFlags;
    return false;
}

// VkPipelineShaderStageCreateInfo 比较操作符
inline bool operator==(const VkPipelineShaderStageCreateInfo& a,
                       const VkPipelineShaderStageCreateInfo& b) {
    return a.stage == b.stage && a.module == b.module &&
           strcmp(a.pName, b.pName) == 0;
}

inline bool operator<(const VkPipelineShaderStageCreateInfo& a,
                      const VkPipelineShaderStageCreateInfo& b) {
    if (a.stage != b.stage) return a.stage < b.stage;
    if (a.module != b.module) return a.module < b.module;
    return false;
}

// VkPushConstantRange 比较操作符
inline bool operator==(const VkPushConstantRange& a,
                       const VkPushConstantRange& b) {
    return a.stageFlags == b.stageFlags && a.offset == b.offset &&
           a.size == b.size;
}

inline bool operator<(const VkPushConstantRange& a,
                      const VkPushConstantRange& b) {
    if (a.stageFlags != b.stageFlags) return a.stageFlags < b.stageFlags;
    if (a.offset != b.offset) return a.offset < b.offset;
    if (a.size != b.size) return a.size < b.size;
    return false;
}

// VkVertexInputAttributeDescription 比较操作符
inline bool operator==(const VkVertexInputAttributeDescription& a,
                       const VkVertexInputAttributeDescription& b) {
    return a.location == b.location && a.binding == b.binding &&
           a.format == b.format && a.offset == b.offset;
}

inline bool operator<(const VkVertexInputAttributeDescription& a,
                      const VkVertexInputAttributeDescription& b) {
    if (a.location != b.location) return a.location < b.location;
    if (a.binding != b.binding) return a.binding < b.binding;
    if (a.format != b.format) return a.format < b.format;
    if (a.offset != b.offset) return a.offset < b.offset;
    return false;
}

// VkVertexInputBindingDescription 比较操作符
inline bool operator==(const VkVertexInputBindingDescription& a,
                       const VkVertexInputBindingDescription& b) {
    return a.binding == b.binding && a.stride == b.stride &&
           a.inputRate == b.inputRate;
}

inline bool operator<(const VkVertexInputBindingDescription& a,
                      const VkVertexInputBindingDescription& b) {
    if (a.binding != b.binding) return a.binding < b.binding;
    if (a.stride != b.stride) return a.stride < b.stride;
    if (a.inputRate != b.inputRate) return a.inputRate < b.inputRate;
    return false;
}