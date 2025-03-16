
#pragma once

#include <functional>

#include "RHIContext.h"

namespace Engine {
    namespace RHI {

        // RHI模块初始化参数
        struct RHIModuleInitParams {
            RHIDeviceCreateParams DeviceParams;
            bool EnableValidation = false;
            bool EnableProfiling = false;
            std::function<void(const char*)> ErrorCallback;
            std::function<void(const char*)> WarningCallback;
            std::function<void(const char*)> InfoCallback;
        };

        // RHI模块状态
        enum class ERHIModuleState {
            Uninitialized,
            Initializing,
            Ready,
            DeviceLost,
            Error,
            ShuttingDown
        };

        // RHI模块接口
        class IRHIModule {
          public:
            virtual ~IRHIModule() = default;

            // 初始化和关闭
            virtual bool Initialize(const RHIModuleInitParams& params) = 0;
            virtual void Shutdown() = 0;

            // 设备管理
            virtual IRHIDevice* GetDevice() = 0;
            virtual IRHIContext* GetImmediateContext() = 0;
            virtual bool IsDeviceLost() const = 0;
            virtual bool ResetDevice() = 0;

            // 状态查询
            virtual ERHIModuleState GetState() const = 0;
            virtual ERHIFeatureLevel GetMaxFeatureLevel() const = 0;
            virtual const RHIStats& GetStats() const = 0;

            // 调试支持
            virtual void EnableDebugLayer(bool enable) = 0;
            virtual void EnableGPUValidation(bool enable) = 0;
            virtual void EnableProfiling(bool enable) = 0;
            virtual void CaptureNextFrame() = 0;
            virtual void BeginCapture() = 0;
            virtual void EndCapture() = 0;

            // 帧管理
            virtual void BeginFrame() = 0;
            virtual void EndFrame() = 0;
            virtual uint64 GetFrameCount() const = 0;

            // 内存管理
            virtual void ReleaseUnusedResources() = 0;
            virtual uint64 GetTotalResourceMemory() const = 0;
            virtual uint64 GetUsedResourceMemory() const = 0;

            // 事件回调
            virtual void SetDeviceLostCallback(
                std::function<void()> callback) = 0;
            virtual void SetDeviceRestoredCallback(
                std::function<void()> callback) = 0;
            virtual void SetErrorCallback(
                std::function<void(const char*)> callback) = 0;

          protected:
            // 内部事件处理
            virtual void OnDeviceLost() = 0;
            virtual void OnDeviceRestored() = 0;
            virtual void OnError(const char* message) = 0;
        };

        // RHI模块实现类
        class RHIModule : public IRHIModule {
          public:
            static RHIModule& Get();

            // IRHIModule接口实现
            virtual bool Initialize(const RHIModuleInitParams& params) override;
            virtual void Shutdown() override;

            virtual IRHIDevice* GetDevice() override;
            virtual IRHIContext* GetImmediateContext() override;
            virtual bool IsDeviceLost() const override;
            virtual bool ResetDevice() override;

            virtual ERHIModuleState GetState() const override;
            virtual ERHIFeatureLevel GetMaxFeatureLevel() const override;
            virtual const RHIStats& GetStats() const override;

            virtual void EnableDebugLayer(bool enable) override;
            virtual void EnableGPUValidation(bool enable) override;
            virtual void EnableProfiling(bool enable) override;
            virtual void CaptureNextFrame() override;
            virtual void BeginCapture() override;
            virtual void EndCapture() override;

            virtual void BeginFrame() override;
            virtual void EndFrame() override;
            virtual uint64 GetFrameCount() const override;

            virtual void ReleaseUnusedResources() override;
            virtual uint64 GetTotalResourceMemory() const override;
            virtual uint64 GetUsedResourceMemory() const override;

            virtual void SetDeviceLostCallback(
                std::function<void()> callback) override;
            virtual void SetDeviceRestoredCallback(
                std::function<void()> callback) override;
            virtual void SetErrorCallback(
                std::function<void(const char*)> callback) override;

          protected:
            virtual void OnDeviceLost() override;
            virtual void OnDeviceRestored() override;
            virtual void OnError(const char* message) override;

          private:
            RHIModule() = default;
            ~RHIModule() = default;

            RHIModule(const RHIModule&) = delete;
            RHIModule& operator=(const RHIModule&) = delete;

            // 内部状态
            ERHIModuleState State = ERHIModuleState::Uninitialized;
            RHIDevicePtr Device;
            RHIContextPtr ImmediateContext;
            RHIModuleInitParams InitParams;
            RHIStats Statistics;
            uint64 FrameCount = 0;

            // 回调函数
            std::function<void()> DeviceLostCallback;
            std::function<void()> DeviceRestoredCallback;
            std::function<void(const char*)> ErrorCallback;

            // 调试标志
            bool DebugLayerEnabled = false;
            bool GPUValidationEnabled = false;
            bool ProfilingEnabled = false;
        };

        // 全局访问函数
        inline RHIModule& GetRHIModule() { return RHIModule::Get(); }

    }  // namespace RHI
}  // namespace Engine
