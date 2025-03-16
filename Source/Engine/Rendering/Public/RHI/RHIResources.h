#pragma once

#include <string>
#include <vector>

#include "RHIDefinitions.h"

namespace Engine {
    namespace RHI {

        // 基础资源接口
        class IRHIResource {
          public:
            virtual ~IRHIResource() = default;

            // 获取资源名称
            virtual const std::string& GetName() const = 0;

            // 获取资源当前状态
            virtual ERHIResourceState GetCurrentState() const = 0;

            // 获取资源维度
            virtual ERHIResourceDimension GetResourceDimension() const = 0;

            // 获取资源大小（字节）
            virtual uint64 GetSize() const = 0;
        };

        // 缓冲区描述
        struct BufferDesc {
            uint64 SizeInBytes = 0;
            uint32 Stride = 0;
            ERHIResourceFlags Flags = ERHIResourceFlags::None;
            ERHIAccessFlags Access = ERHIAccessFlags::None;
            ERHIResourceState InitialState = ERHIResourceState::Common;
            std::string DebugName;
        };

        // 缓冲区接口
        class IRHIBuffer : public IRHIResource {
          public:
            // 获取缓冲区描述
            virtual const BufferDesc& GetDesc() const = 0;

            // 映射缓冲区内存
            virtual void* Map(uint32 subresource = 0) = 0;

            // 解除缓冲区内存映射
            virtual void Unmap(uint32 subresource = 0) = 0;

            // 获取GPU虚拟地址
            virtual uint64 GetGPUVirtualAddress() const = 0;
        };

        // 纹理描述
        struct TextureDesc {
            uint32 Width = 1;
            uint32 Height = 1;
            uint32 Depth = 1;
            uint32 MipLevels = 1;
            uint32 ArraySize = 1;
            EPixelFormat Format = EPixelFormat::Unknown;
            ERHIResourceFlags Flags = ERHIResourceFlags::None;
            ERHIResourceState InitialState = ERHIResourceState::Common;
            ERHIResourceDimension Dimension = ERHIResourceDimension::Texture2D;
            uint32 SampleCount = 1;
            uint32 SampleQuality = 0;
            std::string DebugName;
        };

        // 纹理接口
        class IRHITexture : public IRHIResource {
          public:
            // 获取纹理描述
            virtual const TextureDesc& GetDesc() const = 0;

            // 获取纹理格式
            virtual EPixelFormat GetFormat() const = 0;

            // 获取纹理宽度
            virtual uint32 GetWidth() const = 0;

            // 获取纹理高度
            virtual uint32 GetHeight() const = 0;

            // 获取纹理深度
            virtual uint32 GetDepth() const = 0;

            // 获取Mip等级数量
            virtual uint32 GetMipLevels() const = 0;

            // 获取数组大小
            virtual uint32 GetArraySize() const = 0;
        };

        // 着色器描述
        struct ShaderDesc {
            EShaderType Type;
            std::string EntryPoint;
            std::string DebugName;
        };

        // 着色器接口
        class IRHIShader : public IRHIResource {
          public:
            // 获取着色器描述
            virtual const ShaderDesc& GetDesc() const = 0;

            // 获取着色器类型
            virtual EShaderType GetType() const = 0;

            // 获取入口点名称
            virtual const std::string& GetEntryPoint() const = 0;
        };

        // 资源视图描述基类
        struct ViewDesc {
            std::string DebugName;
        };

        // 渲染目标视图描述
        struct RenderTargetViewDesc : public ViewDesc {
            EPixelFormat Format = EPixelFormat::Unknown;
            uint32 MipSlice = 0;
            uint32 FirstArraySlice = 0;
            uint32 ArraySize = 1;
        };

        // 深度模板视图描述
        struct DepthStencilViewDesc : public ViewDesc {
            EPixelFormat Format = EPixelFormat::Unknown;
            uint32 MipSlice = 0;
            uint32 FirstArraySlice = 0;
            uint32 ArraySize = 1;
        };

        // 着色器资源视图描述
        struct ShaderResourceViewDesc : public ViewDesc {
            EPixelFormat Format = EPixelFormat::Unknown;
            uint32 MostDetailedMip = 0;
            uint32 MipLevels = -1;
            uint32 FirstArraySlice = 0;
            uint32 ArraySize = -1;
        };

        // 无序访问视图描述
        struct UnorderedAccessViewDesc : public ViewDesc {
            EPixelFormat Format = EPixelFormat::Unknown;
            uint32 MipSlice = 0;
            uint32 FirstArraySlice = 0;
            uint32 ArraySize = 1;
            // 添加缓冲区视图所需的成员
            uint32 FirstElement = 0;  // 缓冲区视图的第一个元素索引
            uint32 NumElements = 0;   // 缓冲区视图的元素数量
            uint32 ElementSize = 0;   // 每个元素的大小（字节）
        };

        // 资源视图接口基类
        class IRHIResourceView : public IRHIResource {
          public:
            // 获取关联的资源
            virtual IRHIResource* GetResource() const = 0;
        };

        // 渲染目标视图接口
        class IRHIRenderTargetView : public IRHIResourceView {
          public:
            virtual const RenderTargetViewDesc& GetDesc() const = 0;
        };

        // 深度模板视图接口
        class IRHIDepthStencilView : public IRHIResourceView {
          public:
            virtual const DepthStencilViewDesc& GetDesc() const = 0;
        };

        // 着色器资源视图接口
        class IRHIShaderResourceView : public IRHIResourceView {
          public:
            virtual const ShaderResourceViewDesc& GetDesc() const = 0;
        };

        // 无序访问视图接口
        class IRHIUnorderedAccessView : public IRHIResourceView {
          public:
            virtual const UnorderedAccessViewDesc& GetDesc() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
