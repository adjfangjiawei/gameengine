#pragma once

#include "RHICommands.h"
#include "RHIDefinitions.h"

namespace Engine {
    namespace RHI {

        // 事件接口
        class IRHIEvent {
          public:
            virtual ~IRHIEvent() = default;

            // 设置事件
            virtual void Set() = 0;

            // 重置事件
            virtual void Reset() = 0;

            // 等待事件
            virtual void Wait() = 0;
        };

        // 时间线信号量描述
        struct TimelineSemaphoreDesc {
            uint64 InitialValue = 0;
            std::string DebugName;
        };

        // 时间线信号量接口
        class IRHITimelineSemaphore {
          public:
            virtual ~IRHITimelineSemaphore() = default;

            // 获取当前值
            virtual uint64 GetCurrentValue() const = 0;

            // 等待特定值
            virtual void Wait(uint64 value) = 0;

            // 信号特定值
            virtual void Signal(uint64 value) = 0;
        };

        // 同步点
        struct SyncPoint {
            IRHIFence* Fence = nullptr;
            uint64 Value = 0;
        };

    }  // namespace RHI
}  // namespace Engine
