
#include "ArchetypeManager.h"

#include "FeatureECS2.h"

using namespace Phoenix;
using namespace Phoenix::ECS2;

ArchetypeManager* GEntityManager;

struct TransformComponent2 : IComponent
{
    PHX_DECLARE_TYPE(TransformComponent2)
    uint32 Foo = 123;
};

struct BodyComponent2 : IComponent
{
    PHX_DECLARE_TYPE(BodyComponent2)
    uint32 Bar = 456;
};

void ECS2::Test(Session* session)
{
    GEntityManager = new ArchetypeManager();

    ComponentDefinition defAComps[] =
    {
        { TransformComponent2::StaticTypeName, sizeof(TransformComponent2), 0, &TransformComponent2::GetStaticTypeDescriptor() },
        { BodyComponent2::StaticTypeName, sizeof(BodyComponent2), 0, &BodyComponent2::GetStaticTypeDescriptor() },
    };

    ArchetypeDefinition defA(defAComps, _countof(defAComps));
    auto defAIdExpected = Hashing::FN1VA32Combine(0, (hash32_t)TransformComponent2::StaticTypeName, (hash32_t)BodyComponent2::StaticTypeName);
    PHX_ASSERT((hash32_t)defA.Id == defAIdExpected);

    EntityId entityId = 123;

    auto entityHandle = GEntityManager->Acquire(entityId, defA.Id);
    PHX_ASSERT(!GEntityManager->IsValid(entityHandle));

    GEntityManager->RegisterArchetypeDefinition(defA);

    entityHandle = GEntityManager->Acquire(entityId, defA.Id);
    PHX_ASSERT(GEntityManager->IsValid(entityHandle));

    auto work = [](WorldRef, EntityId, const BodyComponent2& body, TransformComponent2& transform)
    {
        transform.Foo = body.Bar / 2;
    };

    GEntityManager->Entities()
        .RequireAllTags("TestTag0"_n, "TestTag1"_n)
        .RequireAnyTags("TestTag0"_n, "TestTag1"_n)
        .RequireNoneTags("TestTag0"_n, "TestTag1"_n)
        .RequireAllComponents<TransformComponent2, BodyComponent2>()
        .RequireAnyComponents<TransformComponent2, BodyComponent2>()
        .RequireNoneComponents<TransformComponent2, BodyComponent2>()
        .ScheduleParallel(TFunction(work));
    
    auto work2 = [](WorldRef, const EntityComponentSpan<const BodyComponent2&, TransformComponent2&>& span)
    {
        for (auto && [id, body, transform] : span)
        {
            transform.Foo = body.Bar / 2;
        }
    };
    
    GEntityManager->Entities()
        .RequireAllTags("TestTag0"_n, "TestTag1"_n)
        .RequireAnyTags("TestTag0"_n, "TestTag1"_n)
        .RequireNoneTags("TestTag0"_n, "TestTag1"_n)
        .RequireAllComponents<TransformComponent2, BodyComponent2>()
        .RequireAnyComponents<TransformComponent2, BodyComponent2>()
        .RequireNoneComponents<TransformComponent2, BodyComponent2>()
        .ScheduleParallel(TFunction(work2));

    auto world = session->GetWorldManager()->GetPrimaryWorld();
    GEntityManager->ProcessQueries(*world);
}