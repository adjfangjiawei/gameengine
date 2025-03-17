
#include "VulkanAdvancedSync.h"

#include "Core/Public/Log/LogSystem.h"
#include "RHI/RHIOperators.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // VulkanSharedResource实现
        VulkanSharedResource::VulkanSharedResource(
            const SharedResourceDesc& desc, VkDevice device)
            : m_Desc(desc), m_Device(device), m_SharedMemory(VK_NULL_HANDLE) {
            VkExternalMemoryHandleTypeFlags handleTypes = 0;
            if ((desc.Flags & ESharedHandleFlags::Shared) !=
                ESharedHandleFlags::None) {
                handleTypes |= VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
            }
            if ((desc.Flags & ESharedHandleFlags::CrossAdapter) !=
                ESharedHandleFlags::None) {
                handleTypes |= VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
            }

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

            VkExportMemoryAllocateInfo exportAllocInfo = {};
            exportAllocInfo.sType =
                VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            exportAllocInfo.handleTypes = handleTypes;

            allocInfo.pNext = &exportAllocInfo;

            // 分配可共享的设备内存
            VkResult result = vkAllocateMemory(
                m_Device, &allocInfo, nullptr, &m_SharedMemory);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate shared memory");
            }
        }

        VulkanSharedResource::~VulkanSharedResource() {
            if (m_SharedMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, m_SharedMemory, nullptr);
            }
        }

        void* VulkanSharedResource::GetSharedHandle() {
            if (m_SharedMemory == VK_NULL_HANDLE) {
                return nullptr;
            }

            VkMemoryGetFdInfoKHR getFdInfo = {};
            getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
            getFdInfo.memory = m_SharedMemory;
            getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

            int fd;
            VkResult result = vkGetMemoryFdKHR(m_Device, &getFdInfo, &fd);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to get shared memory handle");
                return nullptr;
            }

            return (void*)(intptr_t)fd;
        }

        // VulkanMultiGPUSync实现
        VulkanMultiGPUSync::VulkanMultiGPUSync(const MultiGPUSyncDesc& desc,
                                               VkDevice device)
            : m_Desc(desc), m_Device(device) {
            uint32 gpuCount = 0;
            while (desc.GPUMask >> gpuCount) {
                gpuCount++;
            }

            m_Semaphores.resize(gpuCount);

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (uint32 i = 0; i < gpuCount; i++) {
                if (desc.GPUMask & (1 << i)) {
                    VkResult result = vkCreateSemaphore(
                        m_Device, &semaphoreInfo, nullptr, &m_Semaphores[i]);
                    if (result != VK_SUCCESS) {
                        LOG_ERROR("Failed to create semaphore for GPU {}", i);
                    }
                }
            }
        }

        VulkanMultiGPUSync::~VulkanMultiGPUSync() {
            for (auto semaphore : m_Semaphores) {
                if (semaphore != VK_NULL_HANDLE) {
                    vkDestroySemaphore(m_Device, semaphore, nullptr);
                }
            }
        }

        void VulkanMultiGPUSync::SyncPoint(uint32 gpuIndex) {
            if (gpuIndex >= m_Semaphores.size() ||
                m_Semaphores[gpuIndex] == VK_NULL_HANDLE) {
                LOG_ERROR("Invalid GPU index for sync point");
                return;
            }

            // 在当前命令缓冲区中插入信号操作
            VkSemaphoreSignalInfo signalInfo = {};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.semaphore = m_Semaphores[gpuIndex];
            signalInfo.value = 1;

            VkResult result = vkSignalSemaphore(m_Device, &signalInfo);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to signal semaphore for GPU {}", gpuIndex);
            }
        }

        void VulkanMultiGPUSync::WaitForGPU(uint32 gpuIndex) {
            if (gpuIndex >= m_Semaphores.size() ||
                m_Semaphores[gpuIndex] == VK_NULL_HANDLE) {
                LOG_ERROR("Invalid GPU index for wait");
                return;
            }

            VkSemaphoreWaitInfo waitInfo = {};
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &m_Semaphores[gpuIndex];
            uint64_t value = 1;
            waitInfo.pValues = &value;

            VkResult result = vkWaitSemaphores(m_Device, &waitInfo, UINT64_MAX);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to wait for GPU {}", gpuIndex);
            }
        }

        // VulkanFineSync实现
        VulkanFineSync::VulkanFineSync(const FineSyncDesc& desc,
                                       VkDevice device)
            : m_Desc(desc), m_Device(device) {
            m_Fences.resize(desc.MaxSyncPoints);

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            for (uint32 i = 0; i < desc.MaxSyncPoints; i++) {
                VkResult result =
                    vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fences[i]);
                if (result != VK_SUCCESS) {
                    LOG_ERROR("Failed to create fence for sync point {}", i);
                }
            }
        }

        VulkanFineSync::~VulkanFineSync() {
            for (auto fence : m_Fences) {
                if (fence != VK_NULL_HANDLE) {
                    vkDestroyFence(m_Device, fence, nullptr);
                }
            }
        }

        void VulkanFineSync::InsertSyncPoint(const SyncRange& range) {
            uint32 fenceIndex = m_SyncPoints.size() % m_Fences.size();
            VkFence fence = m_Fences[fenceIndex];

            // 重置fence
            vkResetFences(m_Device, 1, &fence);

            // 记录同步点
            m_SyncPoints[range.Begin] = fenceIndex;

            // 在命令缓冲区中插入fence
            // Note: 这里需要当前正在记录的命令缓冲区，应该从上下文中获取
            // vkCmdSetEvent(currentCommandBuffer, fence,
            // VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        void VulkanFineSync::WaitSyncPoint(uint64 point) {
            auto it = m_SyncPoints.find(point);
            if (it == m_SyncPoints.end()) {
                LOG_ERROR("Invalid sync point");
                return;
            }

            VkFence fence = m_Fences[it->second];
            VkResult result =
                vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to wait for sync point");
            }
        }

        bool VulkanFineSync::IsSyncPointComplete(uint64 point) const {
            auto it = m_SyncPoints.find(point);
            if (it == m_SyncPoints.end()) {
                return false;
            }

            VkFence fence = m_Fences[it->second];
            return vkGetFenceStatus(m_Device, fence) == VK_SUCCESS;
        }

        // VulkanCrossProcessSync实现
        VulkanCrossProcessSync::VulkanCrossProcessSync(
            const CrossProcessSyncDesc& desc, VkDevice device)
            : m_Desc(desc),
              m_Device(device),
              m_ExternalSemaphore(VK_NULL_HANDLE),
              m_HandleType(VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT) {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkExportSemaphoreCreateInfo exportInfo = {};
            exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
            exportInfo.handleTypes = m_HandleType;

            semaphoreInfo.pNext = &exportInfo;

            VkResult result = vkCreateSemaphore(
                m_Device, &semaphoreInfo, nullptr, &m_ExternalSemaphore);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create external semaphore");
            }
        }

        VulkanCrossProcessSync::~VulkanCrossProcessSync() {
            if (m_ExternalSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_ExternalSemaphore, nullptr);
            }
        }

        void VulkanCrossProcessSync::Signal(uint64 value) {
            VkSemaphoreSignalInfo signalInfo = {};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.semaphore = m_ExternalSemaphore;
            signalInfo.value = value;

            VkResult result = vkSignalSemaphore(m_Device, &signalInfo);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to signal external semaphore");
            }
        }

        void VulkanCrossProcessSync::Wait(uint64 value) {
            VkSemaphoreWaitInfo waitInfo = {};
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &m_ExternalSemaphore;
            waitInfo.pValues = &value;

            VkResult result = vkWaitSemaphores(m_Device, &waitInfo, UINT64_MAX);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to wait for external semaphore");
            }
        }

        uint64 VulkanCrossProcessSync::GetCurrentValue() const {
            uint64 value = 0;
            // Note: Vulkan doesn't provide a direct way to get semaphore value
            // We might need to maintain this value ourselves
            return value;
        }

    }  // namespace RHI
}  // namespace Engine
