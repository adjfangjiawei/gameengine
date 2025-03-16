
#include "VulkanAsyncCompute.h"

#include <cassert>
#include <vector>

#include "VulkanCommandList.h"
#include "VulkanFence.h"
#include "VulkanRHI.h"

// NVIDIA Device Generated Commands extension
#ifdef VK_NV_device_generated_commands
typedef void(VKAPI_PTR* PFN_vkCmdGenerateCommandsNV)(
    VkCommandBuffer commandBuffer,
    const VkGeneratedCommandsInfoNV* pGeneratedCommandsInfo);
PFN_vkCmdGenerateCommandsNV vkCmdGenerateCommandsNV = nullptr;
#endif

namespace Engine {
    namespace RHI {

        // VulkanAsyncComputeQueue实现
        VulkanAsyncComputeQueue::VulkanAsyncComputeQueue(
            VkDevice device,
            uint32 queueFamilyIndex,
            uint32 queueIndex,
            const AsyncComputeQueueDesc& desc)
            : m_Device(device),
              m_Queue(VK_NULL_HANDLE),
              m_QueueFamilyIndex(queueFamilyIndex),
              m_Desc(desc) {
            // 获取计算队列
            vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &m_Queue);
            assert(m_Queue != VK_NULL_HANDLE);
        }

        VulkanAsyncComputeQueue::~VulkanAsyncComputeQueue() {
            // Vulkan队列由设备自动清理
        }

        void VulkanAsyncComputeQueue::ExecuteCommandLists(
            uint32 count, IRHICommandListBase* const* commandLists) {
            std::vector<VkCommandBuffer> vkCommandBuffers(count);

            // 收集命令缓冲
            for (uint32 i = 0; i < count; ++i) {
                VulkanCommandList* vulkanCmdList =
                    static_cast<VulkanCommandList*>(commandLists[i]);
                vkCommandBuffers[i] = vulkanCmdList->GetCommandBuffer();
            }

            // 提交命令缓冲
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = count;
            submitInfo.pCommandBuffers = vkCommandBuffers.data();

            VkResult result =
                vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
            assert(result == VK_SUCCESS);
        }

        void VulkanAsyncComputeQueue::Signal(
            IRHIFence* fence,
            uint64 /*value*/) {  // value 用于将来支持时间线信号量
            VulkanFence* vulkanFence = static_cast<VulkanFence*>(fence);
            VkResult result =
                vkQueueSubmit(m_Queue, 0, nullptr, vulkanFence->GetVkFence());
            assert(result == VK_SUCCESS);
        }

        void VulkanAsyncComputeQueue::Wait(IRHIFence* fence, uint64 value) {
            VulkanFence* vulkanFence = static_cast<VulkanFence*>(fence);

            // 根据围栏类型使用适当的等待机制
            if (vulkanFence->GetDesc().Type == EFenceType::Timeline) {
                // 对于时间线信号量，使用 vkWaitSemaphores
                VkSemaphoreWaitInfo waitInfo = {};
                waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                waitInfo.semaphoreCount = 1;
                VkSemaphore semaphore = vulkanFence->GetVkTimelineSemaphore();
                waitInfo.pSemaphores = &semaphore;
                waitInfo.pValues = &value;

                vkWaitSemaphores(m_Device, &waitInfo, UINT64_MAX);
            } else {
                // 对于普通围栏，使用 vkWaitForFences
                VkFence fence = vulkanFence->GetVkFence();
                if (fence != VK_NULL_HANDLE) {
                    vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);
                }
            }
        }

        // VulkanIndirectCommandSignature实现
        VulkanIndirectCommandSignature::VulkanIndirectCommandSignature(
            VkDevice device, const IndirectCommandSignatureDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_IndirectCommandsLayout(VK_NULL_HANDLE) {
            CreateIndirectCommandsLayout();
        }

        VulkanIndirectCommandSignature::~VulkanIndirectCommandSignature() {
            if (m_IndirectCommandsLayout != VK_NULL_HANDLE) {
                vkDestroyIndirectCommandsLayoutNV(
                    m_Device, m_IndirectCommandsLayout, nullptr);
            }
        }

        void VulkanIndirectCommandSignature::CreateIndirectCommandsLayout() {
            std::vector<VkIndirectCommandsLayoutTokenNV> tokens;

            // 根据标志添加令牌
            if (static_cast<uint32>(m_Desc.Flags) &
                static_cast<uint32>(EIndirectCommandFlags::Draw)) {
                VkIndirectCommandsLayoutTokenNV token = {};
                token.sType =
                    VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV;
                token.tokenType = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV;
                tokens.push_back(token);
            }

            if (static_cast<uint32>(m_Desc.Flags) &
                static_cast<uint32>(EIndirectCommandFlags::DrawIndexed)) {
                VkIndirectCommandsLayoutTokenNV token = {};
                token.sType =
                    VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV;
                token.tokenType =
                    VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV;
                tokens.push_back(token);
            }

            if (static_cast<uint32>(m_Desc.Flags) &
                static_cast<uint32>(EIndirectCommandFlags::Dispatch)) {
                VkIndirectCommandsLayoutTokenNV token = {};
                token.sType =
                    VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV;
                token.tokenType = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NV;
                tokens.push_back(token);
            }

            // 创建间接命令布局
            VkIndirectCommandsLayoutCreateInfoNV createInfo = {};
            createInfo.sType =
                VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NV;
            createInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            createInfo.tokenCount = static_cast<uint32>(tokens.size());
            createInfo.pTokens = tokens.data();
            createInfo.streamCount = 1;
            createInfo.pStreamStrides = &m_Desc.ByteStride;

            VkResult result = vkCreateIndirectCommandsLayoutNV(
                m_Device, &createInfo, nullptr, &m_IndirectCommandsLayout);
            assert(result == VK_SUCCESS);
        }

        // VulkanCommandGenerator实现
        VulkanCommandGenerator::VulkanCommandGenerator(VkDevice device)
            : m_Device(device) {}

        VulkanCommandGenerator::~VulkanCommandGenerator() {}

        void VulkanCommandGenerator::GenerateCommands(
            IRHICommandListBase* commandList,
            IRHIBuffer* inputBuffer,
            [[maybe_unused]] IRHIBuffer* outputBuffer,
            uint32 maxCommands) {
            VulkanCommandList* vulkanCmdList =
                static_cast<VulkanCommandList*>(commandList);
            VulkanBuffer* vulkanInputBuffer =
                static_cast<VulkanBuffer*>(inputBuffer);
            // outputBuffer is reserved for future use

            VkGeneratedCommandsInfoNV generatedCommandsInfo = {};
            generatedCommandsInfo.sType =
                VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV;
            generatedCommandsInfo.pipeline = VK_NULL_HANDLE;  // TODO: 设置管线
            generatedCommandsInfo.pipelineBindPoint =
                VK_PIPELINE_BIND_POINT_COMPUTE;
            generatedCommandsInfo.indirectCommandsLayout =
                VK_NULL_HANDLE;  // TODO: 设置布局
            generatedCommandsInfo.streamCount = 1;
            generatedCommandsInfo.pStreams = nullptr;  // TODO: 设置流
            generatedCommandsInfo.sequencesCount = maxCommands;
            generatedCommandsInfo.preprocessBuffer =
                vulkanInputBuffer->GetHandle();
            generatedCommandsInfo.preprocessOffset = 0;
            generatedCommandsInfo.preprocessSize = vulkanInputBuffer->GetSize();
            generatedCommandsInfo.sequencesCountBuffer = VK_NULL_HANDLE;
            generatedCommandsInfo.sequencesCountOffset = 0;

#ifdef VK_NV_device_generated_commands
            if (vkCmdGenerateCommandsNV) {
                vkCmdGenerateCommandsNV(vulkanCmdList->GetCommandBuffer(),
                                        &generatedCommandsInfo);
            } else {
                // 如果扩展不可用，记录错误
                assert(false &&
                       "NVIDIA Device Generated Commands extension is not "
                       "available");
            }
#else
            // 如果编译时没有扩展支持，记录错误
            assert(
                false &&
                "NVIDIA Device Generated Commands extension is not supported");
#endif
        }

        // VulkanAsyncComputeUtils实现
        namespace VulkanAsyncComputeUtils {

            VkQueueFlags GetQueueFlags(EComputeQueuePriority priority) {
                VkQueueFlags flags = VK_QUEUE_COMPUTE_BIT;

                switch (priority) {
                    case EComputeQueuePriority::High:
                        flags |= VK_QUEUE_PROTECTED_BIT;
                        break;
                    case EComputeQueuePriority::Realtime:
                        flags |= VK_QUEUE_PROTECTED_BIT |
                                 VK_QUEUE_SPARSE_BINDING_BIT;
                        break;
                    default:
                        break;
                }

                return flags;
            }

            VkIndirectCommandsLayoutUsageFlagsNV ConvertIndirectCommandFlags(
                EIndirectCommandFlags flags) {
                VkIndirectCommandsLayoutUsageFlagsNV result = 0;

                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(EIndirectCommandFlags::DrawIndexed))
                    result |=
                        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EXPLICIT_PREPROCESS_BIT_NV;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(EIndirectCommandFlags::Draw))
                    result |=
                        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_INDEXED_SEQUENCES_BIT_NV;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(EIndirectCommandFlags::Dispatch))
                    result |=
                        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_UNORDERED_SEQUENCES_BIT_NV;

                return result;
            }

            void ConvertIndirectDrawArgs(const IndirectDrawArgs& src,
                                         VkDrawIndirectCommand& dst) {
                dst.vertexCount = src.VertexCountPerInstance;
                dst.instanceCount = src.InstanceCount;
                dst.firstVertex = src.StartVertexLocation;
                dst.firstInstance = src.StartInstanceLocation;
            }

            void ConvertIndirectDrawIndexedArgs(
                const IndirectDrawIndexedArgs& src,
                VkDrawIndexedIndirectCommand& dst) {
                dst.indexCount = src.IndexCountPerInstance;
                dst.instanceCount = src.InstanceCount;
                dst.firstIndex = src.StartIndexLocation;
                dst.vertexOffset = src.BaseVertexLocation;
                dst.firstInstance = src.StartInstanceLocation;
            }

            void ConvertIndirectDispatchArgs(const IndirectDispatchArgs& src,
                                             VkDispatchIndirectCommand& dst) {
                dst.x = src.ThreadGroupCountX;
                dst.y = src.ThreadGroupCountY;
                dst.z = src.ThreadGroupCountZ;
            }

        }  // namespace VulkanAsyncComputeUtils

    }  // namespace RHI
}  // namespace Engine
