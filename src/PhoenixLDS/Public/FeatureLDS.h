
#pragma once

#include "DLLExport.h"
#include "Features.h"

namespace Phoenix::LDS
{
    struct FeatureLDSStaticBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_STATIC(FeatureLDSStaticBlock)
    };

    struct FeatureLDSDynamicBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_STATIC(FeatureLDSDynamicBlock)
    };

    class PHOENIX_LDS_API FeatureLDS : public IFeature
    {
    public:

        PHX_FEATURE_BEGIN(FeatureLDS)
            FEATURE_SESSION_BLOCK(FeatureLDSStaticBlock)
            FEATURE_SESSION_BLOCK(FeatureLDSDynamicBlock)
        PHX_FEATURE_END()

        void Initialize() override;
        void Shutdown() override;
    };
    
}
