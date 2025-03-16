
#pragma once

#include "RHIDefinitions.h"
#include "RHIMemory.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 稀疏资源块大小
        struct SparseBlockSize {
            uint32 Width;
            uint32 Height;
            uint32 Depth;
        };

        // 稀疏资源标志
        enum class ESparseResourceFlags : uint32 {
            None = 0,
            AllowAliasing = 1 << 0,
            NonResident = 1 << 1,
        };

        // 稀疏资源区域
        struct SparseRegion {
            uint32 MipLevel;
            uint32 ArrayLayer;
            uint32 OffsetX;
            uint32 OffsetY;
            uint32 OffsetZ;
            uint32 ExtentWidth;
            uint32 ExtentHeight;
            uint32 ExtentDepth;
        };

        // 稀疏资源绑定
        struct SparseBinding {
            IRHIHeap* Memory;
            uint64 MemoryOffset;
            SparseRegion Region;
        };

        // 稀疏资源描述
        struct SparseResourceDesc {
            ESparseResourceFlags Flags;
            SparseBlockSize BlockSize;
            uint32 MipLevels;
            uint32 ArraySize;
            std::string DebugName;
        };

        // 稀疏资源接口
        class IRHISparseResource : public IRHIResource {
          public:
            virtual ~IRHISparseResource() = default;

            // 获取描述
            virtual const SparseResourceDesc& GetDesc() const = 0;

            // 获取块信息
            virtual void GetRequiredBlocks(const SparseRegion& region,
                                           uint32& numBlocks,
                                           uint64& memorySize) const = 0;

            // 绑定内存
            virtual void BindBlocks(uint32 numBindings,
                                    const SparseBinding* bindings) = 0;
        };

        // 稀疏纹理接口
        class IRHISparseTexture : public IRHISparseResource {
          public:
            // 获取纹理格式
            virtual EPixelFormat GetFormat() const = 0;

            // 获取维度
            virtual ERHIResourceDimension GetDimension() const = 0;
        };

        // 稀疏缓冲区接口
        class IRHISparseBuffer : public IRHISparseResource {
          public:
            // 获取缓冲区大小
            virtual uint64 GetSize() const = 0;

            // 获取步长
            virtual uint32 GetStride() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
