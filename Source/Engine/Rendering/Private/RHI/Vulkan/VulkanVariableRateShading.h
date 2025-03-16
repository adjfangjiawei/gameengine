
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIVariableRateShading.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan VRS图案实现
        class VulkanShadingRatePattern : public IRHIShadingRatePattern {
          public:
            VulkanShadingRatePattern(VkDevice device,
                                     const ShadingRatePatternDesc& desc);
            virtual ~VulkanShadingRatePattern();

            // IRHIShadingRatePattern接口实现
            virtual const ShadingRatePatternDesc& GetDesc() const override {
                return m_Desc;
            }

            // Vulkan特定方法
            VkImageView GetVkImageView() const { return m_ImageView; }
            VkImage GetVkImage() const { return m_Image; }

          private:
            void CreateImage();
            void CreateImageView();
            void UpdatePattern();

            VkDevice m_Device;
            ShadingRatePatternDesc m_Desc;
            VkImage m_Image;
            VkDeviceMemory m_DeviceMemory;
            VkImageView m_ImageView;
        };

        // VRS管理器
        class VulkanVariableRateShadingManager {
          public:
            VulkanVariableRateShadingManager(VkDevice device,
                                             VkPhysicalDevice physicalDevice);
            ~VulkanVariableRateShadingManager();

            // 查询VRS能力
            VariableRateShadingCapabilities GetCapabilities() const;

            // 设置着色率
            void SetShadingRate(VkCommandBuffer cmdBuffer, EShadingRate rate);

            // 设置着色率组合器
            void SetShadingRateCombiner(VkCommandBuffer cmdBuffer,
                                        EShadingRateCombiner primitiveRate,
                                        EShadingRateCombiner textureRate);

            // 设置着色率图案
            void SetShadingRatePattern(VkCommandBuffer cmdBuffer,
                                       const VulkanShadingRatePattern* pattern);

            // 设置着色率区域
            void SetShadingRateRegion(VkCommandBuffer cmdBuffer,
                                      const ShadingRateRegion& region);

          private:
            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            bool m_SupportsVRS;
            VkPhysicalDeviceFragmentShadingRateFeaturesKHR m_VRSFeatures;
            VkPhysicalDeviceFragmentShadingRatePropertiesKHR m_VRSProperties;
        };

        // 辅助函数
        namespace VulkanVRSUtils {
            VkFragmentShadingRateNV ConvertShadingRate(EShadingRate rate);
            VkFragmentShadingRateCombinerOpKHR ConvertCombiner(
                EShadingRateCombiner combiner);
            void GetFragmentSize(EShadingRate rate,
                                 uint32& width,
                                 uint32& height);
            bool IsShadingRateSupported(VkPhysicalDevice device,
                                        EShadingRate rate);
        }  // namespace VulkanVRSUtils

    }  // namespace RHI
}  // namespace Engine
