
#pragma once

#include "Core/Public/CoreTypes.h"

namespace Engine {
    namespace RHI {

        // 命令列表类型
        enum class ECommandListType : uint8 {
            Direct,   // 直接命令列表，支持所有类型的命令
            Bundle,   // 捆绑命令列表，用于预录制常用命令序列
            Compute,  // 计算命令列表，仅支持计算相关命令
            Copy      // 复制命令列表，仅支持资源复制命令
        };

        // RHI Feature Level - 定义硬件能力级别
        enum class ERHIFeatureLevel : uint8 {
            ES2,    // OpenGL ES 2.0 级别特性
            ES3_1,  // OpenGL ES 3.1 级别特性
            SM5,    // Shader Model 5.0 级别特性
            SM6,    // Shader Model 6.0 级别特性
        };

        // 着色器类型
        enum class EShaderType : uint8 {
            Vertex,
            Pixel,
            Geometry,
            Compute,
            Hull,
            Domain,
        };

        // 资源状态
        enum class ERHIResourceState : uint32 {
            Undefined = 0,
            Common = 1 << 0,
            VertexBuffer = 1 << 1,
            ConstantBuffer = 1 << 2,
            IndexBuffer = 1 << 3,
            RenderTarget = 1 << 4,
            UnorderedAccess = 1 << 5,
            DepthWrite = 1 << 6,
            DepthRead = 1 << 7,
            ShaderResource = 1 << 8,
            StreamOut = 1 << 9,
            IndirectArgument = 1 << 10,
            CopyDest = 1 << 11,
            CopySource = 1 << 12,
            ResolveDest = 1 << 13,
            ResolveSource = 1 << 14,
            Present = 1 << 15,
            GenericRead = (1 << 16) - 1,
        };

        // 资源访问权限
        enum class ERHIAccessFlags : uint32 {
            None = 0,
            GPURead = 1 << 0,
            GPUWrite = 1 << 1,
            CPURead = 1 << 2,
            CPUWrite = 1 << 3,
        };

        // 资源使用标志
        enum class ERHIResourceFlags : uint32 {
            None = 0,
            AllowRenderTarget = 1 << 0,
            AllowDepthStencil = 1 << 1,
            AllowUnorderedAccess = 1 << 2,
            DenyShaderResource = 1 << 3,
            AllowCrossAdapter = 1 << 4,
            AllowSimultaneousAccess = 1 << 5,
        };

        // 像素格式
        enum class EPixelFormat : uint8 {
            Unknown,
            R8G8B8A8_UNORM,
            B8G8R8A8_UNORM,
            R32G32B32A32_FLOAT,
            R32G32B32_FLOAT,
            R32G32_FLOAT,
            R32_FLOAT,
            R16_UINT,
            D24_UNORM_S8_UINT,
            D32_FLOAT,
            D32_FLOAT_S8X24_UINT,
        };

        // 图元类型
        enum class EPrimitiveType : uint8 {
            Points,
            Lines,
            LineStrip,
            Triangles,
            TriangleStrip,
        };

        // 比较函数
        enum class ECompareFunction : uint8 {
            Never,
            Less,
            Equal,
            LessEqual,
            Greater,
            NotEqual,
            GreaterEqual,
            Always,
        };

        // 混合因子
        enum class EBlendFactor : uint8 {
            Zero,
            One,
            SrcColor,
            InvSrcColor,
            SrcAlpha,
            InvSrcAlpha,
            DestAlpha,
            InvDestAlpha,
            DestColor,
            InvDestColor,
            SrcAlphaSat,
            BlendFactor,
            InvBlendFactor,
        };

        // 混合操作
        enum class EBlendOp : uint8 {
            Add,
            Subtract,
            RevSubtract,
            Min,
            Max,
        };

        // 裁剪模式
        enum class ECullMode : uint8 {
            None,
            Front,
            Back,
        };

        // 填充模式
        enum class EFillMode : uint8 {
            Wireframe,
            Solid,
        };

        // 资源维度
        enum class ERHIResourceDimension : uint8 {
            Unknown,
            Buffer,
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
        };

        // 采样器过滤模式
        enum class ESamplerFilter : uint8 {
            MinMagMipPoint,
            MinMagPointMipLinear,
            MinPointMagLinearMipPoint,
            MinPointMagMipLinear,
            MinLinearMagMipPoint,
            MinLinearMagPointMipLinear,
            MinMagLinearMipPoint,
            MinMagMipLinear,
            Anisotropic,
        };

        // 采样器寻址模式
        enum class ESamplerAddressMode : uint8 {
            Wrap,
            Mirror,
            Clamp,
            Border,
            MirrorOnce,
        };

        // 顶点元素格式
        enum class EVertexElementFormat : uint8 {
            Float1,
            Float2,
            Float3,
            Float4,
            UByte4,
            UByte4N,
            Short2,
            Short4,
            Short2N,
            Short4N,
            UShort2N,
            UShort4N,
            Int1,
            Int2,
            Int3,
            Int4,
        };

        // 顶点元素用途
        enum class EVertexElementUsage : uint8 {
            Position,
            Normal,
            Tangent,
            Binormal,
            Color,
            TexCoord0,
            TexCoord1,
            TexCoord2,
            TexCoord3,
            BlendWeight,
            BlendIndices,
        };

        struct VertexElement {
            uint32 StreamIndex;
            uint32 Offset;
            EVertexElementFormat Format;
            EVertexElementUsage Usage;
            uint8 UsageIndex;
        };

        // RHI统计信息
        struct RHIStats {
            uint64 DrawCalls;
            uint64 PrimitivesDrawn;
            uint64 VerticesDrawn;
            uint64 DynamicVBSize;
            uint64 DynamicIBSize;
            uint64 UploadHeapSize;
            uint64 VirtualTextureSize;
            uint64 PhysicalTextureSize;
        };

        // 光栅化状态描述
        struct RasterizerDesc {
            EFillMode FillMode = EFillMode::Solid;
            ECullMode CullMode = ECullMode::Back;
            bool FrontCounterClockwise = false;
            int32 DepthBias = 0;
            float DepthBiasClamp = 0.0f;
            float SlopeScaledDepthBias = 0.0f;
            bool DepthClipEnable = true;
            bool MultisampleEnable = false;
            bool AntialiasedLineEnable = false;
            bool ConservativeRaster = false;
            uint32 ForcedSampleCount = 0;
        };

        // 深度模板操作
        enum class EStencilOp : uint8 {
            Keep,
            Zero,
            Replace,
            IncrSat,
            DecrSat,
            Invert,
            Incr,
            Decr
        };

        // 深度模板操作描述
        struct DepthStencilOpDesc {
            EStencilOp StencilFailOp = EStencilOp::Keep;
            EStencilOp StencilDepthFailOp = EStencilOp::Keep;
            EStencilOp StencilPassOp = EStencilOp::Keep;
            ECompareFunction StencilFunc = ECompareFunction::Always;
        };

        // 深度模板状态描述
        struct DepthStencilDesc {
            bool DepthEnable = true;
            bool DepthWriteMask = true;
            ECompareFunction DepthFunc = ECompareFunction::Less;
            bool StencilEnable = false;
            uint8 StencilReadMask = 0xFF;
            uint8 StencilWriteMask = 0xFF;
            DepthStencilOpDesc FrontFace;
            DepthStencilOpDesc BackFace;
        };

        // 渲染目标混合描述
        struct RenderTargetBlendDesc {
            bool BlendEnable = false;
            bool LogicOpEnable = false;
            EBlendFactor SrcBlend = EBlendFactor::One;
            EBlendFactor DestBlend = EBlendFactor::Zero;
            EBlendOp BlendOp = EBlendOp::Add;
            EBlendFactor SrcBlendAlpha = EBlendFactor::One;
            EBlendFactor DestBlendAlpha = EBlendFactor::Zero;
            EBlendOp BlendOpAlpha = EBlendOp::Add;
            uint8 RenderTargetWriteMask = 0x0F;
        };

        // 混合状态描述
        struct BlendDesc {
            bool AlphaToCoverageEnable = false;
            bool IndependentBlendEnable = false;
            RenderTargetBlendDesc RenderTarget[8];
        };

        // 描述符范围类型
        enum class EDescriptorRangeType : uint8 {
            SRV,     // Shader Resource View
            UAV,     // Unordered Access View
            CBV,     // Constant Buffer View
            Sampler  // Sampler
        };

        // 描述符范围
        struct DescriptorRange {
            EDescriptorRangeType RangeType;
            uint32 BaseShaderRegister;
            uint32 RegisterSpace;
            uint32 NumDescriptors;
            uint32 OffsetInDescriptorsFromTableStart;
        };

        // 静态采样器描述
        struct StaticSamplerDesc {
            uint32 ShaderRegister;
            uint32 RegisterSpace;
            ESamplerFilter Filter;
            ESamplerAddressMode AddressU;
            ESamplerAddressMode AddressV;
            ESamplerAddressMode AddressW;
            float MipLODBias;
            uint32 MaxAnisotropy;
            ECompareFunction ComparisonFunc;
            float BorderColor[4];
            float MinLOD;
            float MaxLOD;
        };

    }  // namespace RHI
}  // namespace Engine
