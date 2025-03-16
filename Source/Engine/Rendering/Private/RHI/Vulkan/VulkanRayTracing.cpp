
#include "VulkanRayTracing.h"

#include <cassert>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // VulkanAccelerationStructure实现
        VulkanAccelerationStructure::VulkanAccelerationStructure(
            VkDevice device, const AccelerationStructureDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_AccelerationStructure(VK_NULL_HANDLE),
              m_DeviceMemory(VK_NULL_HANDLE),
              m_Buffer(VK_NULL_HANDLE),
              m_DeviceAddress(0) {
            CreateAccelerationStructure();
        }

        VulkanAccelerationStructure::~VulkanAccelerationStructure() {
            if (m_AccelerationStructure != VK_NULL_HANDLE) {
                vkDestroyAccelerationStructureKHR(
                    m_Device, m_AccelerationStructure, nullptr);
            }
            if (m_Buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            }
            if (m_DeviceMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
            }
        }

        uint64 VulkanAccelerationStructure::GetGPUVirtualAddress() const {
            return m_DeviceAddress;
        }

        void VulkanAccelerationStructure::CreateAccelerationStructure() {
            // 创建加速结构缓冲
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = m_Desc.ResultDataMaxSize;
            bufferInfo.usage =
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VkResult result =
                vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer);
            assert(result == VK_SUCCESS);

            // 分配内存
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = 0;  // TODO: 找到合适的内存类型

            result = vkAllocateMemory(
                m_Device, &allocInfo, nullptr, &m_DeviceMemory);
            assert(result == VK_SUCCESS);

            vkBindBufferMemory(m_Device, m_Buffer, m_DeviceMemory, 0);

            // 创建加速结构
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.buffer = m_Buffer;
            createInfo.size = m_Desc.ResultDataMaxSize;
            createInfo.type =
                VulkanRayTracingUtils::ConvertAccelerationStructureType(
                    m_Desc.Inputs.Type);

            result = vkCreateAccelerationStructureKHR(
                m_Device, &createInfo, nullptr, &m_AccelerationStructure);
            assert(result == VK_SUCCESS);

            // 获取设备地址
            VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
            addressInfo.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.accelerationStructure = m_AccelerationStructure;
            m_DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(
                m_Device, &addressInfo);
        }

        // VulkanShaderTable实现
        VulkanShaderTable::VulkanShaderTable(VkDevice device,
                                             const ShaderTableDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_Buffer(VK_NULL_HANDLE),
              m_DeviceMemory(VK_NULL_HANDLE),
              m_MappedData(nullptr) {
            CreateShaderTable();
        }

        VulkanShaderTable::~VulkanShaderTable() {
            if (m_MappedData) {
                vkUnmapMemory(m_Device, m_DeviceMemory);
            }
            if (m_Buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            }
            if (m_DeviceMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
            }
        }

        uint64 VulkanShaderTable::GetGPUVirtualAddress() const {
            VkBufferDeviceAddressInfo addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.buffer = m_Buffer;
            return vkGetBufferDeviceAddress(m_Device, &addressInfo);
        }

        void VulkanShaderTable::CreateShaderTable() {
            const uint64 tableSize =
                m_Desc.ShaderRecordSize * m_Desc.ShaderRecordCount;

            // 创建缓冲
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = tableSize;
            bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VkResult result =
                vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer);
            assert(result == VK_SUCCESS);

            // 分配并绑定内存
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = 0;  // TODO: 找到合适的内存类型

            result = vkAllocateMemory(
                m_Device, &allocInfo, nullptr, &m_DeviceMemory);
            assert(result == VK_SUCCESS);

            vkBindBufferMemory(m_Device, m_Buffer, m_DeviceMemory, 0);

            // 映射内存并复制着色器记录
            result = vkMapMemory(
                m_Device, m_DeviceMemory, 0, tableSize, 0, &m_MappedData);
            assert(result == VK_SUCCESS);

            uint8* dst = static_cast<uint8*>(m_MappedData);
            for (const auto& record : m_Desc.Records) {
                memcpy(
                    dst, record.ShaderIdentifier, record.ShaderIdentifierSize);
                if (record.LocalRootArguments) {
                    memcpy(dst + record.ShaderIdentifierSize,
                           record.LocalRootArguments,
                           record.LocalRootArgumentsSize);
                }
                dst += m_Desc.ShaderRecordSize;
            }
        }

        // VulkanRayTracingPipelineState实现
        VulkanRayTracingPipelineState::VulkanRayTracingPipelineState(
            VkDevice device, const RayTracingPipelineStateDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_Pipeline(VK_NULL_HANDLE),
              m_PipelineLayout(VK_NULL_HANDLE) {
            CreatePipeline();
        }

        VulkanRayTracingPipelineState::~VulkanRayTracingPipelineState() {
            if (m_Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            }
            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            }
        }

        const void* VulkanRayTracingPipelineState::GetShaderIdentifier(
            const std::string& entryPoint) const {
            auto it = m_ShaderIdentifiers.find(entryPoint);
            return it != m_ShaderIdentifiers.end() ? it->second.data()
                                                   : nullptr;
        }

        uint32 VulkanRayTracingPipelineState::GetShaderIdentifierSize() const {
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties = {};
            rtProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

            VkPhysicalDeviceProperties2 props2 = {};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &rtProperties;

            // 获取物理设备属性
            VkPhysicalDevice physicalDevice =
                static_cast<VulkanDevice*>(VulkanRHI::Get().GetDevice())
                    ->GetPhysicalDevice()
                    ->GetHandle();
            vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

            return rtProperties.shaderGroupHandleSize;
        }

        void VulkanRayTracingPipelineState::CreatePipeline() {
            // TODO: 实现光线追踪管线创建
            // 1. 创建管线布局
            // 2. 创建着色器阶段
            // 3. 创建着色器组
            // 4. 创建光线追踪管线
            // 5. 获取着色器标识符
        }

        // VulkanRayTracingUtils实现
        namespace VulkanRayTracingUtils {

            VkAccelerationStructureTypeKHR ConvertAccelerationStructureType(
                EAccelerationStructureType type) {
                switch (type) {
                    case EAccelerationStructureType::TopLevel:
                        return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
                    case EAccelerationStructureType::BottomLevel:
                        return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                    default:
                        assert(false && "Invalid acceleration structure type");
                        return VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
                }
            }

            VkBuildAccelerationStructureFlagsKHR ConvertBuildFlags(
                EAccelerationStructureBuildFlags flags) {
                VkBuildAccelerationStructureFlagsKHR result = 0;

                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EAccelerationStructureBuildFlags::AllowUpdate))
                    result |=
                        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EAccelerationStructureBuildFlags::AllowCompaction))
                    result |=
                        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EAccelerationStructureBuildFlags::PreferFastTrace))
                    result |=
                        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EAccelerationStructureBuildFlags::PreferFastBuild))
                    result |=
                        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(
                        EAccelerationStructureBuildFlags::MinimizeMemory))
                    result |=
                        VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;

                return result;
            }

            VkGeometryFlagsKHR ConvertGeometryFlags(EGeometryFlags flags) {
                VkGeometryFlagsKHR result = 0;

                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(EGeometryFlags::Opaque))
                    result |= VK_GEOMETRY_OPAQUE_BIT_KHR;
                if (static_cast<uint32>(flags) &
                    static_cast<uint32>(EGeometryFlags::NoDuplicateAnyHit))
                    result |=
                        VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

                return result;
            }

            void ConvertGeometryDesc(const GeometryDesc& src,
                                     VkAccelerationStructureGeometryKHR& dst) {
                dst.sType =
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                dst.flags = ConvertGeometryFlags(src.Flags);

                // TODO: 实现几何体描述转换
                // 1. 转换三角形几何体
                // 2. 转换AABB几何体
            }

        }  // namespace VulkanRayTracingUtils

    }  // namespace RHI
}  // namespace Engine
