
#include "EntityId.h"

using namespace Phoenix::ECS;

PHOENIXECS_API const EntityId EntityId::Invalid = 0;

EntityId::operator entityid_t() const
{
    return Id;
}

EntityId& EntityId::operator=(const entityid_t& id)
{
    Id = id;
    return *this;
}