
#include "FeatureECS2.h"

using namespace Phoenix;
using namespace Phoenix::ECS2;


struct TransformComponent2
{
    static constexpr FName StaticName = "TransformComponent"_n;
};

struct BodyComponent2
{
    static constexpr FName StaticName = "BodyComponent"_n;
};

using ArchetypeDefinition = TArchetypeDefinition<8>;
using ArchetypeList = TArchetypeList<ArchetypeDefinition, 16000>;
using EntityManager = TEntityManager<ArchetypeDefinition, 32, 1024, 16000>;

EntityManager* GEntityManager = nullptr;

void ECS2::Test()
{
    GEntityManager = new EntityManager();

    ComponentDefinition defAComps[] =
    {
        { TransformComponent2::StaticName, sizeof(TransformComponent2) },
        { BodyComponent2::StaticName, sizeof(BodyComponent2) },
    };

    ArchetypeDefinition defA(defAComps, _countof(defAComps));
    auto asdf = Hashing::FN1VA32Combine(0, (hash32_t)TransformComponent2::StaticName, (hash32_t)BodyComponent2::StaticName);
    PHX_ASSERT((hash32_t)defA.Id == asdf);

    auto entityHandle = GEntityManager->AcquireEntity(defA.Id);
    PHX_ASSERT(!GEntityManager->IsValid(entityHandle));

    GEntityManager->ArchetypeDefinitions.Insert(defA.Id, defA);

    entityHandle = GEntityManager->AcquireEntity(defA.Id);
    PHX_ASSERT(GEntityManager->IsValid(entityHandle));
}
