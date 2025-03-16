
#include "VulkanShaderCompiler.h"

#include <cassert>
#include <fstream>
#include <sstream>

namespace Engine {
    namespace RHI {

        VulkanShaderCompiler::VulkanShaderCompiler() {
            // 设置默认编译选项
            m_CompileOptions.SetTargetEnvironment(
                shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
            m_CompileOptions.SetOptimizationLevel(
                shaderc_optimization_level_performance);
            m_CompileOptions.SetGenerateDebugInfo();
        }

        VulkanShaderCompiler::~VulkanShaderCompiler() {}

        ShaderCompileResult VulkanShaderCompiler::Compile(
            const ShaderCompileOptions& options) {
            ShaderCompileResult result;

            // 只支持SPIR-V目标
            if (options.Target != EShaderTarget::SPIRV) {
                result.Success = false;
                result.ErrorMessage =
                    "VulkanShaderCompiler only supports SPIR-V target";
                return result;
            }

            return CompileSPIRV(options);
        }

        ShaderCompileResult VulkanShaderCompiler::CompileSPIRV(
            const ShaderCompileOptions& options) {
            ShaderCompileResult result;

            // 准备源代码
            std::string sourceCode;
            if (!options.SourceText.empty()) {
                sourceCode = options.SourceText;
            } else if (!options.SourceFile.empty()) {
                std::ifstream file(options.SourceFile);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    sourceCode = buffer.str();
                } else {
                    result.Success = false;
                    result.ErrorMessage =
                        "Failed to open source file: " + options.SourceFile;
                    return result;
                }
            } else {
                result.Success = false;
                result.ErrorMessage = "No shader source provided";
                return result;
            }

            // 设置编译选项
            shaderc::CompileOptions compileOptions = m_CompileOptions;

            // 添加宏定义
            for (const auto& define : options.Defines) {
                size_t pos = define.find('=');
                if (pos != std::string::npos) {
                    compileOptions.AddMacroDefinition(define.substr(0, pos),
                                                      define.substr(pos + 1));
                } else {
                    compileOptions.AddMacroDefinition(define);
                }
            }

            // 设置优化级别
            if (options.Debug) {
                compileOptions.SetOptimizationLevel(
                    shaderc_optimization_level_zero);
            } else {
                compileOptions.SetOptimizationLevel(
                    static_cast<shaderc_optimization_level>(
                        options.OptimizationLevel));
            }

            // 设置包含目录处理
            if (!options.IncludeDirs.empty()) {
                class CustomIncluder
                    : public shaderc::CompileOptions::IncluderInterface {
                  public:
                    explicit CustomIncluder(
                        const std::vector<std::string>& includeDirs)
                        : m_IncludeDirs(includeDirs) {}

                    shaderc_include_result* GetInclude(
                        const char* requested_source,
                        [[maybe_unused]] shaderc_include_type type,
                        [[maybe_unused]] const char* requesting_source,
                        [[maybe_unused]] size_t include_depth) override {
                        // 在所有包含目录中查找文件
                        for (const auto& dir : m_IncludeDirs) {
                            std::string fullPath = dir + "/" + requested_source;
                            std::ifstream file(fullPath);
                            if (file.is_open()) {
                                std::string content(
                                    (std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());

                                auto* data = new char[content.size()];
                                std::copy(content.begin(), content.end(), data);

                                auto* result = new shaderc_include_result;
                                result->source_name = strdup(fullPath.c_str());
                                result->source_name_length = fullPath.size();
                                result->content = data;
                                result->content_length = content.size();
                                result->user_data = nullptr;

                                return result;
                            }
                        }
                        return nullptr;
                    }

                    void ReleaseInclude(shaderc_include_result* data) override {
                        if (data) {
                            delete[] data->source_name;
                            delete[] data->content;
                            delete data;
                        }
                    }

                  private:
                    std::vector<std::string> m_IncludeDirs;
                };

                compileOptions.SetIncluder(
                    std::make_unique<CustomIncluder>(options.IncludeDirs));
            }

            // 编译着色器
            shaderc::SpvCompilationResult module =
                m_Compiler.CompileGlslToSpv(sourceCode,
                                            GetShaderKind(options.EntryPoint),
                                            options.SourceFile.c_str(),
                                            options.EntryPoint.c_str(),
                                            compileOptions);

            // 检查编译结果
            if (module.GetCompilationStatus() !=
                shaderc_compilation_status_success) {
                result.Success = false;
                result.ErrorMessage = module.GetErrorMessage();
                return result;
            }

            // 获取SPIR-V字节码
            std::vector<uint32_t> spirv(module.cbegin(), module.cend());
            result.ByteCode.resize(spirv.size() * sizeof(uint32_t));
            memcpy(
                result.ByteCode.data(), spirv.data(), result.ByteCode.size());

            // 进行反射
            result.Reflection = Reflect(result.ByteCode);
            result.Success = true;

            return result;
        }

        ShaderReflection VulkanShaderCompiler::Reflect(
            const std::vector<uint8>& byteCode) {
            ShaderReflection reflection;

            // 创建SPIRV-Cross编译器
            spirv_cross::Compiler compiler(
                reinterpret_cast<const uint32_t*>(byteCode.data()),
                byteCode.size() / sizeof(uint32_t));

            // 反射资源绑定
            ReflectResourceBindings(compiler, reflection);

            // 反射常量缓冲区
            ReflectConstantBuffers(compiler, reflection);

            // 反射输入输出变量
            ReflectIOVariables(compiler, reflection);

            // 反射计算着色器属性
            ReflectComputeShaderProperties(compiler, reflection);

            return reflection;
        }

        std::vector<EShaderTarget> VulkanShaderCompiler::GetSupportedTargets()
            const {
            return {EShaderTarget::SPIRV};
        }

        shaderc_shader_kind VulkanShaderCompiler::GetShaderKind(
            const std::string& entryPoint) {
            // 根据入口点名称推断着色器类型
            if (entryPoint.find("vs_") == 0) return shaderc_vertex_shader;
            if (entryPoint.find("ps_") == 0) return shaderc_fragment_shader;
            if (entryPoint.find("cs_") == 0) return shaderc_compute_shader;
            if (entryPoint.find("gs_") == 0) return shaderc_geometry_shader;
            if (entryPoint.find("hs_") == 0) return shaderc_tess_control_shader;
            if (entryPoint.find("ds_") == 0)
                return shaderc_tess_evaluation_shader;
            if (entryPoint.find("ms_") == 0) return shaderc_mesh_shader;
            if (entryPoint.find("as_") == 0) return shaderc_task_shader;
            if (entryPoint.find("rgen") == 0) return shaderc_raygen_shader;
            if (entryPoint.find("rchit") == 0) return shaderc_closesthit_shader;
            if (entryPoint.find("rmiss") == 0) return shaderc_miss_shader;

            return shaderc_glsl_default_vertex_shader;
        }

        void VulkanShaderCompiler::ReflectResourceBindings(
            const spirv_cross::Compiler& compiler,
            ShaderReflection& reflection) {
            // 获取所有资源
            spirv_cross::ShaderResources resources =
                compiler.get_shader_resources();

            // 反射统一缓冲区
            for (const auto& resource : resources.uniform_buffers) {
                ShaderResourceBinding binding;
                binding.Name = resource.name;
                binding.Type = EDescriptorRangeType::CBV;
                binding.BindPoint = compiler.get_decoration(
                    resource.id, spv::DecorationBinding);
                binding.Space = compiler.get_decoration(
                    resource.id, spv::DecorationDescriptorSet);
                binding.BindCount = 1;
                reflection.ResourceBindings.push_back(binding);
            }

            // 反射采样器
            for (const auto& resource : resources.sampled_images) {
                ShaderResourceBinding binding;
                binding.Name = resource.name;
                binding.Type = EDescriptorRangeType::SRV;
                binding.BindPoint = compiler.get_decoration(
                    resource.id, spv::DecorationBinding);
                binding.Space = compiler.get_decoration(
                    resource.id, spv::DecorationDescriptorSet);
                binding.BindCount = 1;
                reflection.ResourceBindings.push_back(binding);
            }

            // 反射存储图像
            for (const auto& resource : resources.storage_images) {
                ShaderResourceBinding binding;
                binding.Name = resource.name;
                binding.Type = EDescriptorRangeType::UAV;
                binding.BindPoint = compiler.get_decoration(
                    resource.id, spv::DecorationBinding);
                binding.Space = compiler.get_decoration(
                    resource.id, spv::DecorationDescriptorSet);
                binding.BindCount = 1;
                reflection.ResourceBindings.push_back(binding);
            }
        }

        void VulkanShaderCompiler::ReflectConstantBuffers(
            const spirv_cross::Compiler& compiler,
            ShaderReflection& reflection) {
            spirv_cross::ShaderResources resources =
                compiler.get_shader_resources();

            for (const auto& resource : resources.uniform_buffers) {
                ShaderConstantBuffer cbuffer;
                cbuffer.Name = resource.name;
                cbuffer.BindPoint = compiler.get_decoration(
                    resource.id, spv::DecorationBinding);
                cbuffer.Space = compiler.get_decoration(
                    resource.id, spv::DecorationDescriptorSet);

                // 获取缓冲区类型
                const spirv_cross::SPIRType& type =
                    compiler.get_type(resource.base_type_id);
                cbuffer.Size = compiler.get_declared_struct_size(type);

                // 获取成员
                for (uint32_t i = 0; i < type.member_types.size(); ++i) {
                    std::string memberName =
                        compiler.get_member_name(resource.base_type_id, i);
                    uint32_t offset =
                        compiler.type_struct_member_offset(type, i);
                    cbuffer.Members.push_back({memberName, offset});
                }

                reflection.ConstantBuffers.push_back(cbuffer);
            }
        }

        void VulkanShaderCompiler::ReflectIOVariables(
            const spirv_cross::Compiler& compiler,
            ShaderReflection& reflection) {
            spirv_cross::ShaderResources resources =
                compiler.get_shader_resources();

            // 反射输入变量
            for (const auto& input : resources.stage_inputs) {
                ShaderIOVariable var;
                var.Name = input.name;
                var.Location =
                    compiler.get_decoration(input.id, spv::DecorationLocation);

                const spirv_cross::SPIRType& type =
                    compiler.get_type(input.type_id);
                var.Format = ConvertSPIRVFormat(type.basetype, type.vecsize);

                reflection.InputVariables.push_back(var);
            }

            // 反射输出变量
            for (const auto& output : resources.stage_outputs) {
                ShaderIOVariable var;
                var.Name = output.name;
                var.Location =
                    compiler.get_decoration(output.id, spv::DecorationLocation);

                const spirv_cross::SPIRType& type =
                    compiler.get_type(output.type_id);
                var.Format = ConvertSPIRVFormat(type.basetype, type.vecsize);

                reflection.OutputVariables.push_back(var);
            }
        }

        void VulkanShaderCompiler::ReflectComputeShaderProperties(
            const spirv_cross::Compiler& compiler,
            ShaderReflection& reflection) {
            // 获取计算着色器工作组大小
            const auto& entryPoints = compiler.get_entry_points_and_stages();
            if (!entryPoints.empty()) {
                spirv_cross::SpecializationConstant x, y, z;
                compiler.get_work_group_size_specialization_constants(x, y, z);

                // 将specialization constant的值转换为uint32
                reflection.ThreadGroupSizeX =
                    compiler.get_constant(x.id).scalar();
                reflection.ThreadGroupSizeY =
                    compiler.get_constant(y.id).scalar();
                reflection.ThreadGroupSizeZ =
                    compiler.get_constant(z.id).scalar();
            }
        }

        EVertexElementFormat VulkanShaderCompiler::ConvertSPIRVFormat(
            spirv_cross::SPIRType::BaseType baseType, uint32 vecSize) {
            switch (baseType) {
                case spirv_cross::SPIRType::Float:
                    switch (vecSize) {
                        case 1:
                            return EVertexElementFormat::Float1;
                        case 2:
                            return EVertexElementFormat::Float2;
                        case 3:
                            return EVertexElementFormat::Float3;
                        case 4:
                            return EVertexElementFormat::Float4;
                    }
                    break;

                case spirv_cross::SPIRType::Int:
                    switch (vecSize) {
                        case 1:
                            return EVertexElementFormat::Int1;
                        case 2:
                            return EVertexElementFormat::Int2;
                        case 3:
                            return EVertexElementFormat::Int3;
                        case 4:
                            return EVertexElementFormat::Int4;
                    }
                    break;

                case spirv_cross::SPIRType::UInt:
                    switch (vecSize) {
                        case 1:
                            return EVertexElementFormat::
                                Int1;  // 使用Int1代替UInt
                        case 2:
                            return EVertexElementFormat::
                                Int2;  // 使用Int2代替UInt2
                        case 3:
                            return EVertexElementFormat::
                                Int3;  // 使用Int3代替UInt3
                        case 4:
                            return EVertexElementFormat::
                                Int4;  // 使用Int4代替UInt4
                    }
                    break;

                default:
                    return EVertexElementFormat::
                        Float1;  // 使用Float1作为默认值
            }

            return EVertexElementFormat::Float1;
        }
    }  // namespace RHI
}  // namespace Engine