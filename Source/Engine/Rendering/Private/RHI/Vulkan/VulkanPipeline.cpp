#include "VulkanPipeline.h"

#include "Core/Public/Log/LogSystem.h"
#include "VulkanTypeOperators.h"

namespace Engine {
    namespace RHI {

        namespace {
            constexpr uint32_t MAX_DESCRIPTOR_SETS = 1000;
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
                    *Device->GetHandle(), pair.second, nullptr);
            }
            PipelineLayouts.clear();

            // 清理所有图形管线
            for (auto& pair : GraphicsPipelines) {
                vkDestroyPipeline(*Device->GetHandle(), pair.second, nullptr);
            }
            GraphicsPipelines.clear();

            // 清理所有计算管线
            for (auto& pair : ComputePipelines) {
                vkDestroyPipeline(*Device->GetHandle(), pair.second, nullptr);
            }
            ComputePipelines.clear();
        }

        VkPipelineLayout VulkanPipelineManager::GetPipelineLayout(
            const PipelineLayoutKey& key) {
            auto it = PipelineLayouts.find(key);
            if (it != PipelineLayouts.end()) {
                // 更新使用统计
                auto& stats = PipelineLayoutStats[key];
                stats.UseCount++;
                stats.LastUsedTime = static_cast<double>(time(nullptr));
                return it->second;
            }

            VkPipelineLayout layout = CreatePipelineLayout(key);
            if (layout != VK_NULL_HANDLE) {
                PipelineLayouts[key] = layout;
                // 初始化使用统计
                PipelineLayoutStats[key] = {1,
                                            static_cast<double>(time(nullptr))};
            }
            return layout;
        }

        VkPipeline VulkanPipelineManager::GetGraphicsPipeline(
            const GraphicsPipelineKey& key) {
            auto it = GraphicsPipelines.find(key);
            if (it != GraphicsPipelines.end()) {
                // 更新使用统计
                auto& stats = GraphicsPipelineStats[key];
                stats.UseCount++;
                stats.LastUsedTime = static_cast<double>(time(nullptr));
                return it->second;
            }

            VkPipeline pipeline = CreateGraphicsPipeline(key);
            if (pipeline != VK_NULL_HANDLE) {
                GraphicsPipelines[key] = pipeline;
                // 初始化使用统计
                GraphicsPipelineStats[key] = {
                    1, static_cast<double>(time(nullptr))};
            }
            return pipeline;
        }

        VkPipeline VulkanPipelineManager::GetComputePipeline(
            const ComputePipelineKey& key) {
            auto it = ComputePipelines.find(key);
            if (it != ComputePipelines.end()) {
                // 更新使用统计
                auto& stats = ComputePipelineStats[key];
                stats.UseCount++;
                stats.LastUsedTime = static_cast<double>(time(nullptr));
                return it->second;
            }

            VkPipeline pipeline = CreateComputePipeline(key);
            if (pipeline != VK_NULL_HANDLE) {
                ComputePipelines[key] = pipeline;
                // 初始化使用统计
                ComputePipelineStats[key] = {
                    1, static_cast<double>(time(nullptr))};
            }
            return pipeline;
        }

        VkPipelineLayout VulkanPipelineManager::CreatePipelineLayout(
            const PipelineLayoutKey& key) {
            // 验证输入参数
            if (!Device || !*Device->GetHandle()) {
                LOG_ERROR("Invalid device handle in CreatePipelineLayout!");
                return VK_NULL_HANDLE;
            }

            // 验证描述符集布局
            for (const auto& setLayout : key.SetLayouts) {
                if (setLayout == VK_NULL_HANDLE) {
                    LOG_ERROR(
                        "Invalid descriptor set layout in pipeline layout "
                        "creation!");
                    return VK_NULL_HANDLE;
                }
            }

            // 验证推送常量范围
            uint32_t totalPushConstantSize = 0;
            for (const auto& range : key.PushConstantRanges) {
                if (range.size == 0) {
                    LOG_WARNING("Push constant range with size 0 detected!");
                    continue;
                }
                totalPushConstantSize += range.size;
            }

            // 检查推送常量总大小是否超过设备限制
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties(
                Device->GetPhysicalDevice()->GetHandle(), &deviceProps);
            if (totalPushConstantSize >
                deviceProps.limits.maxPushConstantsSize) {
                LOG_ERROR(
                    "Total push constant size ({} bytes) exceeds device limit "
                    "({} bytes)!",
                    totalPushConstantSize,
                    deviceProps.limits.maxPushConstantsSize);
                return VK_NULL_HANDLE;
            }

            // 创建布局信息
            VkPipelineLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.flags = 0;
            layoutInfo.setLayoutCount =
                static_cast<uint32_t>(key.SetLayouts.size());
            layoutInfo.pSetLayouts =
                key.SetLayouts.empty() ? nullptr : key.SetLayouts.data();
            layoutInfo.pushConstantRangeCount =
                static_cast<uint32_t>(key.PushConstantRanges.size());
            layoutInfo.pPushConstantRanges =
                key.PushConstantRanges.empty() ? nullptr
                                               : key.PushConstantRanges.data();

            // 创建管线布局
            VkPipelineLayout layout;
            VkResult result = vkCreatePipelineLayout(
                *Device->GetHandle(), &layoutInfo, nullptr, &layout);

            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create pipeline layout! Error code: {}",
                          result);
                return VK_NULL_HANDLE;
            }

            // 记录性能统计
            LOG_INFO(
                "Created pipeline layout with {} descriptor sets and {} push "
                "constant ranges",
                key.SetLayouts.size(),
                key.PushConstantRanges.size());

            // 更新缓存统计
            uint32_t currentLayoutCount = GetPipelineLayoutCount();
            if (currentLayoutCount % 100 == 0) {
                LOG_INFO("Pipeline layout cache size: {}", currentLayoutCount);
            }

            return layout;
        }

        VkPipeline VulkanPipelineManager::CreateGraphicsPipeline(
            const GraphicsPipelineKey& key) {
            // 验证输入参数
            if (!Device || !*Device->GetHandle()) {
                LOG_ERROR("Invalid device handle in CreateGraphicsPipeline!");
                return VK_NULL_HANDLE;
            }

            if (!key.Layout || !key.RenderPass) {
                LOG_ERROR(
                    "Invalid pipeline layout or render pass in "
                    "CreateGraphicsPipeline!");
                return VK_NULL_HANDLE;
            }

            if (key.ShaderStages.empty()) {
                LOG_ERROR("No shader stages provided for graphics pipeline!");
                return VK_NULL_HANDLE;
            }

            // 验证着色器阶段
            bool hasVertexShader = false;
            [[maybe_unused]] bool hasFragmentShader = false;
            for (const auto& stage : key.ShaderStages) {
                if (!stage.module) {
                    LOG_ERROR("Invalid shader module in pipeline creation!");
                    return VK_NULL_HANDLE;
                }
                if (stage.stage == VK_SHADER_STAGE_VERTEX_BIT)
                    hasVertexShader = true;
                if (stage.stage == VK_SHADER_STAGE_FRAGMENT_BIT)
                    hasFragmentShader = true;
            }

            if (!hasVertexShader) {
                LOG_ERROR("Graphics pipeline must include a vertex shader!");
                return VK_NULL_HANDLE;
            }

            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType =
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

            // 顶点输入状态
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
            vertexInputInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            // 验证顶点输入绑定和属性描述
            if (!key.VertexInput.BindingDescs.empty()) {
                for (const auto& binding : key.VertexInput.BindingDescs) {
                    if (binding.stride == 0) {
                        LOG_WARNING("Vertex binding {} has zero stride!",
                                    binding.binding);
                    }
                }
                vertexInputInfo.vertexBindingDescriptionCount =
                    static_cast<uint32_t>(key.VertexInput.BindingDescs.size());
                vertexInputInfo.pVertexBindingDescriptions =
                    key.VertexInput.BindingDescs.data();
            }

            if (!key.VertexInput.AttributeDescs.empty()) {
                vertexInputInfo.vertexAttributeDescriptionCount =
                    static_cast<uint32_t>(
                        key.VertexInput.AttributeDescs.size());
                vertexInputInfo.pVertexAttributeDescriptions =
                    key.VertexInput.AttributeDescs.data();
            }

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

            // 创建管线
            VkPipeline pipeline;
            VkResult result = vkCreateGraphicsPipelines(*Device->GetHandle(),
                                                        VK_NULL_HANDLE,
                                                        1,
                                                        &pipelineInfo,
                                                        nullptr,
                                                        &pipeline);

            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create graphics pipeline! Error code: {}",
                          result);
                return VK_NULL_HANDLE;
            }

            LOG_INFO("Successfully created graphics pipeline");
            return pipeline;
        }

        VkPipeline VulkanPipelineManager::CreateComputePipeline(
            const ComputePipelineKey& key) {
            // 验证输入参数
            if (!Device || !*Device->GetHandle()) {
                LOG_ERROR("Invalid device handle in CreateComputePipeline!");
                return VK_NULL_HANDLE;
            }

            if (!key.Layout) {
                LOG_ERROR("Invalid pipeline layout in CreateComputePipeline!");
                return VK_NULL_HANDLE;
            }

            // 验证计算着色器
            if (!key.ShaderStage.module) {
                LOG_ERROR("Invalid compute shader module!");
                return VK_NULL_HANDLE;
            }

            if (key.ShaderStage.stage != VK_SHADER_STAGE_COMPUTE_BIT) {
                LOG_ERROR(
                    "Invalid shader stage for compute pipeline! Must be "
                    "compute shader.");
                return VK_NULL_HANDLE;
            }

            if (!key.ShaderStage.pName) {
                LOG_ERROR("Shader entry point name is null!");
                return VK_NULL_HANDLE;
            }

            VkComputePipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.flags = 0;
            pipelineInfo.stage = key.ShaderStage;
            pipelineInfo.layout = key.Layout;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = -1;

            // 创建管线
            VkPipeline pipeline;
            VkResult result = vkCreateComputePipelines(*Device->GetHandle(),
                                                       VK_NULL_HANDLE,
                                                       1,
                                                       &pipelineInfo,
                                                       nullptr,
                                                       &pipeline);

            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create compute pipeline! Error code: {}",
                          result);
                return VK_NULL_HANDLE;
            }

            LOG_INFO("Successfully created compute pipeline");
            return pipeline;
        }

        void VulkanPipelineManager::CleanupUnusedPipelines() {
            double currentTime = static_cast<double>(time(nullptr));
            uint32_t totalCleaned = 0;

            // 获取当前缓存大小
            LOG_INFO("Starting pipeline cleanup...");
            LOG_INFO(
                "Current cache sizes - Layouts: {}, Graphics: {}, Compute: {}",
                PipelineLayouts.size(),
                GraphicsPipelines.size(),
                ComputePipelines.size());

            // 智能清理策略：根据使用频率和时间动态调整清理阈值
            double adjustedTimeThreshold = UNUSED_THRESHOLD;
            uint32_t adjustedUseThreshold = MIN_USES_BEFORE_CLEANUP;

            // 如果缓存过大，降低清理阈值
            if (PipelineLayouts.size() + GraphicsPipelines.size() +
                    ComputePipelines.size() >
                1000) {
                adjustedTimeThreshold *= 0.5;
                adjustedUseThreshold =
                    static_cast<uint32_t>(adjustedUseThreshold * 0.5);
                LOG_INFO(
                    "Cache size exceeded threshold, adjusting cleanup "
                    "parameters");
            }

            // 清理未使用的管线布局
            for (auto it = PipelineLayouts.begin();
                 it != PipelineLayouts.end();) {
                const auto& key = it->first;
                const auto& stats = PipelineLayoutStats[key];
                bool shouldCleanup = false;

                // 智能清理判断
                if (stats.UseCount < adjustedUseThreshold) {
                    if ((currentTime - stats.LastUsedTime) >
                        adjustedTimeThreshold) {
                        shouldCleanup = true;
                    }
                    // 对于使用频率特别低的布局采用更激进的清理策略
                    else if (stats.UseCount < adjustedUseThreshold / 2 &&
                             (currentTime - stats.LastUsedTime) >
                                 adjustedTimeThreshold / 2) {
                        shouldCleanup = true;
                    }
                }

                if (shouldCleanup) {
                    vkDestroyPipelineLayout(
                        *Device->GetHandle(), it->second, nullptr);
                    PipelineLayoutStats.erase(key);
                    it = PipelineLayouts.erase(it);
                    totalCleaned++;
                } else {
                    ++it;
                }
            }

            // 清理未使用的图形管线
            for (auto it = GraphicsPipelines.begin();
                 it != GraphicsPipelines.end();) {
                const auto& key = it->first;
                const auto& stats = GraphicsPipelineStats[key];
                bool shouldCleanup = false;

                // 智能清理判断
                if (stats.UseCount < adjustedUseThreshold) {
                    if ((currentTime - stats.LastUsedTime) >
                        adjustedTimeThreshold) {
                        shouldCleanup = true;
                    }
                    // 对于复杂的图形管线采用更保守的清理策略
                    else if (stats.UseCount < adjustedUseThreshold / 4 &&
                             (currentTime - stats.LastUsedTime) >
                                 adjustedTimeThreshold * 1.5) {
                        shouldCleanup = true;
                    }
                }

                if (shouldCleanup) {
                    vkDestroyPipeline(
                        *Device->GetHandle(), it->second, nullptr);
                    GraphicsPipelineStats.erase(key);
                    it = GraphicsPipelines.erase(it);
                    totalCleaned++;
                } else {
                    ++it;
                }
            }

            // 清理未使用的计算管线
            for (auto it = ComputePipelines.begin();
                 it != ComputePipelines.end();) {
                const auto& key = it->first;
                const auto& stats = ComputePipelineStats[key];
                bool shouldCleanup = false;

                // 智能清理判断
                if (stats.UseCount < adjustedUseThreshold) {
                    if ((currentTime - stats.LastUsedTime) >
                        adjustedTimeThreshold) {
                        shouldCleanup = true;
                    }
                    // 计算管线通常较小，可以采用更激进的清理策略
                    else if (stats.UseCount < adjustedUseThreshold / 3 &&
                             (currentTime - stats.LastUsedTime) >
                                 adjustedTimeThreshold / 1.5) {
                        shouldCleanup = true;
                    }
                }

                if (shouldCleanup) {
                    vkDestroyPipeline(
                        *Device->GetHandle(), it->second, nullptr);
                    ComputePipelineStats.erase(key);
                    it = ComputePipelines.erase(it);
                    totalCleaned++;
                } else {
                    ++it;
                }
            }

            // 记录清理结果
            LOG_INFO("Pipeline cleanup completed:");
            LOG_INFO("- Total items cleaned: {}", totalCleaned);
            LOG_INFO(
                "- Remaining cache sizes - Layouts: {}, Graphics: {}, Compute: "
                "{}",
                PipelineLayouts.size(),
                GraphicsPipelines.size(),
                ComputePipelines.size());

            // 如果清理效果不理想，记录警告
            if (totalCleaned == 0) {
                LOG_WARNING(
                    "No pipelines were cleaned up. Consider adjusting cleanup "
                    "thresholds.");
            }
        }

        // VulkanDescriptorManager Implementation
        VulkanDescriptorManager::VulkanDescriptorManager(VulkanDevice* device)
            : Device(device), DescriptorPool(VK_NULL_HANDLE) {
            CreateDescriptorPool();
        }

        VulkanDescriptorManager::~VulkanDescriptorManager() {
            if (DescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    *Device->GetHandle(), DescriptorPool, nullptr);
            }

            for (auto& pair : DescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(
                    *Device->GetHandle(), pair.second, nullptr);
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

            if (vkCreateDescriptorPool(*Device->GetHandle(),
                                       &poolInfo,
                                       nullptr,
                                       &DescriptorPool) != VK_SUCCESS) {
                LOG_ERROR("Failed to create descriptor pool!");
            }
        }

        VkDescriptorSetLayout VulkanDescriptorManager::GetDescriptorSetLayout(
            const DescriptorSetLayoutKey& key) {
            auto it = DescriptorSetLayouts.find(key);
            if (it != DescriptorSetLayouts.end()) {
                // 更新使用统计
                auto& stats = LayoutUsageStats[key];
                stats.UseCount++;
                stats.LastUsedTime = static_cast<double>(time(nullptr));
                return it->second;
            }

            VkDescriptorSetLayout layout = CreateDescriptorSetLayout(key);
            if (layout != VK_NULL_HANDLE) {
                DescriptorSetLayouts[key] = layout;
                // 初始化使用统计
                LayoutUsageStats[key] = {
                    1,                                   // UseCount
                    static_cast<double>(time(nullptr)),  // LastUsedTime
                    std::vector<VkDescriptorSet>()       // ActiveSets
                };
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
                    *Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create descriptor set layout!");
                return VK_NULL_HANDLE;
            }

            return layout;
        }

        VkDescriptorSet VulkanDescriptorManager::AllocateDescriptorSet(
            VkDescriptorSetLayout layout) {
            // 验证输入参数
            if (!Device || !*Device->GetHandle()) {
                LOG_ERROR("Invalid device handle in AllocateDescriptorSet!");
                return VK_NULL_HANDLE;
            }

            if (!layout) {
                LOG_ERROR("Invalid descriptor set layout!");
                return VK_NULL_HANDLE;
            }

            if (DescriptorPool == VK_NULL_HANDLE) {
                LOG_ERROR("Descriptor pool is not initialized!");
                return VK_NULL_HANDLE;
            }

            // 检查是否达到最大描述符集数量
            uint32_t totalSets = GetTotalActiveDescriptorSetCount();
            if (totalSets >= MAX_DESCRIPTOR_SETS * 6) {
                LOG_ERROR("Maximum number of descriptor sets reached!");
                // 尝试清理未使用的布局
                CleanupUnusedLayouts();
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &layout;

            VkDescriptorSet descriptorSet;
            VkResult result = vkAllocateDescriptorSets(
                *Device->GetHandle(), &allocInfo, &descriptorSet);

            if (result == VK_ERROR_FRAGMENTED_POOL ||
                result == VK_ERROR_OUT_OF_POOL_MEMORY) {
                // 如果池已满或碎片化，创建新的描述符池
                LOG_WARNING(
                    "Descriptor pool is full or fragmented, creating new "
                    "pool...");
                vkDestroyDescriptorPool(
                    *Device->GetHandle(), DescriptorPool, nullptr);
                CreateDescriptorPool();

                // 重试分配
                result = vkAllocateDescriptorSets(
                    *Device->GetHandle(), &allocInfo, &descriptorSet);
            }

            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate descriptor set! Error code: {}",
                          result);
                return VK_NULL_HANDLE;
            }

            // 更新统计信息
            for (auto& pair : LayoutUsageStats) {
                if (DescriptorSetLayouts[pair.first] == layout) {
                    pair.second.ActiveSets.push_back(descriptorSet);
                    break;
                }
            }

            return descriptorSet;
        }

        void VulkanDescriptorManager::UpdateDescriptorSet(
            VkDescriptorSet descriptorSet,
            const std::vector<VkWriteDescriptorSet>& writes) {
            // 验证输入参数
            if (!Device || !*Device->GetHandle()) {
                LOG_ERROR("Invalid device handle in UpdateDescriptorSet!");
                return;
            }

            if (descriptorSet == VK_NULL_HANDLE) {
                LOG_ERROR("Invalid descriptor set!");
                return;
            }

            if (writes.empty()) {
                LOG_WARNING("No descriptor writes provided!");
                return;
            }

            // 验证写入描述符的有效性
            std::vector<VkWriteDescriptorSet> validatedWrites;
            validatedWrites.reserve(writes.size());

            for (const auto& write : writes) {
                if (write.descriptorCount == 0) {
                    LOG_WARNING("Skipping descriptor write with count 0");
                    continue;
                }

                if (!write.pBufferInfo && !write.pImageInfo &&
                    !write.pTexelBufferView) {
                    LOG_WARNING(
                        "Skipping descriptor write with no buffer, image, or "
                        "texel buffer info");
                    continue;
                }

                VkWriteDescriptorSet validWrite = write;
                validWrite.dstSet = descriptorSet;
                validatedWrites.push_back(validWrite);
            }

            if (!validatedWrites.empty()) {
                vkUpdateDescriptorSets(
                    *Device->GetHandle(),
                    static_cast<uint32_t>(validatedWrites.size()),
                    validatedWrites.data(),
                    0,
                    nullptr);
            }
        }

        void VulkanDescriptorManager::FreeDescriptorSet(
            VkDescriptorSet descriptorSet) {
            if (descriptorSet != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(
                    *Device->GetHandle(), DescriptorPool, 1, &descriptorSet);
            }
        }

        std::vector<std::pair<uint32_t, uint32_t>>
        VulkanPipelineManager::GetPipelineUsageStats() const {
            std::vector<std::pair<uint32_t, uint32_t>> stats;

            // 收集图形管线使用统计
            for (const auto& pair : GraphicsPipelineStats) {
                stats.push_back(
                    {pair.second.UseCount,
                     static_cast<uint32_t>(time(nullptr) -
                                           pair.second.LastUsedTime)});
            }

            // 收集计算管线使用统计
            for (const auto& pair : ComputePipelineStats) {
                stats.push_back(
                    {pair.second.UseCount,
                     static_cast<uint32_t>(time(nullptr) -
                                           pair.second.LastUsedTime)});
            }

            return stats;
        }

        uint32_t VulkanDescriptorManager::GetTotalActiveDescriptorSetCount()
            const {
            uint32_t total = 0;
            for (const auto& pair : LayoutUsageStats) {
                total += static_cast<uint32_t>(pair.second.ActiveSets.size());
            }
            return total;
        }

        std::vector<VulkanDescriptorManager::LayoutUsageInfo>
        VulkanDescriptorManager::GetLayoutUsageStats() const {
            std::vector<LayoutUsageInfo> stats;
            const double currentTime = static_cast<double>(time(nullptr));

            for (const auto& pair : LayoutUsageStats) {
                LayoutUsageInfo info;
                info.UseCount = pair.second.UseCount;
                info.TimeSinceLastUse = static_cast<uint32_t>(
                    currentTime - pair.second.LastUsedTime);
                info.ActiveSetCount =
                    static_cast<uint32_t>(pair.second.ActiveSets.size());
                stats.push_back(info);
            }

            return stats;
        }

        void VulkanDescriptorManager::CleanupUnusedLayouts() {
            double currentTime = static_cast<double>(time(nullptr));

            for (auto it = DescriptorSetLayouts.begin();
                 it != DescriptorSetLayouts.end();) {
                const auto& key = it->first;
                const auto& stats = LayoutUsageStats[key];

                // 检查是否满足清理条件：
                // 1. 使用次数少于最小阈值
                // 2. 超过未使用时间阈值
                // 3. 活跃描述符集数量在允许范围内
                if (stats.UseCount < MIN_USES_BEFORE_CLEANUP &&
                    (currentTime - stats.LastUsedTime) > UNUSED_THRESHOLD &&
                    stats.ActiveSets.size() <= MAX_UNUSED_SETS_PER_LAYOUT) {
                    // 释放所有关联的描述符集
                    for (VkDescriptorSet set : stats.ActiveSets) {
                        if (set != VK_NULL_HANDLE) {
                            vkFreeDescriptorSets(
                                *Device->GetHandle(), DescriptorPool, 1, &set);
                        }
                    }

                    // 销毁描述符集布局
                    vkDestroyDescriptorSetLayout(
                        *Device->GetHandle(), it->second, nullptr);

                    // 从统计信息中移除
                    LayoutUsageStats.erase(key);

                    // 从布局映射中移除并获取下一个迭代器
                    it = DescriptorSetLayouts.erase(it);

                    LOG_INFO("Cleaned up unused descriptor set layout");
                } else {
                    ++it;
                }
            }
        }

    }  // namespace RHI
}  // namespace Engine
