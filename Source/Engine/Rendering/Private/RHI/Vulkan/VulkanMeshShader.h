
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIMeshShader.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan网格着色器管线状态实现
        class VulkanMeshPipelineState : public IRHIMeshPipelineState {
          public:
            VulkanMeshPipelineState(VkDevice device,
                                    const MeshPipelineStateDesc& desc);
            virtual ~VulkanMeshPipelineState();

            // IRHIMeshPipelineState接口实现
            virtual const MeshPipelineStateDesc& GetDesc() const override {
                return m_Desc;
            }

            // Vulkan特定方法
            VkPipeline GetVkPipeline() const { return m_Pipeline; }
            VkPipelineLayout GetVkPipelineLayout() const {
                return m_PipelineLayout;
            }

          private:
            void CreatePipeline();
            VkPipelineRasterizationStateCreateInfo CreateRasterizationState()
                const;
            VkPipelineDepthStencilStateCreateInfo CreateDepthStencilState()
                const;
            VkPipelineColorBlendStateCreateInfo CreateBlendState() const;
            VkPipelineMultisampleStateCreateInfo CreateMultisampleState() const;

            VkDevice m_Device;
            MeshPipelineStateDesc m_Desc;
            VkPipeline m_Pipeline;
            VkPipelineLayout m_PipelineLayout;
        };

        // 辅助函数
        namespace VulkanMeshShaderUtils {
            VkFormat ConvertPixelFormat(EPixelFormat format);
            void ConvertRasterizerDesc(
                const RasterizerDesc& src,
                VkPipelineRasterizationStateCreateInfo& dst);
            void ConvertDepthStencilDesc(
                const DepthStencilDesc& src,
                VkPipelineDepthStencilStateCreateInfo& dst);
            void ConvertBlendDesc(const BlendDesc& src,
                                  VkPipelineColorBlendStateCreateInfo& dst);
        }  // namespace VulkanMeshShaderUtils

    }  // namespace RHI
}  // namespace Engine
