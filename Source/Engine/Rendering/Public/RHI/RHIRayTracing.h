
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 加速结构类型
        enum class EAccelerationStructureType : uint8 {
            TopLevel,     // 顶层加速结构
            BottomLevel,  // 底层加速结构
        };

        // 加速结构构建标志
        enum class EAccelerationStructureBuildFlags : uint32 {
            None = 0,
            AllowUpdate = 1 << 0,
            AllowCompaction = 1 << 1,
            PreferFastTrace = 1 << 2,
            PreferFastBuild = 1 << 3,
            MinimizeMemory = 1 << 4,
        };

        // 几何体标志
        enum class EGeometryFlags : uint32 {
            None = 0,
            Opaque = 1 << 0,
            NoDuplicateAnyHit = 1 << 1,
        };

        // 几何体描述
        struct GeometryDesc {
            EGeometryFlags Flags = EGeometryFlags::None;

            // 三角形几何体
            struct {
                IRHIBuffer* VertexBuffer;
                uint32 VertexCount;
                uint32 VertexStride;
                EVertexElementFormat VertexFormat;

                IRHIBuffer* IndexBuffer;
                uint32 IndexCount;
                EPixelFormat IndexFormat;

                IRHIBuffer* TransformBuffer;
            } Triangles;

            // AABB几何体
            struct {
                IRHIBuffer* AABBBuffer;
                uint32 AABBCount;
                uint32 AABBStride;
            } AABBs;
        };

        // 加速结构输入
        struct AccelerationStructureInputs {
            EAccelerationStructureType Type;
            std::vector<GeometryDesc> Geometries;
        };

        // 加速结构描述
        struct AccelerationStructureDesc {
            EAccelerationStructureBuildFlags Flags;
            AccelerationStructureInputs Inputs;
            uint64 ResultDataMaxSize;
            uint64 ScratchDataSize;
            std::string DebugName;
        };

        // 加速结构接口
        class IRHIAccelerationStructure : public IRHIResource {
          public:
            virtual ~IRHIAccelerationStructure() = default;

            // 获取描述
            virtual const AccelerationStructureDesc& GetDesc() const = 0;

            // 获取GPU虚拟地址
            virtual uint64 GetGPUVirtualAddress() const = 0;
        };

        // 着色器记录
        struct ShaderRecord {
            const void* ShaderIdentifier;
            uint32 ShaderIdentifierSize;
            const void* LocalRootArguments;
            uint32 LocalRootArgumentsSize;
        };

        // 着色器表描述
        struct ShaderTableDesc {
            uint32 ShaderRecordSize;
            uint32 ShaderRecordCount;
            std::vector<ShaderRecord> Records;
            std::string DebugName;
        };

        // 着色器表接口
        class IRHIShaderTable : public IRHIResource {
          public:
            virtual ~IRHIShaderTable() = default;

            // 获取描述
            virtual const ShaderTableDesc& GetDesc() const = 0;

            // 获取GPU虚拟地址
            virtual uint64 GetGPUVirtualAddress() const = 0;
        };

        // 光线追踪管线状态描述
        struct RayTracingPipelineStateDesc {
            // 着色器阶段
            IRHIShader* RayGenShader = nullptr;
            std::vector<IRHIShader*> MissShaders;
            std::vector<IRHIShader*> HitGroups;

            // 最大递归深度
            uint32 MaxRecursionDepth = 1;

            // 负载大小
            uint32 MaxPayloadSize = 0;
            uint32 MaxAttributeSize = 0;

            std::string DebugName;
        };

        // 光线追踪管线状态接口
        class IRHIRayTracingPipelineState : public IRHIResource {
          public:
            virtual ~IRHIRayTracingPipelineState() = default;

            // 获取描述
            virtual const RayTracingPipelineStateDesc& GetDesc() const = 0;

            // 获取着色器标识符
            virtual const void* GetShaderIdentifier(
                const std::string& entryPoint) const = 0;
            virtual uint32 GetShaderIdentifierSize() const = 0;
        };

    }  // namespace RHI
}  // namespace Engine
