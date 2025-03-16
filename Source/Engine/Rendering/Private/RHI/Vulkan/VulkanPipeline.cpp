#include "VulkanPipeline.h"

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        namespace {
            constexpr uint32_t MAX_DESCRIPTOR_SETS = 1000;
            constexpr uint32_t MAX_DESCRIPTOR_BINDINGS = 16;
        }  // namespace

        // GraphicsPipelineKey Implementation
        bool GraphicsPipelineKey::operator==(
            const GraphicsPipelineKey& other) const {
            return Layout == other.Layout && RenderPass == other.RenderPass &&
                   SubpassIndex == other.SubpassIndex &&
                   VertexInput.BindingDescs == other.VertexInput.BindingDescs &&
                   VertexInput.AttributeDescs ==
                       other.VertexInput.AttributeDescs &&
                   ShaderStages == other.ShaderStages &&
                   memcmp(&InputAssembly,
                          &other.InputAssembly,
                          sizeof(InputAssembly)) == 0 &&
                   memcmp(&Rasterization,
                          &other.Rasterization,
                          sizeof(Rasterization)) == 0 &&
                   memcmp(&Multisample,
                          &other.Multisample,
                          sizeof(Multisample)) == 0 &&
                   memcmp(&DepthStencil,
                          &other.DepthStencil,
                          sizeof(DepthStencil)) == 0 &&
                   memcmp(&ColorBlend, &other.ColorBlend, sizeof(ColorBlend)) ==
                       0 &&
                   memcmp(&DynamicState,
                          &other.DynamicState,
                          sizeof(DynamicState)) == 0;
        }

        std::size_t GraphicsPipelineKeyHash::operator()(
            const GraphicsPipelineKey& key) const {
            std::size_t hash = 0;
            hash ^=
                std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(key.Layout));
            hash ^= std::hash<uint64_t>{}(
                reinterpret_cast<uint64_t>(key.RenderPass));
            hash ^= std::hash<uint32_t>{}(key.SubpassIndex);

            for (const auto& binding : key.VertexInput.BindingDescs) {
                hash ^= std::hash<uint32_t>{}(binding.binding);
                hash ^= std::hash<uint32_t>{}(binding.stride);
                hash ^= std::hash<uint32_t>{}(binding.inputRate);
            }

            for (const auto& attr : key.VertexInput.AttributeDescs) {
                hash ^= std::hash<uint32_t>{}(attr.location);
                hash ^= std::hash<uint32_t>{}(attr.binding);
                hash ^= std::hash<uint32_t>{}(attr.format);
                hash ^= std::hash<uint32_t>{}(attr.offset);
            }

            for (const auto& stage : key.ShaderStages) {
                hash ^= std::hash<uint32_t>{}(stage.stage);
                hash ^= std::hash<uint64_t>{}(
                    reinterpret_cast<uint64_t>(stage.module));
                hash ^= std::hash<const char*>{}(stage.pName);
            }

            return hash;
        }

        // VulkanPipelineManager Implementation
        VulkanPipelineManager::VulkanPipelineManager(VulkanDevice* device)
            : Device(device) {}

        VulkanPipelineManager::~VulkanPipelineManager() {
            // 清理所有管线布局
            for (auto& pair : PipelineLayouts) {
                vkDestroyPipelineLayout(
                    Device->GetHandle(), pair.second, nullptr);
            }
            PipelineLayouts.clear();

            // 清理所有图形管线
            for (auto& pair : GraphicsPipelines) {
                vkDestroyPipeline(Device->GetHandle(), pair.second, nullptr);
            }
            GraphicsPipelines.clear();

            // 清理所有计算管线
            for (auto& pair : ComputePipelines) {
                vkDestroyPipeline(Device->GetHandle(), pair.second, nullptr);
            }
            ComputePipelines.clear();
        }

        VkPipelineLayout VulkanPipelineManager::GetPipelineLayout(
            const PipelineLayoutKey& key) {
            auto it = PipelineLayouts.find(key);
            if (it != PipelineLayouts.end()) {
                return it->second;
            }

            VkPipelineLayout layout = CreatePipelineLayout(key);
            if (layout != VK_NULL_HANDLE) {
                PipelineLayouts[key] = layout;
            }
            return layout;
        }

        VkPipeline VulkanPipelineManager::GetGraphicsPipeline(
            const GraphicsPipelineKey& key) {
            auto it = GraphicsPipelines.find(key);
            if (it != GraphicsPipelines.end()) {
                return it->second;
            }

            VkPipeline pipeline = CreateGraphicsPipeline(key);
            if (pipeline != VK_NULL_HANDLE) {
                GraphicsPipelines[key] = pipeline;
            }
            return pipeline;
        }

        VkPipeline VulkanPipelineManager::GetComputePipeline(
            const ComputePipelineKey& key) {
            auto it = ComputePipelines.find(key);
            if (it != ComputePipelines.end()) {
                return it->second;
            }

            VkPipeline pipeline = CreateComputePipeline(key);
            if (pipeline != VK_NULL_HANDLE) {
                ComputePipelines[key] = pipeline;
            }
            return pipeline;
        }

        VkPipelineLayout VulkanPipelineManager::CreatePipelineLayout(
            const PipelineLayoutKey& key) {
            VkPipelineLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.setLayoutCount =
                static_cast<uint32_t>(key.SetLayouts.size());
            layoutInfo.pSetLayouts = key.SetLayouts.data();
            layoutInfo.pushConstantRangeCount =
                static_cast<uint32_t>(key.PushConstantRanges.size());
            layoutInfo.pPushConstantRanges = key.PushConstantRanges.data();

            VkPipelineLayout layout;
            if (vkCreatePipelineLayout(
                    Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create pipeline layout!");
                return VK_NULL_HANDLE;
            }

            return layout;
        }

        VkPipeline VulkanPipelineManager::CreateGraphicsPipeline(
            const GraphicsPipelineKey& key) {
            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType =
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

            // 顶点输入状态
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
            vertexInputInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount =
                static_cast<uint32_t>(key.VertexInput.BindingDescs.size());
            vertexInputInfo.pVertexBindingDescriptions =
                key.VertexInput.BindingDescs.data();
            vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(key.VertexInput.AttributeDescs.size());
            vertexInputInfo.pVertexAttributeDescriptions =
                key.VertexInput.AttributeDescs.data();

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &key.InputAssembly;
            pipelineInfo.pRasterizationState = &key.Rasterization;
            pipelineInfo.pMultisampleState = &key.Multisample;
            pipelineInfo.pDepthStencilState = &key.DepthStencil;
            pipelineInfo.pColorBlendState = &key.ColorBlend;
            pipelineInfo.pDynamicState = &key.DynamicState;
            pipelineInfo.layout = key.Layout;
            pipelineInfo.renderPass = key.RenderPass;
            pipelineInfo.subpass = key.SubpassIndex;
            pipelineInfo.stageCount =
                static_cast<uint32_t>(key.ShaderStages.size());
            pipelineInfo.pStages = key.ShaderStages.data();

            // 视口和裁剪矩形状态（使用动态状态）
            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;
            pipelineInfo.pViewportState = &viewportState;

            VkPipeline pipeline;
            if (vkCreateGraphicsPipelines(Device->GetHandle(),
                                          VK_NULL_HANDLE,
                                          1,
                                          &pipelineInfo,
                                          nullptr,
                                          &pipeline) != VK_SUCCESS) {
                LOG_ERROR("Failed to create graphics pipeline!");
                return VK_NULL_HANDLE;
            }

            return pipeline;
        }

        VkPipeline VulkanPipelineManager::CreateComputePipeline(
            const ComputePipelineKey& key) {
            VkComputePipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.stage = key.ShaderStage;
            pipelineInfo.layout = key.Layout;

            VkPipeline pipeline;
            if (vkCreateComputePipelines(Device->GetHandle(),
                                         VK_NULL_HANDLE,
                                         1,
                                         &pipelineInfo,
                                         nullptr,
                                         &pipeline) != VK_SUCCESS) {
                LOG_ERROR("Failed to create compute pipeline!");
                return VK_NULL_HANDLE;
            }

            return pipeline;
        }

        void VulkanPipelineManager::CleanupUnusedPipelines() {
            // TODO: 实现管线缓存清理逻辑
        }

        // VulkanDescriptorManager Implementation
        VulkanDescriptorManager::VulkanDescriptorManager(VulkanDevice* device)
            : Device(device), DescriptorPool(VK_NULL_HANDLE) {
            CreateDescriptorPool();
        }

        VulkanDescriptorManager::~VulkanDescriptorManager() {
            if (DescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    Device->GetHandle(), DescriptorPool, nullptr);
            }

            for (auto& pair : DescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(
                    Device->GetHandle(), pair.second, nullptr);
            }
        }

        void VulkanDescriptorManager::CreateDescriptorPool() {
            std::array<VkDescriptorPoolSize, 6> poolSizes = {
                {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DESCRIPTOR_SETS},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DESCRIPTOR_SETS},
                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  MAX_DESCRIPTOR_SETS},
                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_DESCRIPTOR_SETS},
                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DESCRIPTOR_SETS},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                  MAX_DESCRIPTOR_SETS}}};

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = MAX_DESCRIPTOR_SETS * 6;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();

            if (vkCreateDescriptorPool(
                    Device->GetHandle(), &poolInfo, nullptr, &DescriptorPool) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create descriptor pool!");
            }
        }

        VkDescriptorSetLayout VulkanDescriptorManager::GetDescriptorSetLayout(
            const DescriptorSetLayoutKey& key) {
            auto it = DescriptorSetLayouts.find(key);
            if (it != DescriptorSetLayouts.end()) {
                return it->second;
            }

            VkDescriptorSetLayout layout = CreateDescriptorSetLayout(key);
            if (layout != VK_NULL_HANDLE) {
                DescriptorSetLayouts[key] = layout;
            }
            return layout;
        }

        VkDescriptorSetLayout
        VulkanDescriptorManager::CreateDescriptorSetLayout(
            const DescriptorSetLayoutKey& key) {
            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount =
                static_cast<uint32_t>(key.Bindings.size());
            layoutInfo.pBindings = key.Bindings.data();

            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(
                    Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create descriptor set layout!");
                return VK_NULL_HANDLE;
            }

            return layout;
        }

        VkDescriptorSet VulkanDescriptorManager::AllocateDescriptorSet(
            VkDescriptorSetLayout layout) {
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &layout;

            VkDescriptorSet descriptorSet;
            if (vkAllocateDescriptorSets(Device->GetHandle(),
                                         &allocInfo,
                                         &descriptorSet) != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate descriptor set!");
                return VK_NULL_HANDLE;
            }

            return descriptorSet;
        }

        void VulkanDescriptorManager::UpdateDescriptorSet(
            VkDescriptorSet descriptorSet,
            const std::vector<VkWriteDescriptorSet>& writes) {
            for (auto& write : writes) {
                const_cast<VkWriteDescriptorSet&>(write).dstSet = descriptorSet;
            }

            vkUpdateDescriptorSets(Device->GetHandle(),
                                   static_cast<uint32_t>(writes.size()),
                                   writes.data(),
                                   0,
                                   nullptr);
        }

        void VulkanDescriptorManager::FreeDescriptorSet(
            VkDescriptorSet descriptorSet) {
            if (descriptorSet != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(
                    Device->GetHandle(), DescriptorPool, 1, &descriptorSet);
            }
        }

        void VulkanDescriptorManager::CleanupUnusedLayouts() {
            // TODO: 实现描述符集布局清理逻辑
        }

    }  // namespace RHI
}  // namespace Engine
