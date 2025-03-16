
#pragma once

#include <vulkan/vulkan.h>

#include <cstring>
#include <string>

namespace Engine {
    namespace RHI {

        // VkDescriptorSetLayoutBinding 比较操作符
        inline bool operator==(const VkDescriptorSetLayoutBinding& a,
                               const VkDescriptorSetLayoutBinding& b) {
            return a.binding == b.binding &&
                   a.descriptorType == b.descriptorType &&
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
            if (a.stageFlags != b.stageFlags)
                return a.stageFlags < b.stageFlags;
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
            if (a.stageFlags != b.stageFlags)
                return a.stageFlags < b.stageFlags;
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

    }  // namespace RHI
}  // namespace Engine
