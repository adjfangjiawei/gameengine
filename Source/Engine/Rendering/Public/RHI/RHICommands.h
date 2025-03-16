#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 命令列表描述
        struct CommandListDesc {
            ECommandListType Type = ECommandListType::Direct;
            std::string DebugName;
        };

        // 基础命令列表接口
        class IRHICommandListBase : public IRHIResource {
          public:
            virtual ~IRHICommandListBase() = default;

            // 获取描述
            virtual const CommandListDesc& GetDesc() const = 0;

            // 重置命令列表
            virtual void Reset() = 0;

            // 关闭命令列表
            virtual void Close() = 0;
        };

        // 围栏类型
        enum class EFenceType : uint8 {
            Default,  // 默认类型
            Shared,   // 跨进程共享
            Timeline  // 时间线类型
        };

        // 围栏描述
        struct FenceDesc {
            EFenceType Type = EFenceType::Default;
            uint64 InitialValue = 0;
            std::string DebugName;
        };

        // 围栏接口
        class IRHIFence : public IRHIResource {
          public:
            virtual ~IRHIFence() = default;

            // 获取描述
            virtual const FenceDesc& GetDesc() const = 0;

            // 获取当前值
            virtual uint64 GetCompletedValue() const = 0;

            // 等待特定值
            virtual void Wait(uint64 value) = 0;

            // 设置值
            virtual void Signal(uint64 value) = 0;
        };

    }  // namespace RHI
}  // namespace Engine
