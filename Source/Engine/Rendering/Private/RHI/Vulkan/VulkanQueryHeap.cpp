
#include "VulkanQueryHeap.h"

#include <cassert>
#include <chrono>

namespace Engine {
    namespace RHI {

        // VulkanQueryHeap实现
        VulkanQueryHeap::VulkanQueryHeap(VkDevice device,
                                         const QueryHeapDesc& desc)
            : m_Device(device), m_Desc(desc), m_QueryPool(VK_NULL_HANDLE) {
            CreateQueryPool();
        }

        VulkanQueryHeap::~VulkanQueryHeap() {
            if (m_QueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(m_Device, m_QueryPool, nullptr);
            }
        }

        void VulkanQueryHeap::GetResults(uint32 startIndex,
                                         uint32 numQueries,
                                         void* data,
                                         uint32 stride) {
            VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;

            // 对于时间戳查询，等待结果可用
            if (m_Desc.Type == EQueryType::Timestamp) {
                flags |= VK_QUERY_RESULT_WAIT_BIT;
            }

            VkResult result = vkGetQueryPoolResults(m_Device,
                                                    m_QueryPool,
                                                    startIndex,
                                                    numQueries,
                                                    numQueries * stride,
                                                    data,
                                                    stride,
                                                    flags);
            assert(result == VK_SUCCESS || result == VK_NOT_READY);
        }

        void VulkanQueryHeap::CreateQueryPool() {
            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.queryType =
                VulkanQueryUtils::ConvertQueryType(m_Desc.Type);
            createInfo.queryCount = m_Desc.Count;

            // 对于管线统计查询，设置统计标志
            if (m_Desc.Type == EQueryType::PipelineStatistics) {
                createInfo.pipelineStatistics =
                    VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
            }

            VkResult result =
                vkCreateQueryPool(m_Device, &createInfo, nullptr, &m_QueryPool);
            assert(result == VK_SUCCESS);
        }

        VkQueryType VulkanQueryHeap::GetVkQueryType() const {
            return VulkanQueryUtils::ConvertQueryType(m_Desc.Type);
        }

        // VulkanTimestampQuery实现
        VulkanTimestampQuery::VulkanTimestampQuery(
            VkDevice device, VkPhysicalDevice physicalDevice)
            : m_Device(device),
              m_PhysicalDevice(physicalDevice),
              m_CalibrationQueryPool(VK_NULL_HANDLE),
              m_TimestampPeriod(0) {
            // 获取时间戳周期
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            m_TimestampPeriod = properties.limits.timestampPeriod;

            // 创建校准查询池
            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
            createInfo.queryCount = 2;  // 用于CPU/GPU同步

            VkResult result = vkCreateQueryPool(
                m_Device, &createInfo, nullptr, &m_CalibrationQueryPool);
            assert(result == VK_SUCCESS);
        }

        VulkanTimestampQuery::~VulkanTimestampQuery() {
            if (m_CalibrationQueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(m_Device, m_CalibrationQueryPool, nullptr);
            }
        }

        uint64 VulkanTimestampQuery::GetTimestampFrequency() const {
            return static_cast<uint64>(1.0e9 / m_TimestampPeriod);
        }

        void VulkanTimestampQuery::CalibrateGPUTimestamp(uint64& gpuTimestamp,
                                                         uint64& cpuTimestamp) {
            // 重置查询池
            vkResetQueryPool(m_Device, m_CalibrationQueryPool, 0, 2);

            // 记录CPU时间戳
            auto cpuTime = std::chrono::high_resolution_clock::now();
            cpuTimestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               cpuTime.time_since_epoch())
                               .count();

            // 获取GPU时间戳
            uint64 queryResults[2];
            VkResult result = vkGetQueryPoolResults(
                m_Device,
                m_CalibrationQueryPool,
                0,
                2,
                sizeof(queryResults),
                queryResults,
                sizeof(uint64),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
            assert(result == VK_SUCCESS);

            gpuTimestamp = queryResults[0] * m_TimestampPeriod;
        }

        // VulkanPerformanceCounters实现
        VulkanPerformanceCounters::VulkanPerformanceCounters(
            VkDevice device, VkPhysicalDevice physicalDevice)
            : m_Device(device),
              m_PhysicalDevice(physicalDevice),
              m_PerformanceQueryPool(VK_NULL_HANDLE),
              m_IsSampling(false) {
            InitializeCounters();
        }

        VulkanPerformanceCounters::~VulkanPerformanceCounters() {
            if (m_PerformanceQueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(m_Device, m_PerformanceQueryPool, nullptr);
            }
        }

        std::vector<PerformanceCounterDesc>
        VulkanPerformanceCounters::GetAvailableCounters() const {
            return m_AvailableCounters;
        }

        void VulkanPerformanceCounters::BeginSampling(
            [[maybe_unused]] const std::vector<std::string>& counterNames) {
            if (m_IsSampling) return;

            // 重置查询池
            if (m_PerformanceQueryPool != VK_NULL_HANDLE) {
                vkResetQueryPool(
                    m_Device,
                    m_PerformanceQueryPool,
                    0,
                    static_cast<uint32>(m_AvailableCounters.size()));
            }

            m_IsSampling = true;
        }

        void VulkanPerformanceCounters::EndSampling() {
            if (!m_IsSampling) return;

            // 获取查询结果
            if (m_PerformanceQueryPool != VK_NULL_HANDLE) {
                std::vector<uint64> results(m_AvailableCounters.size());
                VkResult result = vkGetQueryPoolResults(
                    m_Device,
                    m_PerformanceQueryPool,
                    0,
                    static_cast<uint32>(m_AvailableCounters.size()),
                    results.size() * sizeof(uint64),
                    results.data(),
                    sizeof(uint64),
                    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
                assert(result == VK_SUCCESS);

                // 更新计数器值
                m_CounterValues.resize(results.size());
                for (size_t i = 0; i < results.size(); ++i) {
                    m_CounterValues[i] = static_cast<double>(results[i]);
                }
            }

            m_IsSampling = false;
        }

        double VulkanPerformanceCounters::GetCounterValue(
            const std::string& counterName) const {
            auto it = m_CounterIndices.find(counterName);
            if (it != m_CounterIndices.end() &&
                it->second < m_CounterValues.size()) {
                return m_CounterValues[it->second];
            }
            return 0.0;
        }

        void VulkanPerformanceCounters::InitializeCounters() {
            // 添加基本性能计数器
            m_AvailableCounters.push_back({"GPU Time",
                                           EPerformanceCounterType::Generic,
                                           "GPU execution time",
                                           "microseconds"});

            m_AvailableCounters.push_back({"Memory Usage",
                                           EPerformanceCounterType::Memory,
                                           "GPU memory usage",
                                           "megabytes"});

            // 更新计数器索引映射
            for (size_t i = 0; i < m_AvailableCounters.size(); ++i) {
                m_CounterIndices[m_AvailableCounters[i].Name] =
                    static_cast<uint32>(i);
            }

            // 创建性能查询池
            if (!m_AvailableCounters.empty()) {
                VkQueryPoolCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
                createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR;
                createInfo.queryCount =
                    static_cast<uint32>(m_AvailableCounters.size());

                VkResult result = vkCreateQueryPool(
                    m_Device, &createInfo, nullptr, &m_PerformanceQueryPool);
                assert(result == VK_SUCCESS);
            }
        }

        uint32 VulkanPerformanceCounters::GetCounterIndex(
            const std::string& counterName) const {
            auto it = m_CounterIndices.find(counterName);
            return it != m_CounterIndices.end() ? it->second : UINT32_MAX;
        }

        // VulkanQueryUtils实现
        namespace VulkanQueryUtils {

            VkQueryType ConvertQueryType(EQueryType type) {
                switch (type) {
                    case EQueryType::Occlusion:
                        return VK_QUERY_TYPE_OCCLUSION;
                    case EQueryType::BinaryOcclusion:
                        return VK_QUERY_TYPE_OCCLUSION;
                    case EQueryType::Timestamp:
                        return VK_QUERY_TYPE_TIMESTAMP;
                    case EQueryType::PipelineStatistics:
                        return VK_QUERY_TYPE_PIPELINE_STATISTICS;
                    default:
                        assert(false && "Unsupported query type");
                        return VK_QUERY_TYPE_OCCLUSION;
                }
            }

            VkQueryPipelineStatisticFlags ConvertPipelineStatisticFlags(
                EPipelineStatisticFlags flags) {
                VkQueryPipelineStatisticFlags result = 0;

                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::InputAssemblyVertices))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::InputAssemblyPrimitives))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::VertexShaderInvocations))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::GeometryShaderInvocations))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::GeometryShaderPrimitives))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::ClippingInvocations))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::ClippingPrimitives))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::PixelShaderInvocations))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EPipelineStatisticFlags::ComputeShaderInvocations))
                    result |=
                        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

                return result;
            }

            void ConvertPipelineStatistics(const void* src,
                                           PipelineStatistics& dst) {
                const uint64* data = static_cast<const uint64*>(src);

                dst.InputAssemblyVertices = data[0];
                dst.InputAssemblyPrimitives = data[1];
                dst.VertexShaderInvocations = data[2];
                dst.GeometryShaderInvocations = data[3];
                dst.GeometryShaderPrimitives = data[4];
                dst.ClippingInvocations = data[5];
                dst.ClippingPrimitives = data[6];
                dst.PixelShaderInvocations = data[7];
                dst.ComputeShaderInvocations = data[8];
            }

        }  // namespace VulkanQueryUtils

    }  // namespace RHI
}  // namespace Engine
