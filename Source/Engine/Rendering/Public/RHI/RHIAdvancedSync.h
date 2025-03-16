
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 共享句柄标志
        enum class ESharedHandleFlags : uint32 {
            None = 0,
            Shared = 1 << 0,        // 进程间共享
            CrossAdapter = 1 << 1,  // 跨适配器共享
            GlobalShare = 1 << 2,   // 全局共享
        };

        // 共享资源描述
        struct SharedResourceDesc {
            ESharedHandleFlags Flags;
            void* SharedHandle;  // 平台相关的共享句柄
            std::string DebugName;
        };

        // 共享资源接口
        class IRHISharedResource : public IRHIResource {
          public:
            virtual ~IRHISharedResource() = default;

            // 获取描述
            virtual const SharedResourceDesc& GetDesc() const = 0;

            // 获取共享句柄
            virtual void* GetSharedHandle() = 0;
        };

        // 多GPU同步模式
        enum class EMultiGPUSyncMode : uint8 {
            Broadcast,     // 广播同步
            RoundRobin,    // 轮询同步
            LoadBalanced,  // 负载均衡
            Custom,        // 自定义同步
        };

        // 多GPU同步描述
        struct MultiGPUSyncDesc {
            EMultiGPUSyncMode Mode;
            uint32 GPUMask;  // GPU掩码
            std::string DebugName;
        };

        // 多GPU同步接口
        class IRHIMultiGPUSync {
          public:
            virtual ~IRHIMultiGPUSync() = default;

            // 获取描述
            virtual const MultiGPUSyncDesc& GetDesc() const = 0;

            // 同步操作
            virtual void SyncPoint(uint32 gpuIndex) = 0;
            virtual void WaitForGPU(uint32 gpuIndex) = 0;
        };

        // 细粒度同步范围
        struct SyncRange {
            uint64 Begin;
            uint64 End;
        };

        // 细粒度同步标志
        enum class EFineSyncFlags : uint32 {
            None = 0,
            GPUOnly = 1 << 0,     // 仅GPU同步
            CPUVisible = 1 << 1,  // CPU可见
            Persistent = 1 << 2,  // 持久同步点
        };

        // 细粒度同步描述
        struct FineSyncDesc {
            EFineSyncFlags Flags;
            uint32 MaxSyncPoints;
            std::string DebugName;
        };

        // 细粒度同步接口
        class IRHIFineSync {
          public:
            virtual ~IRHIFineSync() = default;

            // 获取描述
            virtual const FineSyncDesc& GetDesc() const = 0;

            // 同步操作
            virtual void InsertSyncPoint(const SyncRange& range) = 0;
            virtual void WaitSyncPoint(uint64 point) = 0;
            virtual bool IsSyncPointComplete(uint64 point) const = 0;
        };

        // 跨进程同步描述
        struct CrossProcessSyncDesc {
            std::string Name;  // 全局唯一名称
            bool CreateIfNotExist;
            std::string DebugName;
        };

        // 跨进程同步接口
        class IRHICrossProcessSync {
          public:
            virtual ~IRHICrossProcessSync() = default;

            // 获取描述
            virtual const CrossProcessSyncDesc& GetDesc() const = 0;

            // 同步操作
            virtual void Signal(uint64 value) = 0;
            virtual void Wait(uint64 value) = 0;
            virtual uint64 GetCurrentValue() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
