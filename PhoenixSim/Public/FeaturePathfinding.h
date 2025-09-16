#pragma once

#include "Features.h"

namespace Phoenix
{
    namespace Pathfinding
    {        
        class PHOENIXSIM_API FeaturePathfinding : public IFeature
        {
        public:

            static const FName StaticName;

            FeaturePathfinding();

            FName GetName() const override;

            FeatureDefinition GetFeatureDefinition() override;

        private:

            FeatureDefinition WorldFeatureDefinition;
        };
    }
}

