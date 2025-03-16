#include "VulkanSync.h"

#include "Core/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        // VulkanSyncManager Implementation
        VulkanSyncManager::VulkanSyncManager(VulkanDevice* device)
            : Device(device) {}

        VulkanSyncManager::~VulkanSyncManager() {
            // 清理所有信号量
            for (auto semaphore : Semaphores) {
                if (semaphore != VK_NULL_HANDLE) {
                    vkDestroySemaphore(Device->GetHandle(), semaphore, nullptr);
                }
            }
            Semaphores.clear();

            // 清理所有栅栏
            for (auto fence : Fences) {
                if (fence != VK_NULL_HANDLE) {
                    vkDestroyFence(Device->GetHandle(), fence, nullptr);
                }
            }
            Fences.clear();
        }

        VkSemaphore VulkanSyncManager::CreateSemaphore() {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkSemaphore semaphore;
            if (vkCreateSemaphore(
                    Device->GetHandle(), &semaphoreInfo, nullptr, &semaphore) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create semaphore!");
                return VK_NULL_HANDLE;
            }

            Semaphores.push_back(semaphore);
            return semaphore;
        }

        VkFence VulkanSyncManager::CreateFence(bool signaled) {
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            if (signaled) {
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            }

            VkFence fence;
            if (vkCreateFence(
                    Device->GetHandle(), &fenceInfo, nullptr, &fence) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create fence!");
                return VK_NULL_HANDLE;
            }

            Fences.push_back(fence);
            return fence;
        }

        void VulkanSyncManager::WaitForFence(VkFence fence, uint64_t timeout) {
            if (fence != VK_NULL_HANDLE) {
                vkWaitForFences(
                    Device->GetHandle(), 1, &fence, VK_TRUE, timeout);
            }
        }

        void VulkanSyncManager::ResetFence(VkFence fence) {
            if (fence != VK_NULL_HANDLE) {
                vkResetFences(Device->GetHandle(), 1, &fence);
            }
        }

        void VulkanSyncManager::DestroySemaphore(VkSemaphore semaphore) {
            if (semaphore != VK_NULL_HANDLE) {
                auto it =
                    std::find(Semaphores.begin(), Semaphores.end(), semaphore);
                if (it != Semaphores.end()) {
                    vkDestroySemaphore(Device->GetHandle(), semaphore, nullptr);
                    Semaphores.erase(it);
                }
            }
        }

        void VulkanSyncManager::DestroyFence(VkFence fence) {
            if (fence != VK_NULL_HANDLE) {
                auto it = std::find(Fences.begin(), Fences.end(), fence);
                if (it != Fences.end()) {
                    vkDestroyFence(Device->GetHandle(), fence, nullptr);
                    Fences.erase(it);
                }
            }
        }

        void VulkanSyncManager::WaitIdle() {
            vkDeviceWaitIdle(Device->GetHandle());
        }

        // VulkanCommandBufferSync Implementation
        VulkanCommandBufferSync::VulkanCommandBufferSync(
            VulkanDevice* device, VulkanSyncManager* syncManager)
            : Device(device),
              SyncManager(syncManager),
              Fence(VK_NULL_HANDLE),
              ExecutionSemaphore(VK_NULL_HANDLE),
              IsRecording(false) {
            Fence = SyncManager->CreateFence(true);
            ExecutionSemaphore = SyncManager->CreateSemaphore();
        }

        VulkanCommandBufferSync::~VulkanCommandBufferSync() {
            if (Fence != VK_NULL_HANDLE) {
                SyncManager->DestroyFence(Fence);
            }
            if (ExecutionSemaphore != VK_NULL_HANDLE) {
                SyncManager->DestroySemaphore(ExecutionSemaphore);
            }
        }

        void VulkanCommandBufferSync::Begin() {
            SyncManager->WaitForFence(Fence);
            SyncManager->ResetFence(Fence);
            IsRecording = true;
        }

        void VulkanCommandBufferSync::End() { IsRecording = false; }

        void VulkanCommandBufferSync::Submit(VkCommandBuffer cmdBuffer,
                                             VkQueue queue) {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &ExecutionSemaphore;

            vkQueueSubmit(queue, 1, &submitInfo, Fence);
        }

        void VulkanCommandBufferSync::WaitForCompletion() {
            SyncManager->WaitForFence(Fence);
        }

        // VulkanTimestampQuery Implementation
        VulkanTimestampQuery::VulkanTimestampQuery(VulkanDevice* device)
            : Device(device), QueryPool(VK_NULL_HANDLE), QueryCount(2) {
            VkQueryPoolCreateInfo queryPoolInfo = {};
            queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
            queryPoolInfo.queryCount = QueryCount;

            if (vkCreateQueryPool(
                    Device->GetHandle(), &queryPoolInfo, nullptr, &QueryPool) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create timestamp query pool!");
                return;
            }

            // 获取时间戳周期
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(
                Device->GetPhysicalDevice()->GetHandle(), &properties);
            TimestampPeriod = properties.limits.timestampPeriod;
        }

        VulkanTimestampQuery::~VulkanTimestampQuery() {
            if (QueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(Device->GetHandle(), QueryPool, nullptr);
            }
        }

        void VulkanTimestampQuery::Begin(VkCommandBuffer cmdBuffer) {
            vkCmdResetQueryPool(cmdBuffer, QueryPool, 0, QueryCount);
            vkCmdWriteTimestamp(
                cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPool, 0);
        }

        void VulkanTimestampQuery::End(VkCommandBuffer cmdBuffer) {
            vkCmdWriteTimestamp(
                cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool, 1);
        }

        float VulkanTimestampQuery::GetResult() {
            uint64_t timestamps[2];
            vkGetQueryPoolResults(
                Device->GetHandle(),
                QueryPool,
                0,
                2,
                sizeof(timestamps),
                timestamps,
                sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

            return static_cast<float>(timestamps[1] - timestamps[0]) *
                   TimestampPeriod * 0.000001f;  // 转换为毫秒
        }

        // VulkanOcclusionQuery Implementation
        VulkanOcclusionQuery::VulkanOcclusionQuery(VulkanDevice* device)
            : Device(device), QueryPool(VK_NULL_HANDLE) {
            VkQueryPoolCreateInfo queryPoolInfo = {};
            queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
            queryPoolInfo.queryCount = 1;

            if (vkCreateQueryPool(
                    Device->GetHandle(), &queryPoolInfo, nullptr, &QueryPool) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create occlusion query pool!");
            }
        }

        VulkanOcclusionQuery::~VulkanOcclusionQuery() {
            if (QueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(Device->GetHandle(), QueryPool, nullptr);
            }
        }

        void VulkanOcclusionQuery::Begin(VkCommandBuffer cmdBuffer) {
            vkCmdResetQueryPool(cmdBuffer, QueryPool, 0, 1);
            vkCmdBeginQuery(cmdBuffer, QueryPool, 0, 0);
        }

        void VulkanOcclusionQuery::End(VkCommandBuffer cmdBuffer) {
            vkCmdEndQuery(cmdBuffer, QueryPool, 0);
        }

        uint64_t VulkanOcclusionQuery::GetResult() {
            uint64_t result;
            vkGetQueryPoolResults(
                Device->GetHandle(),
                QueryPool,
                0,
                1,
                sizeof(result),
                &result,
                sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
            return result;
        }

        // VulkanPipelineStatisticsQuery Implementation
        VulkanPipelineStatisticsQuery::VulkanPipelineStatisticsQuery(
            VulkanDevice* device)
            : Device(device), QueryPool(VK_NULL_HANDLE) {
            VkQueryPoolCreateInfo queryPoolInfo = {};
            queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
            queryPoolInfo.queryCount = 1;
            queryPoolInfo.pipelineStatistics =
                VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
                VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
                VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

            if (vkCreateQueryPool(
                    Device->GetHandle(), &queryPoolInfo, nullptr, &QueryPool) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create pipeline statistics query pool!");
            }
        }

        VulkanPipelineStatisticsQuery::~VulkanPipelineStatisticsQuery() {
            if (QueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(Device->GetHandle(), QueryPool, nullptr);
            }
        }

        void VulkanPipelineStatisticsQuery::Begin(VkCommandBuffer cmdBuffer) {
            vkCmdResetQueryPool(cmdBuffer, QueryPool, 0, 1);
            vkCmdBeginQuery(cmdBuffer, QueryPool, 0, 0);
        }

        void VulkanPipelineStatisticsQuery::End(VkCommandBuffer cmdBuffer) {
            vkCmdEndQuery(cmdBuffer, QueryPool, 0);
        }

        VulkanPipelineStatisticsQuery::Statistics
        VulkanPipelineStatisticsQuery::GetResult() {
            Statistics stats;
            uint64_t results[9];
            vkGetQueryPoolResults(
                Device->GetHandle(),
                QueryPool,
                0,
                1,
                sizeof(results),
                results,
                sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

            stats.InputAssemblyVertices = results[0];
            stats.InputAssemblyPrimitives = results[1];
            stats.VertexShaderInvocations = results[2];
            stats.GeometryShaderInvocations = results[3];
            stats.GeometryShaderPrimitives = results[4];
            stats.ClippingInvocations = results[5];
            stats.ClippingPrimitives = results[6];
            stats.FragmentShaderInvocations = results[7];
            stats.ComputeShaderInvocations = results[8];

            return stats;
        }

    }  // namespace RHI
}  // namespace Engine
