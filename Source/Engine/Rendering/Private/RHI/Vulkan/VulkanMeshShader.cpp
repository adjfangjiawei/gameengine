
#include "VulkanMeshShader.h"

#include <cassert>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        VulkanMeshPipelineState::VulkanMeshPipelineState(
            VkDevice device, const MeshPipelineStateDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_Pipeline(VK_NULL_HANDLE),
              m_PipelineLayout(VK_NULL_HANDLE) {
            CreatePipeline();
        }

        VulkanMeshPipelineState::~VulkanMeshPipelineState() {
            if (m_Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            }
            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            }
        }

        void VulkanMeshPipelineState::CreatePipeline() {
            // 创建管线布局
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            // TODO: 添加描述符集布局和推送常量

            VkResult result = vkCreatePipelineLayout(
                m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
            assert(result == VK_SUCCESS);

            // 创建着色器阶段
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

            // 网格着色器
            if (m_Desc.MeshShader) {
                VkPipelineShaderStageCreateInfo meshShaderStage = {};
                meshShaderStage.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                meshShaderStage.stage = VK_SHADER_STAGE_MESH_BIT_NV;
                // TODO: 设置着色器模块和入口点
                shaderStages.push_back(meshShaderStage);
            }

            // 像素着色器
            if (m_Desc.PixelShader) {
                VkPipelineShaderStageCreateInfo pixelShaderStage = {};
                pixelShaderStage.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                pixelShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                // TODO: 设置着色器模块和入口点
                shaderStages.push_back(pixelShaderStage);
            }

            // 放大着色器
            if (m_Desc.AmplificationShader) {
                VkPipelineShaderStageCreateInfo amplificationShaderStage = {};
                amplificationShaderStage.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                amplificationShaderStage.stage = VK_SHADER_STAGE_TASK_BIT_NV;
                // TODO: 设置着色器模块和入口点
                shaderStages.push_back(amplificationShaderStage);
            }

            // 创建渲染目标格式数组
            std::vector<VkFormat> colorFormats;
            for (const auto& format : m_Desc.RenderTargetFormats) {
                colorFormats.push_back(
                    VulkanMeshShaderUtils::ConvertPixelFormat(format));
            }

            // 创建管线状态
            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType =
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount =
                static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.layout = m_PipelineLayout;

            // 设置各种状态
            auto rasterizationState = CreateRasterizationState();
            auto depthStencilState = CreateDepthStencilState();
            auto blendState = CreateBlendState();
            auto multisampleState = CreateMultisampleState();

            pipelineInfo.pRasterizationState = &rasterizationState;
            pipelineInfo.pDepthStencilState = &depthStencilState;
            pipelineInfo.pColorBlendState = &blendState;
            pipelineInfo.pMultisampleState = &multisampleState;

            // 创建管线
            result = vkCreateGraphicsPipelines(m_Device,
                                               VK_NULL_HANDLE,
                                               1,
                                               &pipelineInfo,
                                               nullptr,
                                               &m_Pipeline);
            assert(result == VK_SUCCESS);
        }

        VkPipelineRasterizationStateCreateInfo
        VulkanMeshPipelineState::CreateRasterizationState() const {
            VkPipelineRasterizationStateCreateInfo rasterizationState = {};
            rasterizationState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            VulkanMeshShaderUtils::ConvertRasterizerDesc(m_Desc.RasterizerState,
                                                         rasterizationState);
            return rasterizationState;
        }

        VkPipelineDepthStencilStateCreateInfo
        VulkanMeshPipelineState::CreateDepthStencilState() const {
            VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
            depthStencilState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            VulkanMeshShaderUtils::ConvertDepthStencilDesc(
                m_Desc.DepthStencilState, depthStencilState);
            return depthStencilState;
        }

        VkPipelineColorBlendStateCreateInfo
        VulkanMeshPipelineState::CreateBlendState() const {
            VkPipelineColorBlendStateCreateInfo blendState = {};
            blendState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            VulkanMeshShaderUtils::ConvertBlendDesc(m_Desc.BlendState,
                                                    blendState);
            return blendState;
        }

        VkPipelineMultisampleStateCreateInfo
        VulkanMeshPipelineState::CreateMultisampleState() const {
            VkPipelineMultisampleStateCreateInfo multisampleState = {};
            multisampleState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.rasterizationSamples =
                static_cast<VkSampleCountFlagBits>(m_Desc.SampleCount);
            multisampleState.sampleShadingEnable = VK_FALSE;
            multisampleState.minSampleShading = 1.0f;
            return multisampleState;
        }

        // VulkanMeshShaderUtils实现
        namespace VulkanMeshShaderUtils {

            VkFormat ConvertPixelFormat(EPixelFormat format) {
                switch (format) {
                    case EPixelFormat::R8G8B8A8_UNORM:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                    case EPixelFormat::B8G8R8A8_UNORM:
                        return VK_FORMAT_B8G8R8A8_UNORM;
                    case EPixelFormat::R32G32B32A32_FLOAT:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    // 添加更多格式转换...
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }

            void ConvertRasterizerDesc(
                [[maybe_unused]] const RasterizerDesc& src,
                VkPipelineRasterizationStateCreateInfo& dst) {
                dst.depthClampEnable = VK_FALSE;
                dst.rasterizerDiscardEnable = VK_FALSE;
                // TODO: 设置其他光栅化状态
            }

            void ConvertDepthStencilDesc(
                [[maybe_unused]] const DepthStencilDesc& src,
                [[maybe_unused]] VkPipelineDepthStencilStateCreateInfo& dst) {
                // TODO: 实现深度模板状态转换
            }

            void ConvertBlendDesc(const BlendDesc& src,
                                  VkPipelineColorBlendStateCreateInfo& dst) {
                dst.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                dst.logicOpEnable = VK_FALSE;    // 默认禁用逻辑操作
                dst.logicOp = VK_LOGIC_OP_COPY;  // 默认复制操作
                dst.blendConstants[0] = 0.0f;    // 默认混合常量
                dst.blendConstants[1] = 0.0f;
                dst.blendConstants[2] = 0.0f;
                dst.blendConstants[3] = 0.0f;

                // 设置每个渲染目标的混合状态
                std::vector<VkPipelineColorBlendAttachmentState> attachments;
                for (uint32_t i = 0; i < 8; ++i) {
                    const auto& rtBlend = src.RenderTarget[i];
                    VkPipelineColorBlendAttachmentState attachment = {};
                    attachment.blendEnable = rtBlend.BlendEnable;
                    attachment.srcColorBlendFactor =
                        static_cast<VkBlendFactor>(rtBlend.SrcBlend);
                    attachment.dstColorBlendFactor =
                        static_cast<VkBlendFactor>(rtBlend.DestBlend);
                    attachment.colorBlendOp =
                        static_cast<VkBlendOp>(rtBlend.BlendOp);
                    attachment.srcAlphaBlendFactor =
                        static_cast<VkBlendFactor>(rtBlend.SrcBlendAlpha);
                    attachment.dstAlphaBlendFactor =
                        static_cast<VkBlendFactor>(rtBlend.DestBlendAlpha);
                    attachment.alphaBlendOp =
                        static_cast<VkBlendOp>(rtBlend.BlendOpAlpha);
                    attachment.colorWriteMask = rtBlend.RenderTargetWriteMask;
                    attachments.push_back(attachment);
                }

                dst.attachmentCount = static_cast<uint32_t>(attachments.size());
                dst.pAttachments = attachments.data();
            }

        }  // namespace VulkanMeshShaderUtils

    }  // namespace RHI
}  // namespace Engine
