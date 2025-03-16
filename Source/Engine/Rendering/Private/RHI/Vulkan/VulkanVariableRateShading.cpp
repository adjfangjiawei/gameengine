
#include "VulkanVariableRateShading.h"

#include <cassert>

namespace Engine {
    namespace RHI {

        // VulkanShadingRatePattern实现
        VulkanShadingRatePattern::VulkanShadingRatePattern(
            VkDevice device, const ShadingRatePatternDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_Image(VK_NULL_HANDLE),
              m_DeviceMemory(VK_NULL_HANDLE),
              m_ImageView(VK_NULL_HANDLE) {
            CreateImage();
            CreateImageView();
            UpdatePattern();
        }

        VulkanShadingRatePattern::~VulkanShadingRatePattern() {
            if (m_ImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Device, m_ImageView, nullptr);
            }
            if (m_Image != VK_NULL_HANDLE) {
                vkDestroyImage(m_Device, m_Image, nullptr);
            }
            if (m_DeviceMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
            }
        }

        void VulkanShadingRatePattern::CreateImage() {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = VK_FORMAT_R8_UINT;
            imageInfo.extent.width = m_Desc.Width;
            imageInfo.extent.height = m_Desc.Height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage =
                VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkResult result =
                vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image);
            assert(result == VK_SUCCESS);

            // 分配内存
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = 0;  // TODO: 找到合适的内存类型

            result = vkAllocateMemory(
                m_Device, &allocInfo, nullptr, &m_DeviceMemory);
            assert(result == VK_SUCCESS);

            vkBindImageMemory(m_Device, m_Image, m_DeviceMemory, 0);
        }

        void VulkanShadingRatePattern::CreateImageView() {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8_UINT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkResult result =
                vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView);
            assert(result == VK_SUCCESS);
        }

        void VulkanShadingRatePattern::UpdatePattern() {
            // TODO: 将着色率图案数据上传到图像
            // 1. 创建暂存缓冲
            // 2. 将图案数据复制到暂存缓冲
            // 3. 记录命令以将数据从暂存缓冲复制到图像
            // 4. 提交命令并等待完成
        }

        // VulkanVariableRateShadingManager实现
        VulkanVariableRateShadingManager::VulkanVariableRateShadingManager(
            VkDevice device, VkPhysicalDevice physicalDevice)
            : m_Device(device),
              m_PhysicalDevice(physicalDevice),
              m_SupportsVRS(false) {
            // 查询VRS特性
            m_VRSFeatures = {};
            m_VRSFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

            m_VRSProperties = {};
            m_VRSProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

            VkPhysicalDeviceFeatures2 features2 = {};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features2.pNext = &m_VRSFeatures;

            VkPhysicalDeviceProperties2 props2 = {};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &m_VRSProperties;

            vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
            vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

            m_SupportsVRS = m_VRSFeatures.pipelineFragmentShadingRate &&
                            m_VRSFeatures.attachmentFragmentShadingRate &&
                            m_VRSFeatures.primitiveFragmentShadingRate;
        }

        VulkanVariableRateShadingManager::~VulkanVariableRateShadingManager() {}

        VariableRateShadingCapabilities
        VulkanVariableRateShadingManager::GetCapabilities() const {
            VariableRateShadingCapabilities caps;
            caps.SupportsVariableRateShading = m_SupportsVRS;
            caps.SupportsPerDrawVRS = m_VRSFeatures.pipelineFragmentShadingRate;
            caps.SupportsTextureBasedVRS =
                m_VRSFeatures.attachmentFragmentShadingRate;
            caps.MaxShadingRatePatternSize =
                m_VRSProperties.maxFragmentShadingRateAttachmentTexelSize.width;

            // 添加支持的着色率
            if (m_SupportsVRS) {
                caps.SupportedShadingRates = {EShadingRate::Rate1x1,
                                              EShadingRate::Rate1x2,
                                              EShadingRate::Rate2x1,
                                              EShadingRate::Rate2x2};

                // 检查是否支持更大的着色率
                if (m_VRSProperties.maxFragmentShadingRateRasterizationSamples >
                    VK_SAMPLE_COUNT_2_BIT) {
                    caps.SupportedShadingRates.push_back(EShadingRate::Rate2x4);
                    caps.SupportedShadingRates.push_back(EShadingRate::Rate4x2);
                    caps.SupportedShadingRates.push_back(EShadingRate::Rate4x4);
                }
            }

            // 添加支持的组合器
            if (m_SupportsVRS) {
                caps.SupportedCombiners = {EShadingRateCombiner::Override,
                                           EShadingRateCombiner::Min,
                                           EShadingRateCombiner::Max};
            }

            return caps;
        }

        void VulkanVariableRateShadingManager::SetShadingRate(
            VkCommandBuffer cmdBuffer, [[maybe_unused]] EShadingRate rate) {
            if (!m_SupportsVRS) return;

            uint32_t width = 0, height = 0;
            VulkanVRSUtils::GetFragmentSize(rate, width, height);
            VkExtent2D fragmentSize = {width, height};
            vkCmdSetFragmentShadingRateKHR(cmdBuffer, &fragmentSize, nullptr);
        }

        void VulkanVariableRateShadingManager::SetShadingRateCombiner(
            VkCommandBuffer cmdBuffer,
            EShadingRateCombiner primitiveRate,
            EShadingRateCombiner textureRate) {
            if (!m_SupportsVRS) return;

            VkFragmentShadingRateCombinerOpKHR combiners[2] = {
                VulkanVRSUtils::ConvertCombiner(primitiveRate),
                VulkanVRSUtils::ConvertCombiner(textureRate)};

            vkCmdSetFragmentShadingRateKHR(cmdBuffer, nullptr, combiners);
        }

        void VulkanVariableRateShadingManager::SetShadingRatePattern(
            [[maybe_unused]] VkCommandBuffer cmdBuffer,
            const VulkanShadingRatePattern* pattern) {
            if (!m_SupportsVRS || !pattern) return;

            // TODO: 设置VRS附件
        }

        void VulkanVariableRateShadingManager::SetShadingRateRegion(
            [[maybe_unused]] VkCommandBuffer cmdBuffer,
            [[maybe_unused]] const ShadingRateRegion& region) {
            if (!m_SupportsVRS) return;

            // TODO: 实现区域VRS设置
        }

        // VulkanVRSUtils实现
        namespace VulkanVRSUtils {

            VkFragmentShadingRateNV ConvertShadingRate(EShadingRate rate) {
                switch (rate) {
                    case EShadingRate::Rate1x1:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
                    case EShadingRate::Rate1x2:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV;
                    case EShadingRate::Rate2x1:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV;
                    case EShadingRate::Rate2x2:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
                    case EShadingRate::Rate2x4:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV;
                    case EShadingRate::Rate4x2:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV;
                    case EShadingRate::Rate4x4:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;
                    default:
                        return VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
                }
            }

            VkExtent2D GetShadingRateExtent(EShadingRate rate) {
                VkExtent2D extent = {1, 1};
                switch (rate) {
                    case EShadingRate::Rate1x1:
                        extent = {1, 1};
                        break;
                    case EShadingRate::Rate1x2:
                        extent = {1, 2};
                        break;
                    case EShadingRate::Rate2x1:
                        extent = {2, 1};
                        break;
                    case EShadingRate::Rate2x2:
                        extent = {2, 2};
                        break;
                    case EShadingRate::Rate2x4:
                        extent = {2, 4};
                        break;
                    case EShadingRate::Rate4x2:
                        extent = {4, 2};
                        break;
                    case EShadingRate::Rate4x4:
                        extent = {4, 4};
                        break;
                    default:
                        extent = {1, 1};
                        break;
                }
                return extent;
            }

            VkFragmentShadingRateCombinerOpKHR ConvertCombiner(
                EShadingRateCombiner combiner) {
                switch (combiner) {
                    case EShadingRateCombiner::Override:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
                    case EShadingRateCombiner::Min:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR;
                    case EShadingRateCombiner::Max:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
                    case EShadingRateCombiner::Sum:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR;
                    case EShadingRateCombiner::Multiply:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR;
                    default:
                        return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
                }
            }

            void GetFragmentSize(EShadingRate rate,
                                 uint32& width,
                                 uint32& height) {
                switch (rate) {
                    case EShadingRate::Rate1x1:
                        width = 1;
                        height = 1;
                        break;
                    case EShadingRate::Rate1x2:
                        width = 1;
                        height = 2;
                        break;
                    case EShadingRate::Rate2x1:
                        width = 2;
                        height = 1;
                        break;
                    case EShadingRate::Rate2x2:
                        width = 2;
                        height = 2;
                        break;
                    case EShadingRate::Rate2x4:
                        width = 2;
                        height = 4;
                        break;
                    case EShadingRate::Rate4x2:
                        width = 4;
                        height = 2;
                        break;
                    case EShadingRate::Rate4x4:
                        width = 4;
                        height = 4;
                        break;
                    default:
                        width = 1;
                        height = 1;
                        break;
                }
            }

            bool IsShadingRateSupported(
                [[maybe_unused]] VkPhysicalDevice device,
                [[maybe_unused]] EShadingRate rate) {
                // TODO: 实现着色率支持检查
                return true;
            }
        }  // namespace VulkanVRSUtils

    }  // namespace RHI

}  // namespace Engine
