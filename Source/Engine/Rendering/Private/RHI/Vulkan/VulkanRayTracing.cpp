
#include "VulkanRayTracing.h"

#include <cassert>

#include "VulkanRHI.h"
#include "VulkanResources.h"
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
            // 查找支持设备地址和设备本地的内存类型
            VkPhysicalDeviceMemoryProperties memProperties;
            VkPhysicalDevice physicalDevice =
                VulkanRHI::Get().GetDevice()->GetPhysicalDevice()->GetHandle();
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            // 需要设备本地内存以获得最佳性能
            VkMemoryPropertyFlags properties =
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            // 查找满足要求的内存类型
            uint32_t memoryTypeIndex = UINT32_MAX;
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((memRequirements.memoryTypeBits & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & properties) ==
                        properties) {
                    memoryTypeIndex = i;
                    break;
                }
            }
            assert(memoryTypeIndex != UINT32_MAX &&
                   "Failed to find suitable memory type");

            allocInfo.memoryTypeIndex = memoryTypeIndex;

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

        static uint32 GetShaderIdentifierSizeForDevice(
            VkPhysicalDevice physicalDevice) {
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties = {};
            rtProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

            VkPhysicalDeviceProperties2 props2 = {};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &rtProperties;

            vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

            return rtProperties.shaderGroupHandleSize;
        }

        uint32 VulkanRayTracingPipelineState::GetShaderIdentifierSize() const {
            VkPhysicalDevice physicalDevice =
                static_cast<VulkanDevice*>(VulkanRHI::Get().GetDevice())
                    ->GetPhysicalDevice()
                    ->GetHandle();
            return GetShaderIdentifierSizeForDevice(physicalDevice);
        }

        void VulkanRayTracingPipelineState::CreatePipeline() {
            // 1. 创建管线布局
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount =
                0;  // TODO: Add descriptor set layouts support
            pipelineLayoutInfo.pSetLayouts = nullptr;
            pipelineLayoutInfo.pushConstantRangeCount =
                0;  // TODO: Add push constants support
            pipelineLayoutInfo.pPushConstantRanges = nullptr;

            VkResult result = vkCreatePipelineLayout(
                m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
            assert(result == VK_SUCCESS && "Failed to create pipeline layout!");

            // 2. 创建着色器阶段
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

            // Ray Generation Shader
            if (m_Desc.RayGenShader) {
                VkPipelineShaderStageCreateInfo stageInfo = {};
                stageInfo.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                stageInfo.module =
                    dynamic_cast<VulkanShader*>(m_Desc.RayGenShader)
                        ->GetHandle();
                stageInfo.pName = "main";  // TODO: Support custom entry points
                shaderStages.push_back(stageInfo);
            }

            // Miss Shaders
            for (auto* missShader : m_Desc.MissShaders) {
                VkPipelineShaderStageCreateInfo stageInfo = {};
                stageInfo.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                stageInfo.module =
                    dynamic_cast<VulkanShader*>(missShader)->GetHandle();
                stageInfo.pName = "main";  // TODO: Support custom entry points
                shaderStages.push_back(stageInfo);
            }

            // Hit Groups
            for (auto* hitGroup : m_Desc.HitGroups) {
                VkPipelineShaderStageCreateInfo stageInfo = {};
                stageInfo.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.stage =
                    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  // TODO: Support other
                                                          // hit shader types
                stageInfo.module =
                    dynamic_cast<VulkanShader*>(hitGroup)->GetHandle();
                stageInfo.pName = "main";  // TODO: Support custom entry points
                shaderStages.push_back(stageInfo);
            }

            // 3. 创建着色器组
            std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

            // Ray Generation Group
            {
                VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
                groupInfo.sType =
                    VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                groupInfo.generalShader = 0;  // RayGen shader is always first
                groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
                groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
                groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
                shaderGroups.push_back(groupInfo);
            }

            // Miss Groups
            for (size_t i = 0; i < m_Desc.MissShaders.size(); ++i) {
                VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
                groupInfo.sType =
                    VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                groupInfo.generalShader =
                    static_cast<uint32_t>(i + 1);  // Offset by RayGen shader
                groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
                groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
                groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
                shaderGroups.push_back(groupInfo);
            }

            // Hit Groups
            size_t hitGroupOffset =
                1 +
                m_Desc.MissShaders.size();  // Offset by RayGen + Miss shaders
            for (size_t i = 0; i < m_Desc.HitGroups.size(); ++i) {
                VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
                groupInfo.sType =
                    VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                groupInfo.type =
                    VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
                groupInfo.closestHitShader =
                    static_cast<uint32_t>(i + hitGroupOffset);
                groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
                groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
                shaderGroups.push_back(groupInfo);
            }

            // 4. 创建光线追踪管线
            VkRayTracingPipelineCreateInfoKHR pipelineInfo = {};
            pipelineInfo.sType =
                VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
            pipelineInfo.stageCount =
                static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.groupCount =
                static_cast<uint32_t>(shaderGroups.size());
            pipelineInfo.pGroups = shaderGroups.data();
            pipelineInfo.maxPipelineRayRecursionDepth =
                m_Desc.MaxRecursionDepth;
            pipelineInfo.layout = m_PipelineLayout;

            result = vkCreateRayTracingPipelinesKHR(m_Device,
                                                    VK_NULL_HANDLE,
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineInfo,
                                                    nullptr,
                                                    &m_Pipeline);
            assert(result == VK_SUCCESS &&
                   "Failed to create ray tracing pipeline!");

            // 5. 获取着色器标识符
            VkPhysicalDevice physicalDevice =
                static_cast<VulkanDevice*>(VulkanRHI::Get().GetDevice())
                    ->GetPhysicalDevice()
                    ->GetHandle();
            uint32_t handleSize =
                GetShaderIdentifierSizeForDevice(physicalDevice);
            uint32_t handleSizeAligned = (handleSize + 15) & ~15;
            uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
            std::vector<uint8_t> shaderHandleStorage(groupCount *
                                                     handleSizeAligned);

            result = vkGetRayTracingShaderGroupHandlesKHR(
                m_Device,
                m_Pipeline,
                0,
                groupCount,
                shaderHandleStorage.size(),
                shaderHandleStorage.data());
            assert(result == VK_SUCCESS &&
                   "Failed to get shader group handles!");

            // 存储每个着色器的标识符
            // RayGen shader
            {
                std::vector<uint8_t> identifier(handleSize);
                memcpy(
                    identifier.data(), shaderHandleStorage.data(), handleSize);
                m_ShaderIdentifiers["raygen"] = std::move(identifier);
            }

            // Miss shaders
            for (size_t i = 0; i < m_Desc.MissShaders.size(); ++i) {
                std::vector<uint8_t> identifier(handleSize);
                memcpy(
                    identifier.data(),
                    shaderHandleStorage.data() + ((i + 1) * handleSizeAligned),
                    handleSize);
                m_ShaderIdentifiers["miss" + std::to_string(i)] =
                    std::move(identifier);
            }

            // Hit groups
            for (size_t i = 0; i < m_Desc.HitGroups.size(); ++i) {
                std::vector<uint8_t> identifier(handleSize);
                size_t offset =
                    (1 + m_Desc.MissShaders.size() + i) * handleSizeAligned;
                memcpy(identifier.data(),
                       shaderHandleStorage.data() + offset,
                       handleSize);
                m_ShaderIdentifiers["hitgroup" + std::to_string(i)] =
                    std::move(identifier);
            }
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

                // 检查是否有三角形数据
                if (src.Triangles.VertexBuffer != nullptr) {
                    dst.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

                    // 设置三角形几何体数据
                    auto& triangles = dst.geometry.triangles;
                    triangles.sType =
                        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

                    // 设置顶点数据
                    triangles.vertexFormat =
                        static_cast<VkFormat>(src.Triangles.VertexFormat);
                    triangles.vertexData.deviceAddress =
                        src.Triangles.VertexBuffer->GetGPUVirtualAddress();
                    triangles.vertexStride = src.Triangles.VertexStride;
                    triangles.maxVertex = src.Triangles.VertexCount;

                    // 设置索引数据（如果有）
                    if (src.Triangles.IndexBuffer != nullptr) {
                        triangles.indexType =
                            static_cast<VkIndexType>(src.Triangles.IndexFormat);
                        triangles.indexData.deviceAddress =
                            src.Triangles.IndexBuffer->GetGPUVirtualAddress();
                    } else {
                        triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
                        triangles.indexData.deviceAddress = 0;
                    }

                    // 设置变换数据（如果有）
                    if (src.Triangles.TransformBuffer != nullptr) {
                        triangles.transformData.deviceAddress =
                            src.Triangles.TransformBuffer
                                ->GetGPUVirtualAddress();
                    } else {
                        triangles.transformData.deviceAddress = 0;
                    }
                }
                // 检查是否有AABB数据
                else if (src.AABBs.AABBBuffer != nullptr) {
                    dst.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;

                    // 设置AABB几何体数据
                    auto& aabbs = dst.geometry.aabbs;
                    aabbs.sType =
                        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;

                    // 设置AABB数据
                    aabbs.data.deviceAddress =
                        src.AABBs.AABBBuffer->GetGPUVirtualAddress();
                    aabbs.stride = src.AABBs.AABBStride;
                } else {
                    assert(false &&
                           "Invalid geometry description - neither triangles "
                           "nor AABBs specified");
                }
            }

        }  // namespace VulkanRayTracingUtils

    }  // namespace RHI
}  // namespace Engine
