
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIAdvancedSync.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan共享资源实现
        class VulkanSharedResource : public IRHISharedResource {
          public:
            VulkanSharedResource(const SharedResourceDesc& desc,
                                 VkDevice device);
            virtual ~VulkanSharedResource();

            // IRHISharedResource接口实现
            virtual const SharedResourceDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void* GetSharedHandle() override;

          private:
            SharedResourceDesc m_Desc;
            VkDevice m_Device;
            VkDeviceMemory m_SharedMemory;
        };

        // Vulkan多GPU同步实现
        class VulkanMultiGPUSync : public IRHIMultiGPUSync {
          public:
            VulkanMultiGPUSync(const MultiGPUSyncDesc& desc, VkDevice device);
            virtual ~VulkanMultiGPUSync();

            // IRHIMultiGPUSync接口实现
            virtual const MultiGPUSyncDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void SyncPoint(uint32 gpuIndex) override;
            virtual void WaitForGPU(uint32 gpuIndex) override;

          private:
            MultiGPUSyncDesc m_Desc;
            VkDevice m_Device;
            std::vector<VkSemaphore> m_Semaphores;
        };

        // Vulkan细粒度同步实现
        class VulkanFineSync : public IRHIFineSync {
          public:
            VulkanFineSync(const FineSyncDesc& desc, VkDevice device);
            virtual ~VulkanFineSync();

            // IRHIFineSync接口实现
            virtual const FineSyncDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void InsertSyncPoint(const SyncRange& range) override;
            virtual void WaitSyncPoint(uint64 point) override;
            virtual bool IsSyncPointComplete(uint64 point) const override;

          private:
            FineSyncDesc m_Desc;
            VkDevice m_Device;
            std::vector<VkFence> m_Fences;
            std::unordered_map<uint64, uint32> m_SyncPoints;
        };

        // Vulkan跨进程同步实现
        class VulkanCrossProcessSync : public IRHICrossProcessSync {
          public:
            VulkanCrossProcessSync(const CrossProcessSyncDesc& desc,
                                   VkDevice device);
            virtual ~VulkanCrossProcessSync();

            // IRHICrossProcessSync接口实现
            virtual const CrossProcessSyncDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void Signal(uint64 value) override;
            virtual void Wait(uint64 value) override;
            virtual uint64 GetCurrentValue() const override;

          private:
            CrossProcessSyncDesc m_Desc;
            VkDevice m_Device;
            VkSemaphore m_ExternalSemaphore;
            VkExternalSemaphoreHandleTypeFlagBits m_HandleType;
        };

    }  // namespace RHI
}  // namespace Engine
