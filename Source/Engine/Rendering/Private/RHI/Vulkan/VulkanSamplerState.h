
#pragma once

#include "RHI/RHIDevice.h"
#include "VulkanResources.h"

namespace Engine {
    namespace RHI {
        class VulkanSamplerState : public VulkanResource,
                                   public IRHISamplerState {
          public:
            VulkanSamplerState(VulkanDevice* device, const SamplerDesc& desc);
            virtual ~VulkanSamplerState();

            // IRHISamplerState interface
            virtual const SamplerDesc& GetDesc() const override { return Desc; }

            // IRHIResource interface
            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return ERHIResourceDimension::Unknown;
            }
            virtual uint64 GetSize() const override { return 0; }

            // Vulkan specific
            VkSampler GetHandle() const { return Sampler; }
            void SetHandle(VkSampler sampler) { Sampler = sampler; }

          private:
            bool CreateSampler();

            SamplerDesc Desc;
            VkSampler Sampler;
            std::string DebugName;
            ERHIResourceState CurrentState;
        };
    }  // namespace RHI
}  // namespace Engine
