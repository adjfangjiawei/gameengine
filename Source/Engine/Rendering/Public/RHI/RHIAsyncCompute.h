
#pragma once

#include "RHICommands.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 异步计算队列优先级
        enum class EComputeQueuePriority : uint8 { Normal, High, Realtime };

        // 异步计算队列描述
        struct AsyncComputeQueueDesc {
            EComputeQueuePriority Priority = EComputeQueuePriority::Normal;
            std::string DebugName;
        };

        // 异步计算队列接口
        class IRHIAsyncComputeQueue {
          public:
            virtual ~IRHIAsyncComputeQueue() = default;

            // 提交命令列表
            virtual void ExecuteCommandLists(
                uint32 count, IRHICommandList* const* commandLists) = 0;

            // 同步原语
            virtual void Signal(IRHIFence* fence, uint64 value) = 0;
            virtual void Wait(IRHIFence* fence, uint64 value) = 0;

            // 获取描述
            virtual const AsyncComputeQueueDesc& GetDesc() const = 0;
        };

        // 间接命令签名标志
        enum class EIndirectCommandFlags : uint32 {
            None = 0,
            DrawIndexed = 1 << 0,
            Draw = 1 << 1,
            Dispatch = 1 << 2,
            VertexBufferView = 1 << 3,
            IndexBufferView = 1 << 4,
            ConstantBufferView = 1 << 5,
            ShaderResourceView = 1 << 6,
            UnorderedAccessView = 1 << 7,
        };

        // 间接命令签名描述
        struct IndirectCommandSignatureDesc {
            EIndirectCommandFlags Flags;
            uint32 ByteStride;
            std::string DebugName;
        };

        // 间接命令签名接口
        class IRHIIndirectCommandSignature : public IRHIResource {
          public:
            virtual ~IRHIIndirectCommandSignature() = default;

            // 获取描述
            virtual const IndirectCommandSignatureDesc& GetDesc() const = 0;
        };

        // 间接绘制参数
        struct IndirectDrawArgs {
            uint32 VertexCountPerInstance;
            uint32 InstanceCount;
            uint32 StartVertexLocation;
            uint32 StartInstanceLocation;
        };

        // 间接索引绘制参数
        struct IndirectDrawIndexedArgs {
            uint32 IndexCountPerInstance;
            uint32 InstanceCount;
            uint32 StartIndexLocation;
            int32 BaseVertexLocation;
            uint32 StartInstanceLocation;
        };

        // 间接调度参数
        struct IndirectDispatchArgs {
            uint32 ThreadGroupCountX;
            uint32 ThreadGroupCountY;
            uint32 ThreadGroupCountZ;
        };

        // 命令生成器接口
        class IRHICommandGenerator {
          public:
            virtual ~IRHICommandGenerator() = default;

            // 生成间接命令
            virtual void GenerateCommands(IRHICommandList* commandList,
                                          IRHIBuffer* inputBuffer,
                                          IRHIBuffer* outputBuffer,
                                          uint32 maxCommands) = 0;
        };

    }  // namespace RHI
}  // namespace Engine
