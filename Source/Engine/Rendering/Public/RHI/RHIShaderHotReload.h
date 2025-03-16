
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 着色器变体标志
        enum class EShaderVariantFlags : uint32 {
            None = 0,
            Debug = 1 << 0,        // 调试变体
            Optimized = 1 << 1,    // 优化变体
            Development = 1 << 2,  // 开发变体
            Shipping = 1 << 3,     // 发布变体
        };

        // 着色器变体描述
        struct ShaderVariantDesc {
            std::string Name;
            EShaderVariantFlags Flags;
            std::vector<std::string> Defines;
            std::string EntryPoint;
            std::string DebugName;
        };

        // 着色器变体接口
        class IRHIShaderVariant : public IRHIResource {
          public:
            virtual ~IRHIShaderVariant() = default;

            // 获取描述
            virtual const ShaderVariantDesc& GetDesc() const = 0;

            // 编译变体
            virtual bool Compile() = 0;

            // 获取编译状态
            virtual bool IsCompiled() const = 0;
        };

        // 着色器热重载监听器
        class IRHIShaderHotReloadListener {
          public:
            virtual ~IRHIShaderHotReloadListener() = default;

            // 着色器重新编译通知
            virtual void OnShaderRecompiled(IRHIShader* shader) = 0;

            // 着色器编译错误通知
            virtual void OnShaderCompileError(const std::string& error) = 0;
        };

        // 着色器热重载系统
        class IRHIShaderHotReload {
          public:
            virtual ~IRHIShaderHotReload() = default;

            // 注册监听器
            virtual void RegisterListener(
                IRHIShaderHotReloadListener* listener) = 0;

            // 注销监听器
            virtual void UnregisterListener(
                IRHIShaderHotReloadListener* listener) = 0;

            // 启动监视
            virtual void StartWatching(const std::string& directory) = 0;

            // 停止监视
            virtual void StopWatching() = 0;

            // 手动触发重新编译
            virtual void TriggerRecompile(const std::string& shaderPath) = 0;
        };

        // 资源虚拟化标志
        enum class EResourceVirtualizationFlags : uint32 {
            None = 0,
            Streaming = 1 << 0,        // 支持流式传输
            Residency = 1 << 1,        // 支持驻留管理
            Defragmentation = 1 << 2,  // 支持碎片整理
        };

        // 资源虚拟化描述
        struct ResourceVirtualizationDesc {
            EResourceVirtualizationFlags Flags;
            uint64 VirtualSize;
            uint64 PhysicalSize;
            std::string DebugName;
        };

        // 资源虚拟化接口
        class IRHIResourceVirtualization {
          public:
            virtual ~IRHIResourceVirtualization() = default;

            // 获取描述
            virtual const ResourceVirtualizationDesc& GetDesc() const = 0;

            // 资源流式传输
            virtual void StreamIn(uint64 offset, uint64 size) = 0;
            virtual void StreamOut(uint64 offset, uint64 size) = 0;

            // 驻留管理
            virtual void MakeResident(uint64 offset, uint64 size) = 0;
            virtual void Evict(uint64 offset, uint64 size) = 0;

            // 碎片整理
            virtual void Defragment() = 0;

            // 获取状态
            virtual float GetResidencyPercentage() const = 0;
            virtual uint64 GetPhysicalUsage() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
