
#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // VRS着色率
        enum class EShadingRate : uint8 {
            Rate1x1,  // 每像素着色
            Rate1x2,  // 每2像素着色一次（水平）
            Rate2x1,  // 每2像素着色一次（垂直）
            Rate2x2,  // 每4像素着色一次
            Rate2x4,  // 每8像素着色一次
            Rate4x2,  // 每8像素着色一次
            Rate4x4,  // 每16像素着色一次
        };

        // VRS组合器
        enum class EShadingRateCombiner : uint8 {
            Override,  // 使用第二个着色率
            Min,       // 使用较小的着色率
            Max,       // 使用较大的着色率
            Sum,       // 将两个着色率相加
            Multiply,  // 将两个着色率相乘
        };

        // VRS图案描述
        struct ShadingRatePatternDesc {
            uint32 Width;
            uint32 Height;
            std::vector<EShadingRate> Pattern;
            std::string DebugName;
        };

        // VRS图案接口
        class IRHIShadingRatePattern : public IRHIResource {
          public:
            virtual ~IRHIShadingRatePattern() = default;

            // 获取描述
            virtual const ShadingRatePatternDesc& GetDesc() const = 0;
        };

        // VRS区域
        struct ShadingRateRegion {
            uint32 X;
            uint32 Y;
            uint32 Width;
            uint32 Height;
            EShadingRate Rate;
        };

        // VRS能力
        struct VariableRateShadingCapabilities {
            bool SupportsVariableRateShading;
            bool SupportsPerDrawVRS;
            bool SupportsTextureBasedVRS;
            uint32 MaxShadingRatePatternSize;
            std::vector<EShadingRate> SupportedShadingRates;
            std::vector<EShadingRateCombiner> SupportedCombiners;
        };

    }  // namespace RHI
}  // namespace Engine
