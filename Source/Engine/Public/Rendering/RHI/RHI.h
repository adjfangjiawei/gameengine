
#pragma once

/**
 * RHI (Render Hardware Interface) 模块
 *
 * 这个模块提供了一个硬件无关的渲染接口，用于抽象不同图形API的实现细节。
 * 主要功能包括：
 * - 资源管理（缓冲区、纹理、着色器等）
 * - 渲染状态管理
 * - 命令列表和提交
 * - 设备和上下文管理
 */

#include "RHICommandList.h"
#include "RHIContext.h"
#include "RHIDefinitions.h"
#include "RHIDevice.h"
#include "RHIModule.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        /**
         * 初始化RHI模块
         * @param params 初始化参数
         * @return 是否初始化成功
         */
        inline bool Initialize(const RHIModuleInitParams& params) {
            return GetRHIModule().Initialize(params);
        }

        /**
         * 关闭RHI模块
         */
        inline void Shutdown() { GetRHIModule().Shutdown(); }

        /**
         * 获取RHI设备
         * @return RHI设备接口指针
         */
        inline IRHIDevice* GetDevice() { return GetRHIModule().GetDevice(); }

        /**
         * 获取RHI立即上下文
         * @return RHI上下文接口指针
         */
        inline IRHIContext* GetImmediateContext() {
            return GetRHIModule().GetImmediateContext();
        }

        /**
         * 开始新的渲染帧
         */
        inline void BeginFrame() { GetRHIModule().BeginFrame(); }

        /**
         * 结束当前渲染帧
         */
        inline void EndFrame() { GetRHIModule().EndFrame(); }

        /**
         * 获取RHI统计信息
         * @return RHI统计信息
         */
        inline const RHIStats& GetStats() { return GetRHIModule().GetStats(); }

    }  // namespace RHI
}  // namespace Engine
