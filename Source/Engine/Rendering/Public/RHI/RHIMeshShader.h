
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 网格着色器管线状态描述
        struct MeshPipelineStateDesc {
            // 着色器阶段
            IRHIShader* MeshShader = nullptr;
            IRHIShader* PixelShader = nullptr;
            IRHIShader* AmplificationShader = nullptr;

            // 光栅化状态
            RasterizerDesc RasterizerState;

            // 深度模板状态
            DepthStencilDesc DepthStencilState;

            // 混合状态
            BlendDesc BlendState;

            // 渲染目标格式
            std::vector<EPixelFormat> RenderTargetFormats;
            EPixelFormat DepthStencilFormat = EPixelFormat::Unknown;

            // 多重采样
            uint32 SampleCount = 1;
            uint32 SampleQuality = 0;

            std::string DebugName;
        };

        // 网格着色器管线状态接口
        class IRHIMeshPipelineState : public IRHIResource {
          public:
            virtual ~IRHIMeshPipelineState() = default;

            // 获取描述
            virtual const MeshPipelineStateDesc& GetDesc() const = 0;
        };

        // 网格任务参数
        struct MeshTaskParameters {
            uint32 ThreadGroupCountX;
            uint32 ThreadGroupCountY;
            uint32 ThreadGroupCountZ;
        };

        // 放大因子
        struct AmplificationFactor {
            uint32 X;
            uint32 Y;
            uint32 Z;
        };

    }  // namespace RHI
}  // namespace Engine
