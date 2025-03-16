
#pragma once

#include <string>
#include <vector>

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 着色器编译目标
        enum class EShaderTarget : uint8 {
            SPIRV,  // Vulkan SPIR-V
            DXBC,   // DirectX Bytecode
            DXIL,   // DirectX Intermediate Language
            MSL,    // Metal Shading Language
            GLSL,   // OpenGL Shading Language
        };

        // 着色器编译选项
        struct ShaderCompileOptions {
            EShaderTarget Target = EShaderTarget::SPIRV;
            std::string EntryPoint = "main";
            std::vector<std::string> Defines;
            std::vector<std::string> IncludeDirs;
            bool Debug = false;
            bool OptimizationLevel = 3;
            std::string SourceFile;
            std::string SourceText;
        };

        // 着色器反射信息 - 资源绑定
        struct ShaderResourceBinding {
            std::string Name;
            uint32 BindPoint;
            uint32 BindCount;
            uint32 Space;
            EDescriptorRangeType Type;
        };

        // 着色器反射信息 - 常量缓冲区
        struct ShaderConstantBuffer {
            std::string Name;
            uint32 Size;
            uint32 BindPoint;
            uint32 Space;
            std::vector<std::pair<std::string, uint32>> Members;
        };

        // 着色器反射信息 - 输入输出
        struct ShaderIOVariable {
            std::string Name;
            uint32 Location;
            EVertexElementFormat Format;
            uint32 SemanticIndex;
        };

        // 着色器反射信息
        struct ShaderReflection {
            std::vector<ShaderResourceBinding> ResourceBindings;
            std::vector<ShaderConstantBuffer> ConstantBuffers;
            std::vector<ShaderIOVariable> InputVariables;
            std::vector<ShaderIOVariable> OutputVariables;
            uint32 ThreadGroupSizeX;
            uint32 ThreadGroupSizeY;
            uint32 ThreadGroupSizeZ;
        };

        // 着色器编译结果
        struct ShaderCompileResult {
            bool Success;
            std::vector<uint8> ByteCode;
            std::string ErrorMessage;
            ShaderReflection Reflection;
        };

        // 着色器编译器接口
        class IRHIShaderCompiler {
          public:
            virtual ~IRHIShaderCompiler() = default;

            // 编译着色器
            virtual ShaderCompileResult Compile(
                const ShaderCompileOptions& options) = 0;

            // 反射着色器
            virtual ShaderReflection Reflect(
                const std::vector<uint8>& byteCode) = 0;

            // 获取支持的目标平台
            virtual std::vector<EShaderTarget> GetSupportedTargets() const = 0;
        };

        // 创建着色器编译器
        IRHIShaderCompiler* CreateShaderCompiler();

    }  // namespace RHI
}  // namespace Engine
