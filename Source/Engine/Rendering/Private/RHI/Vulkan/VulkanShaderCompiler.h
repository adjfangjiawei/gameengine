
#pragma once

#include <vulkan/vulkan.h>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

#include "RHI/RHIShaderCompiler.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan着色器编译器实现
        class VulkanShaderCompiler : public IRHIShaderCompiler {
          public:
            VulkanShaderCompiler();
            virtual ~VulkanShaderCompiler();

            // IRHIShaderCompiler接口实现
            virtual ShaderCompileResult Compile(
                const ShaderCompileOptions& options) override;
            virtual ShaderReflection Reflect(
                const std::vector<uint8>& byteCode) override;
            virtual std::vector<EShaderTarget> GetSupportedTargets()
                const override;

          private:
            // 内部编译方法
            ShaderCompileResult CompileSPIRV(
                const ShaderCompileOptions& options);

            // 着色器类型转换
            shaderc_shader_kind GetShaderKind(const std::string& entryPoint);

            // 反射辅助方法
            void ReflectResourceBindings(const spirv_cross::Compiler& compiler,
                                         ShaderReflection& reflection);
            void ReflectConstantBuffers(const spirv_cross::Compiler& compiler,
                                        ShaderReflection& reflection);
            void ReflectIOVariables(const spirv_cross::Compiler& compiler,
                                    ShaderReflection& reflection);
            void ReflectComputeShaderProperties(
                const spirv_cross::Compiler& compiler,
                ShaderReflection& reflection);

            // 格式转换
            EVertexElementFormat ConvertSPIRVFormat(
                spirv_cross::SPIRType::BaseType baseType, uint32 vecSize);
            EDescriptorRangeType ConvertSPIRVResourceType(
                spirv_cross::SPIRType::BaseType baseType);

            // 编译器实例
            shaderc::Compiler m_Compiler;
            shaderc::CompileOptions m_CompileOptions;
        };

        // 着色器编译器工厂函数实现
        IRHIShaderCompiler* CreateShaderCompiler() {
            return new VulkanShaderCompiler();
        }

    }  // namespace RHI
}  // namespace Engine
