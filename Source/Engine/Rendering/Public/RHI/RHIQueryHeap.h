
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 查询类型
        enum class EQueryType : uint8 {
            Occlusion,            // 遮挡查询
            BinaryOcclusion,      // 二进制遮挡查询
            Timestamp,            // 时间戳查询
            PipelineStatistics,   // 管线统计查询
            StreamOutStatistics,  // 流输出统计查询
        };

        // 查询堆描述
        struct QueryHeapDesc {
            EQueryType Type;
            uint32 Count;
            std::string DebugName;
        };

        // 查询堆接口
        class IRHIQueryHeap : public IRHIResource {
          public:
            virtual ~IRHIQueryHeap() = default;

            // 获取描述
            virtual const QueryHeapDesc& GetDesc() const = 0;

            // 获取结果
            virtual void GetResults(uint32 startIndex,
                                    uint32 numQueries,
                                    void* data,
                                    uint32 stride) = 0;
        };

        // 管线统计标志
        enum class EPipelineStatisticFlags : uint32 {
            None = 0,
            InputAssemblyVertices = 1 << 0,
            InputAssemblyPrimitives = 1 << 1,
            VertexShaderInvocations = 1 << 2,
            GeometryShaderInvocations = 1 << 3,
            GeometryShaderPrimitives = 1 << 4,
            ClippingInvocations = 1 << 5,
            ClippingPrimitives = 1 << 6,
            PixelShaderInvocations = 1 << 7,
            ComputeShaderInvocations = 1 << 8,
        };

        // 管线统计数据
        struct PipelineStatistics {
            uint64 InputAssemblyVertices;
            uint64 InputAssemblyPrimitives;
            uint64 VertexShaderInvocations;
            uint64 GeometryShaderInvocations;
            uint64 GeometryShaderPrimitives;
            uint64 ClippingInvocations;
            uint64 ClippingPrimitives;
            uint64 PixelShaderInvocations;
            uint64 ComputeShaderInvocations;
        };

        // 时间戳查询接口扩展
        class IRHITimestampQuery {
          public:
            virtual ~IRHITimestampQuery() = default;

            // 获取GPU时间戳频率
            virtual uint64 GetTimestampFrequency() const = 0;

            // 校准CPU和GPU时间戳
            virtual void CalibrateGPUTimestamp(uint64& gpuTimestamp,
                                               uint64& cpuTimestamp) = 0;
        };

        // 性能计数器类型
        enum class EPerformanceCounterType : uint8 {
            Generic,       // 通用计数器
            RenderTarget,  // 渲染目标相关
            Compute,       // 计算相关
            Memory,        // 内存相关
        };

        // 性能计数器描述
        struct PerformanceCounterDesc {
            std::string Name;
            EPerformanceCounterType Type;
            std::string Description;
            std::string Units;
        };

        // 性能计数器接口
        class IRHIPerformanceCounters {
          public:
            virtual ~IRHIPerformanceCounters() = default;

            // 获取可用计数器
            virtual std::vector<PerformanceCounterDesc> GetAvailableCounters()
                const = 0;

            // 开始采样
            virtual void BeginSampling(
                const std::vector<std::string>& counterNames) = 0;

            // 结束采样
            virtual void EndSampling() = 0;

            // 获取计数器值
            virtual double GetCounterValue(
                const std::string& counterName) const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
