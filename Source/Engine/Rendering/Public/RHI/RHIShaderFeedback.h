
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 着色器反馈标志
        enum class EShaderFeedbackFlags : uint32 {
            None = 0,
            MinMipLevel = 1 << 0,
            MipRegion = 1 << 1,
            TextureRegion = 1 << 2,
        };

        // 着色器反馈描述
        struct ShaderFeedbackDesc {
            EShaderFeedbackFlags Flags;
            uint32 SampleCount;
            uint32 MaxEntries;
            std::string DebugName;
        };

        // 着色器反馈缓冲区接口
        class IRHIShaderFeedbackBuffer : public IRHIResource {
          public:
            virtual ~IRHIShaderFeedbackBuffer() = default;

            // 获取描述
            virtual const ShaderFeedbackDesc& GetDesc() const = 0;

            // 获取反馈数据
            virtual void GetFeedbackData(void* data, uint32 size) = 0;

            // 重置缓冲区
            virtual void Reset() = 0;
        };

        // 条件渲染谓词类型
        enum class EPredicateType : uint8 {
            Binary,        // 二进制谓词
            Numeric,       // 数值谓词
            CompareValue,  // 比较值谓词
        };

        // 条件渲染谓词描述
        struct PredicateDesc {
            EPredicateType Type;
            union {
                struct {
                    bool Value;
                } Binary;
                struct {
                    uint64 Value;
                    uint64 Reference;
                    ECompareFunction CompareOp;
                } Numeric;
            };
            std::string DebugName;
        };

        // 条件渲染谓词接口
        class IRHIPredicate : public IRHIResource {
          public:
            virtual ~IRHIPredicate() = default;

            // 获取描述
            virtual const PredicateDesc& GetDesc() const = 0;

            // 设置谓词值
            virtual void SetValue(const PredicateDesc& value) = 0;
        };

        // 保守光栅化模式
        enum class EConservativeRasterMode : uint8 {
            Off,            // 禁用保守光栅化
            Underestimate,  // 低估覆盖
            Overestimate,   // 高估覆盖
        };

        // 保守光栅化描述
        struct ConservativeRasterDesc {
            EConservativeRasterMode Mode;
            float DilationValue;  // 扩张值（仅用于Overestimate模式）
        };

    }  // namespace RHI
}  // namespace Engine
