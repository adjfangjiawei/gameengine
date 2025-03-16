
#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>

#include "RHI/RHIQueryHeap.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan查询堆实现
        class VulkanQueryHeap : public IRHIQueryHeap {
          public:
            VulkanQueryHeap(VkDevice device, const QueryHeapDesc& desc);
            virtual ~VulkanQueryHeap();

            // IRHIQueryHeap接口实现
            virtual const QueryHeapDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void GetResults(uint32 startIndex,
                                    uint32 numQueries,
                                    void* data,
                                    uint32 stride) override;

            // Vulkan特定方法
            VkQueryPool GetVkQueryPool() const { return m_QueryPool; }
            VkQueryType GetVkQueryType() const;

          private:
            void CreateQueryPool();

            VkDevice m_Device;
            QueryHeapDesc m_Desc;
            VkQueryPool m_QueryPool;
        };

        // Vulkan时间戳查询实现
        class VulkanTimestampQuery : public IRHITimestampQuery {
          public:
            VulkanTimestampQuery(VkDevice device,
                                 VkPhysicalDevice physicalDevice);
            virtual ~VulkanTimestampQuery();

            // IRHITimestampQuery接口实现
            virtual uint64 GetTimestampFrequency() const override;
            virtual void CalibrateGPUTimestamp(uint64& gpuTimestamp,
                                               uint64& cpuTimestamp) override;

          private:
            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            VkQueryPool m_CalibrationQueryPool;
            uint64 m_TimestampPeriod;
        };

        // Vulkan性能计数器实现
        class VulkanPerformanceCounters : public IRHIPerformanceCounters {
          public:
            VulkanPerformanceCounters(VkDevice device,
                                      VkPhysicalDevice physicalDevice);
            virtual ~VulkanPerformanceCounters();

            // IRHIPerformanceCounters接口实现
            virtual std::vector<PerformanceCounterDesc> GetAvailableCounters()
                const override;
            virtual void BeginSampling(
                const std::vector<std::string>& counterNames) override;
            virtual void EndSampling() override;
            virtual double GetCounterValue(
                const std::string& counterName) const override;

          private:
            void InitializeCounters();
            uint32 GetCounterIndex(const std::string& counterName) const;

            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            VkQueryPool m_PerformanceQueryPool;

            std::vector<PerformanceCounterDesc> m_AvailableCounters;
            std::unordered_map<std::string, uint32> m_CounterIndices;
            std::vector<double> m_CounterValues;
            bool m_IsSampling;
        };

        // 辅助函数
        namespace VulkanQueryUtils {
            VkQueryType ConvertQueryType(EQueryType type);
            VkQueryPipelineStatisticFlags ConvertPipelineStatisticFlags(
                EPipelineStatisticFlags flags);
            void ConvertPipelineStatistics(const void* src,
                                           PipelineStatistics& dst);
        }  // namespace VulkanQueryUtils

    }  // namespace RHI
}  // namespace Engine
