
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIRayTracing.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan加速结构实现
        class VulkanAccelerationStructure : public IRHIAccelerationStructure {
          public:
            VulkanAccelerationStructure(VkDevice device,
                                        const AccelerationStructureDesc& desc);
            virtual ~VulkanAccelerationStructure();

            // IRHIAccelerationStructure接口实现
            virtual const AccelerationStructureDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual uint64 GetGPUVirtualAddress() const override;

            // Vulkan特定方法
            VkAccelerationStructureKHR GetVkAccelerationStructure() const {
                return m_AccelerationStructure;
            }
            VkDeviceMemory GetVkDeviceMemory() const { return m_DeviceMemory; }

          private:
            void CreateAccelerationStructure();

            VkDevice m_Device;
            AccelerationStructureDesc m_Desc;
            VkAccelerationStructureKHR m_AccelerationStructure;
            VkDeviceMemory m_DeviceMemory;
            VkBuffer m_Buffer;
            uint64 m_DeviceAddress;
        };

        // Vulkan着色器表实现
        class VulkanShaderTable : public IRHIShaderTable {
          public:
            VulkanShaderTable(VkDevice device, const ShaderTableDesc& desc);
            virtual ~VulkanShaderTable();

            // IRHIShaderTable接口实现
            virtual const ShaderTableDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual uint64 GetGPUVirtualAddress() const override;

            // Vulkan特定方法
            VkBuffer GetVkBuffer() const { return m_Buffer; }

          private:
            void CreateShaderTable();

            VkDevice m_Device;
            ShaderTableDesc m_Desc;
            VkBuffer m_Buffer;
            VkDeviceMemory m_DeviceMemory;
            void* m_MappedData;
        };

        // Vulkan光线追踪管线状态实现
        class VulkanRayTracingPipelineState
            : public IRHIRayTracingPipelineState {
          public:
            VulkanRayTracingPipelineState(
                VkDevice device, const RayTracingPipelineStateDesc& desc);
            virtual ~VulkanRayTracingPipelineState();

            // IRHIRayTracingPipelineState接口实现
            virtual const RayTracingPipelineStateDesc& GetDesc()
                const override {
                return m_Desc;
            }
            virtual const void* GetShaderIdentifier(
                const std::string& entryPoint) const override;
            virtual uint32 GetShaderIdentifierSize() const override;

            // Vulkan特定方法
            VkPipeline GetVkPipeline() const { return m_Pipeline; }

          private:
            void CreatePipeline();

            VkDevice m_Device;
            RayTracingPipelineStateDesc m_Desc;
            VkPipeline m_Pipeline;
            VkPipelineLayout m_PipelineLayout;
            std::unordered_map<std::string, std::vector<uint8>>
                m_ShaderIdentifiers;
        };

        // 辅助函数
        namespace VulkanRayTracingUtils {
            VkAccelerationStructureTypeKHR ConvertAccelerationStructureType(
                EAccelerationStructureType type);
            VkBuildAccelerationStructureFlagsKHR ConvertBuildFlags(
                EAccelerationStructureBuildFlags flags);
            VkGeometryFlagsKHR ConvertGeometryFlags(EGeometryFlags flags);
            void ConvertGeometryDesc(const GeometryDesc& src,
                                     VkAccelerationStructureGeometryKHR& dst);
        }  // namespace VulkanRayTracingUtils

    }  // namespace RHI
}  // namespace Engine
