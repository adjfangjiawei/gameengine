
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 输入布局描述
        struct InputLayoutDesc {
            std::vector<VertexElement> Elements;
            std::string DebugName;
        };

        // 管线状态描述
        struct GraphicsPipelineStateDesc {
            // 着色器阶段
            IRHIShader* VertexShader = nullptr;
            IRHIShader* PixelShader = nullptr;
            IRHIShader* GeometryShader = nullptr;
            IRHIShader* HullShader = nullptr;
            IRHIShader* DomainShader = nullptr;

            // 输入布局
            InputLayoutDesc InputLayout;

            // 光栅化状态
            RasterizerDesc RasterizerState;

            // 深度模板状态
            DepthStencilDesc DepthStencilState;

            // 混合状态
            BlendDesc BlendState;

            // 图元拓扑
            EPrimitiveType PrimitiveTopology = EPrimitiveType::Triangles;

            // 渲染目标格式
            std::vector<EPixelFormat> RenderTargetFormats;
            EPixelFormat DepthStencilFormat = EPixelFormat::Unknown;

            // 多重采样
            uint32 SampleCount = 1;
            uint32 SampleQuality = 0;

            std::string DebugName;
        };

        struct ComputePipelineStateDesc {
            IRHIShader* ComputeShader = nullptr;
            std::string DebugName;
        };

        // 管线状态对象接口
        class IRHIPipelineState : public IRHIResource {
          public:
            virtual ~IRHIPipelineState() = default;

            // 获取管线类型
            virtual bool IsCompute() const = 0;

            // 获取描述
            virtual const GraphicsPipelineStateDesc& GetGraphicsDesc()
                const = 0;
            virtual const ComputePipelineStateDesc& GetComputeDesc() const = 0;
        };

        // 根参数类型
        enum class ERootParameterType : uint8 {
            DescriptorTable,  // 描述符表
            Constants,        // 根常量
            CBV,              // 根常量缓冲区视图
            SRV,              // 根着色器资源视图
            UAV,              // 根无序访问视图
        };

        // 根参数
        struct RootParameter {
            ERootParameterType ParameterType;
            union {
                struct {
                    std::vector<DescriptorRange> Ranges;
                } DescriptorTable;
                struct {
                    uint32 ShaderRegister;
                    uint32 RegisterSpace;
                    uint32 Num32BitValues;
                } Constants;
                struct {
                    uint32 ShaderRegister;
                    uint32 RegisterSpace;
                } Descriptor;
            };
        };

        // 根签名描述
        struct RootSignatureDesc {
            std::vector<RootParameter> Parameters;
            std::vector<StaticSamplerDesc> StaticSamplers;
            std::string DebugName;
        };

        // 根签名接口
        class IRHIRootSignature : public IRHIResource {
          public:
            virtual ~IRHIRootSignature() = default;

            // 获取描述
            virtual const RootSignatureDesc& GetDesc() const = 0;
        };

        // 描述符堆类型
        enum class EDescriptorHeapType : uint8 {
            CBV_SRV_UAV,  // 常量缓冲区、着色器资源、无序访问视图
            Sampler,      // 采样器
            RTV,          // 渲染目标视图
            DSV,          // 深度模板视图
        };

        // 描述符堆描述
        struct DescriptorHeapDesc {
            EDescriptorHeapType Type;
            uint32 NumDescriptors;
            bool ShaderVisible;
            std::string DebugName;
        };

        // 描述符堆接口
        class IRHIDescriptorHeap : public IRHIResource {
          public:
            virtual ~IRHIDescriptorHeap() = default;

            // 获取描述
            virtual const DescriptorHeapDesc& GetDesc() const = 0;

            // 获取CPU句柄
            virtual uint64 GetCPUDescriptorHandleForHeapStart() const = 0;

            // 获取GPU句柄（仅对ShaderVisible的堆有效）
            virtual uint64 GetGPUDescriptorHandleForHeapStart() const = 0;

            // 获取描述符大小
            virtual uint32 GetDescriptorSize() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
