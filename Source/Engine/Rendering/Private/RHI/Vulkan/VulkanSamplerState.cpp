
#include "VulkanSamplerState.h"

#include "Core/Public/Log/LogSystem.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {
        VulkanSamplerState::VulkanSamplerState(VulkanDevice* device,
                                               const SamplerDesc& desc)
            : VulkanResource(device, "Sampler"),
              Desc(desc),
              Sampler(VK_NULL_HANDLE),
              DebugName("Sampler"),
              CurrentState(ERHIResourceState::Common) {
            CreateSampler();
        }

        VulkanSamplerState::~VulkanSamplerState() {
            if (Sampler != VK_NULL_HANDLE) {
                vkDestroySampler(*Device->GetHandle(), Sampler, nullptr);
                Sampler = VK_NULL_HANDLE;
            }
        }

        bool VulkanSamplerState::CreateSampler() {
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            // 设置过滤模式
            switch (Desc.Filter) {
                case ESamplerFilter::MinMagMipPoint:
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;
                case ESamplerFilter::MinMagMipLinear:
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;
                default:
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }

            // 设置寻址模式
            auto convertAddressMode = [](ESamplerAddressMode mode) {
                switch (mode) {
                    case ESamplerAddressMode::Wrap:
                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                    case ESamplerAddressMode::Mirror:
                        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                    case ESamplerAddressMode::Clamp:
                        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                    case ESamplerAddressMode::Border:
                        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                    default:
                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                }
            };

            samplerInfo.addressModeU = convertAddressMode(Desc.AddressU);
            samplerInfo.addressModeV = convertAddressMode(Desc.AddressV);
            samplerInfo.addressModeW = convertAddressMode(Desc.AddressW);

            // 各向异性过滤
            samplerInfo.anisotropyEnable =
                Desc.MaxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
            samplerInfo.maxAnisotropy = static_cast<float>(Desc.MaxAnisotropy);

            // MipMap设置
            samplerInfo.mipLodBias = Desc.MipLODBias;
            samplerInfo.minLod = Desc.MinLOD;
            samplerInfo.maxLod = Desc.MaxLOD;

            // 比较函数
            samplerInfo.compareEnable =
                Desc.ComparisonFunc != ECompareFunction::Never ? VK_TRUE
                                                               : VK_FALSE;
            switch (Desc.ComparisonFunc) {
                case ECompareFunction::Less:
                    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
                    break;
                case ECompareFunction::LessEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                    break;
                case ECompareFunction::Greater:
                    samplerInfo.compareOp = VK_COMPARE_OP_GREATER;
                    break;
                case ECompareFunction::GreaterEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
                    break;
                case ECompareFunction::Equal:
                    samplerInfo.compareOp = VK_COMPARE_OP_EQUAL;
                    break;
                case ECompareFunction::NotEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                    break;
                case ECompareFunction::Always:
                    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                    break;
                default:
                    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            }

            // 边框颜色
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

            if (vkCreateSampler(
                    *Device->GetHandle(), &samplerInfo, nullptr, &Sampler) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create sampler!");
                return false;
            }

            return true;
        }
    }  // namespace RHI
}  // namespace Engine
