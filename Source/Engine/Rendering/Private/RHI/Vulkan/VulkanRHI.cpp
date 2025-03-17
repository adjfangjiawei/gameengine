
#include "VulkanRHI.h"

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        VulkanRHI& VulkanRHI::Get() {
            static VulkanRHI instance;
            return instance;
        }

        bool VulkanRHI::Initialize(const RHIModuleInitParams& params) {
            // 初始化Vulkan实例
            if (!Instance.Initialize(params)) {
                LOG_ERROR("Failed to initialize Vulkan instance!");
                return false;
            }

            // 初始化物理设备
            if (!PhysicalDevice.Initialize(Instance.GetHandle())) {
                LOG_ERROR("Failed to initialize Vulkan physical device!");
                return false;
            }

            // 初始化逻辑设备
            if (!Device.Initialize(&PhysicalDevice)) {
                LOG_ERROR("Failed to initialize Vulkan device!");
                return false;
            }

            // 初始化命令池管理器
            if (!CommandPoolManager.Initialize(&Device)) {
                LOG_ERROR("Failed to initialize Vulkan command pool manager!");
                return false;
            }

            // 初始化内存分配器
            if (!GetMemoryAllocator()->Initialize(Device.GetHandle())) {
                LOG_ERROR("Failed to initialize Vulkan memory allocator!");
                return false;
            }

            return true;
        }

        void VulkanRHI::Shutdown() {
            // 按照依赖关系的相反顺序关闭各个组件
            GetMemoryAllocator()->Shutdown();
            CommandPoolManager.Shutdown();
            Device.Shutdown();
            // PhysicalDevice不需要显式关闭
            Instance.Shutdown();
        }

    }  // namespace RHI
}  // namespace Engine
