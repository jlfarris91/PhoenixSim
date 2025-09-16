#include "FeaturePathfinding.h"

using namespace Phoenix;
using namespace Phoenix::Pathfinding;

const FName FeaturePathfinding::StaticName = "Pathfinding"_n;

FeaturePathfinding::FeaturePathfinding()
{
}

FName FeaturePathfinding::GetName() const
{
    return StaticName;
}

FeatureDefinition FeaturePathfinding::GetFeatureDefinition()
{
    return WorldFeatureDefinition;
}
