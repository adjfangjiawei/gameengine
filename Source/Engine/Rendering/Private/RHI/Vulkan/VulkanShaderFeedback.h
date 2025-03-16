
#pragma once

#include "RHI/RHIShaderFeedback.h"
#include "VulkanRHI.h"
#include "VulkanResources.h"

namespace Engine {
    namespace RHI {

        // Vulkan实现的着色器反馈缓冲区
        class VulkanShaderFeedbackBuffer : public IRHIShaderFeedbackBuffer {
          public:
            VulkanShaderFeedbackBuffer(VulkanDevice* InDevice,
                                       const ShaderFeedbackDesc& InDesc);
            virtual ~VulkanShaderFeedbackBuffer();

            // IRHIShaderFeedbackBuffer接口实现
            virtual const ShaderFeedbackDesc& GetDesc() const override {
                return Desc;
            }
            virtual void GetFeedbackData(void* data, uint32 size) override;
            virtual void Reset() override;

            // Vulkan特定方法
            VkBuffer GetHandle() const { return Buffer; }
            VkDeviceMemory GetMemory() const { return Memory; }

          private:
            VulkanDevice* Device;
            ShaderFeedbackDesc Desc;
            VkBuffer Buffer;
            VkDeviceMemory Memory;
            VkDeviceSize BufferSize;
        };

        // Vulkan实现的条件渲染谓词
        class VulkanPredicate : public IRHIPredicate {
          public:
            VulkanPredicate(VulkanDevice* InDevice,
                            const PredicateDesc& InDesc);
            virtual ~VulkanPredicate();

            // IRHIPredicate接口实现
            virtual const PredicateDesc& GetDesc() const override {
                return Desc;
            }
            virtual void SetValue(const PredicateDesc& value) override;

            // Vulkan特定方法
            VkBuffer GetHandle() const { return Buffer; }
            VkDeviceMemory GetMemory() const { return Memory; }

          private:
            VulkanDevice* Device;
            PredicateDesc Desc;
            VkBuffer Buffer;
            VkDeviceMemory Memory;
        };

    }  // namespace RHI
}  // namespace Engine
