
#pragma once

#include <unordered_map>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // 顶点输入布局描述
        struct VulkanVertexInputLayout {
            std::vector<VkVertexInputBindingDescription> BindingDescs;
            std::vector<VkVertexInputAttributeDescription> AttributeDescs;
        };

        // 管线布局缓存键
        struct PipelineLayoutKey {
            std::vector<VkDescriptorSetLayout> SetLayouts;
            std::vector<VkPushConstantRange> PushConstantRanges;

            bool operator==(const PipelineLayoutKey& other) const {
                return SetLayouts == other.SetLayouts &&
                       PushConstantRanges == other.PushConstantRanges;
            }
        };

        // 管线布局缓存键的哈希函数
        struct PipelineLayoutKeyHash {
            std::size_t operator()(const PipelineLayoutKey& key) const {
                std::size_t hash = 0;
                for (const auto& layout : key.SetLayouts) {
                    hash ^= std::hash<uint64_t>{}(
                        reinterpret_cast<uint64_t>(layout));
                }
                for (const auto& range : key.PushConstantRanges) {
                    hash ^= std::hash<uint32_t>{}(range.stageFlags);
                    hash ^= std::hash<uint32_t>{}(range.offset);
                    hash ^= std::hash<uint32_t>{}(range.size);
                }
                return hash;
            }
        };

        // 图形管线缓存键
        struct GraphicsPipelineKey {
            VkPipelineLayout Layout;
            VkRenderPass RenderPass;
            uint32 SubpassIndex;
            VulkanVertexInputLayout VertexInput;
            std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
            VkPipelineInputAssemblyStateCreateInfo InputAssembly;
            VkPipelineRasterizationStateCreateInfo Rasterization;
            VkPipelineMultisampleStateCreateInfo Multisample;
            VkPipelineDepthStencilStateCreateInfo DepthStencil;
            VkPipelineColorBlendStateCreateInfo ColorBlend;
            VkPipelineDynamicStateCreateInfo DynamicState;

            bool operator==(const GraphicsPipelineKey& other) const;
        };

        // 图形管线缓存键的哈希函数
        struct GraphicsPipelineKeyHash {
            std::size_t operator()(const GraphicsPipelineKey& key) const;
        };

        // 计算管线缓存键
        struct ComputePipelineKey {
            VkPipelineLayout Layout;
            VkPipelineShaderStageCreateInfo ShaderStage;

            bool operator==(const ComputePipelineKey& other) const {
                return Layout == other.Layout &&
                       ShaderStage.module == other.ShaderStage.module &&
                       ShaderStage.stage == other.ShaderStage.stage &&
                       strcmp(ShaderStage.pName, other.ShaderStage.pName) == 0;
            }
        };

        // 计算管线缓存键的哈希函数
        struct ComputePipelineKeyHash {
            std::size_t operator()(const ComputePipelineKey& key) const {
                return std::hash<uint64_t>{}(
                           reinterpret_cast<uint64_t>(key.Layout)) ^
                       std::hash<uint64_t>{}(
                           reinterpret_cast<uint64_t>(key.ShaderStage.module)) ^
                       std::hash<uint32_t>{}(key.ShaderStage.stage) ^
                       std::hash<const char*>{}(key.ShaderStage.pName);
            }
        };

        // 管线状态管理器
        class VulkanPipelineManager {
          public:
            VulkanPipelineManager(VulkanDevice* device);
            ~VulkanPipelineManager();

            // 获取或创建管线布局
            VkPipelineLayout GetPipelineLayout(const PipelineLayoutKey& key);

            // 获取或创建图形管线
            VkPipeline GetGraphicsPipeline(const GraphicsPipelineKey& key);

            // 获取或创建计算管线
            VkPipeline GetComputePipeline(const ComputePipelineKey& key);

            // 清理未使用的管线
            void CleanupUnusedPipelines();

          private:
            VkPipelineLayout CreatePipelineLayout(const PipelineLayoutKey& key);
            VkPipeline CreateGraphicsPipeline(const GraphicsPipelineKey& key);
            VkPipeline CreateComputePipeline(const ComputePipelineKey& key);

            VulkanDevice* Device;

            std::unordered_map<PipelineLayoutKey,
                               VkPipelineLayout,
                               PipelineLayoutKeyHash>
                PipelineLayouts;
            std::unordered_map<GraphicsPipelineKey,
                               VkPipeline,
                               GraphicsPipelineKeyHash>
                GraphicsPipelines;
            std::unordered_map<ComputePipelineKey,
                               VkPipeline,
                               ComputePipelineKeyHash>
                ComputePipelines;
        };

        // 描述符集布局缓存键
        struct DescriptorSetLayoutKey {
            std::vector<VkDescriptorSetLayoutBinding> Bindings;

            bool operator==(const DescriptorSetLayoutKey& other) const {
                return Bindings == other.Bindings;
            }
        };

        // 描述符集布局缓存键的哈希函数
        struct DescriptorSetLayoutKeyHash {
            std::size_t operator()(const DescriptorSetLayoutKey& key) const {
                std::size_t hash = 0;
                for (const auto& binding : key.Bindings) {
                    hash ^= std::hash<uint32_t>{}(binding.binding);
                    hash ^= std::hash<uint32_t>{}(binding.descriptorType);
                    hash ^= std::hash<uint32_t>{}(binding.descriptorCount);
                    hash ^= std::hash<uint32_t>{}(binding.stageFlags);
                }
                return hash;
            }
        };

        // 描述符集管理器
        class VulkanDescriptorManager {
          public:
            VulkanDescriptorManager(VulkanDevice* device);
            ~VulkanDescriptorManager();

            // 获取或创建描述符集布局
            VkDescriptorSetLayout GetDescriptorSetLayout(
                const DescriptorSetLayoutKey& key);

            // 分配描述符集
            VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);

            // 更新描述符集
            void UpdateDescriptorSet(
                VkDescriptorSet descriptorSet,
                const std::vector<VkWriteDescriptorSet>& writes);

            // 释放描述符集
            void FreeDescriptorSet(VkDescriptorSet descriptorSet);

            // 清理未使用的描述符集布局
            void CleanupUnusedLayouts();

          private:
            VkDescriptorSetLayout CreateDescriptorSetLayout(
                const DescriptorSetLayoutKey& key);
            void CreateDescriptorPool();

            VulkanDevice* Device;
            VkDescriptorPool DescriptorPool;

            std::unordered_map<DescriptorSetLayoutKey,
                               VkDescriptorSetLayout,
                               DescriptorSetLayoutKeyHash>
                DescriptorSetLayouts;
        };

    }  // namespace RHI
}  // namespace Engine
