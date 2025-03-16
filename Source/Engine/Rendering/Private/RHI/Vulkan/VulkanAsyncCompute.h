
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIAsyncCompute.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan异步计算队列实现
        class VulkanAsyncComputeQueue : public IRHIAsyncComputeQueue {
          public:
            VulkanAsyncComputeQueue(VkDevice device,
                                    uint32 queueFamilyIndex,
                                    uint32 queueIndex,
                                    const AsyncComputeQueueDesc& desc);
            virtual ~VulkanAsyncComputeQueue();

            // IRHIAsyncComputeQueue接口实现
            virtual void ExecuteCommandLists(
                uint32 count,
                IRHICommandListBase* const* commandLists) override;
            virtual void Signal(IRHIFence* fence, uint64 value) override;
            virtual void Wait(IRHIFence* fence, uint64 value) override;
            virtual const AsyncComputeQueueDesc& GetDesc() const override {
                return m_Desc;
            }

            // Vulkan特定方法
            VkQueue GetVkQueue() const { return m_Queue; }
            uint32 GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }

          private:
            VkDevice m_Device;
            VkQueue m_Queue;
            uint32 m_QueueFamilyIndex;
            AsyncComputeQueueDesc m_Desc;
        };

        // Vulkan间接命令签名实现
        class VulkanIndirectCommandSignature
            : public IRHIIndirectCommandSignature {
          public:
            VulkanIndirectCommandSignature(
                VkDevice device, const IndirectCommandSignatureDesc& desc);
            virtual ~VulkanIndirectCommandSignature();

            // IRHIIndirectCommandSignature接口实现
            virtual const IndirectCommandSignatureDesc& GetDesc()
                const override {
                return m_Desc;
            }

            // Vulkan特定方法
            VkIndirectCommandsLayoutNV GetVkIndirectCommandsLayout() const {
                return m_IndirectCommandsLayout;
            }

          private:
            void CreateIndirectCommandsLayout();

            VkDevice m_Device;
            IndirectCommandSignatureDesc m_Desc;
            VkIndirectCommandsLayoutNV m_IndirectCommandsLayout;
        };

        // Vulkan命令生成器实现
        class VulkanCommandGenerator : public IRHICommandGenerator {
          public:
            VulkanCommandGenerator(VkDevice device);
            virtual ~VulkanCommandGenerator();

            // IRHICommandGenerator接口实现
            virtual void GenerateCommands(IRHICommandListBase* commandList,
                                          IRHIBuffer* inputBuffer,
                                          IRHIBuffer* outputBuffer,
                                          uint32 maxCommands) override;

          private:
            VkDevice m_Device;
        };

        // 辅助函数
        namespace VulkanAsyncComputeUtils {
            VkQueueFlags GetQueueFlags(EComputeQueuePriority priority);
            VkIndirectCommandsLayoutUsageFlagsNV ConvertIndirectCommandFlags(
                EIndirectCommandFlags flags);
            void ConvertIndirectDrawArgs(const IndirectDrawArgs& src,
                                         VkDrawIndirectCommand& dst);
            void ConvertIndirectDrawIndexedArgs(
                const IndirectDrawIndexedArgs& src,
                VkDrawIndexedIndirectCommand& dst);
            void ConvertIndirectDispatchArgs(const IndirectDispatchArgs& src,
                                             VkDispatchIndirectCommand& dst);
        }  // namespace VulkanAsyncComputeUtils

    }  // namespace RHI
}  // namespace Engine
