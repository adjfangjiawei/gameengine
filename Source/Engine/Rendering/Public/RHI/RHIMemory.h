
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 内存类型
        enum class EMemoryType : uint8 {
            Default,   // GPU本地内存
            Upload,    // CPU可写GPU可读的上传堆
            Readback,  // GPU可写CPU可读的回读堆
            Custom,    // 自定义内存类型
        };

        // 内存属性
        struct MemoryProperties {
            bool DeviceLocal = false;   // GPU本地内存
            bool HostVisible = false;   // CPU可访问
            bool HostCoherent = false;  // CPU访问无需显式刷新
            bool HostCached = false;    // CPU访问已缓存
        };

        // 堆类型
        enum class EHeapType : uint8 {
            Default,   // 默认堆
            Upload,    // 上传堆
            Readback,  // 回读堆
            Custom,    // 自定义堆
        };

        // 堆属性
        struct HeapProperties {
            EHeapType Type = EHeapType::Default;
            bool CPUPageable = false;
            MemoryProperties MemoryProps;
        };

        // 堆描述
        struct HeapDesc {
            uint64 SizeInBytes = 0;
            HeapProperties Properties;
            std::string DebugName;
        };

        // 内存堆接口
        class IRHIHeap : public IRHIResource {
          public:
            virtual ~IRHIHeap() = default;

            // 获取堆描述
            virtual const HeapDesc& GetDesc() const = 0;

            // 获取堆大小
            virtual uint64 GetSize() const = 0;

            // 映射内存
            virtual void* Map() = 0;

            // 解除映射
            virtual void Unmap() = 0;
        };

        // 内存分配器接口
        class IRHIMemoryAllocator {
          public:
            virtual ~IRHIMemoryAllocator() = default;

            // 创建堆
            virtual IRHIHeap* CreateHeap(const HeapDesc& desc) = 0;

            // 分配内存
            virtual void* Allocate(uint64 size, uint64 alignment) = 0;

            // 释放内存
            virtual void Free(void* ptr) = 0;

            // 获取内存统计信息
            virtual void GetMemoryStats(uint64& totalSize,
                                        uint64& usedSize) const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
