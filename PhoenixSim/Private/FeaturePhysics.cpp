
#include "FeaturePhysics.h"

#include <algorithm>

#include "MortonCode.h"
#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

const FName FeaturePhysics::StaticName = "Physics"_n;
const FName FeaturePhysicsScratchBlock::StaticName = "Physics_Scratch"_n;

void PhysicsSystem::OnUpdate(WorldRef world)
{
    float dt = 1 / 60.0f;

    ECSComponentAccessor<BodyComponent> componentsAccessor(world);
    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    TArray<BodyComponent*> sortedBodies;
    sortedBodies.reserve(ECS_MAX_ENTITIES);

    for (auto && [entity, body] : componentsAccessor)
    {
        uint32 x = static_cast<uint32>(body->Transform.Position.X) >> MortonCodeGridBits;
        uint32 y = static_cast<uint32>(body->Transform.Position.Y) >> MortonCodeGridBits;
        uint64 zcode = MortonCode(x, y);
        body->ZCode = zcode;
        sortedBodies.push_back(body);
    }

    // Sort bodies by their zcodes
    std::ranges::sort(sortedBodies,
        [](const BodyComponent* a, const BodyComponent* b)
        {
            return a->ZCode < b->ZCode;
        });

    scratchBlock.NumIterations = 0;
    scratchBlock.NumCollisions = 0;
    scratchBlock.MaxQueryBodyCount = 0;
    scratchBlock.ContactsSize = 0;

    std::vector<TTuple<uint64, uint64>> ranges;

    // Determine contacts
    for (uint32 i = 0; i < sortedBodies.size(); ++i)
    {
        BodyComponent* bodyA = sortedBodies[i];

        if (bodyA->Movement == EBodyMovement::Attached)
            continue;

        // Query for overlapping morton ranges
        {
            Distance radius = bodyA->Radius * 1.001f;
            uint32 lox = static_cast<uint32>(bodyA->Transform.Position.X - radius);
            uint32 hix = static_cast<uint32>(bodyA->Transform.Position.X + radius);
            uint32 loy = static_cast<uint32>(bodyA->Transform.Position.Y - radius);                
            uint32 hiy = static_cast<uint32>(bodyA->Transform.Position.Y + radius);                

            MortonCodeAABB aabb;
            aabb.MinX = lox >> MortonCodeGridBits;
            aabb.MinY = loy >> MortonCodeGridBits;
            aabb.MaxX = hix >> MortonCodeGridBits;
            aabb.MaxY = hiy >> MortonCodeGridBits;
        
            ranges.clear();
            MortonCodeQuery(aabb, ranges);
        }

        TArray<BodyComponent*> overlappingBodies;
        MortonCodeRangeIter<BodyComponent, &BodyComponent::ZCode>(sortedBodies, ranges, overlappingBodies);

        if (!overlappingBodies.empty())
        {
            scratchBlock.MaxQueryBodyCount = max(scratchBlock.MaxQueryBodyCount, overlappingBodies.size() - 1);
        }

        for (uint32 j = 0; j < overlappingBodies.size(); ++j)
        {
            BodyComponent* bodyB = overlappingBodies[j];

            if (scratchBlock.ContactsSize == scratchBlock.ContactsCapacity)
            {
                continue;
            }

            if (bodyA == bodyB)
            {
                continue;
            }

            if ((bodyA->CollisionMask & bodyB->CollisionMask) == 0)
            {
                continue;
            }
            
            ++scratchBlock.NumIterations;

            Vec2 v;
            if (Vec2::Equals(bodyA->Transform.Position, bodyB->Transform.Position))
            {
                v = Vec2::RandUnitVector();
                float correction = 1.0f / (bodyA->InvMass + bodyB->InvMass);
                bodyA->Transform.Position -= v * correction;
                bodyB->Transform.Position += v * correction;
            }

            v = bodyB->Transform.Position - bodyA->Transform.Position;
            
            float vLen = v.Length();
            float rr = bodyA->Radius + bodyB->Radius;
            if (vLen > rr)
            {
                continue;
            }
            
            const float baum = 0.3f;
            const float slop = 0.01f;
            float d = rr - vLen;
            float bias = baum * max(0, d - slop) / dt;
            float effMass = 1.0f / (bodyA->InvMass + bodyB->InvMass);

            Contact& contact = scratchBlock.Contacts[scratchBlock.ContactsSize++];
            contact.BodyA = bodyA;
            contact.BodyB = bodyB;
            contact.Normal = v.Normalized();
            contact.Bias = bias;
            contact.EffMass = effMass;
            contact.Impulse = 0;

            ++scratchBlock.NumCollisions;
        }
    }

    // Multi-pass solver
    for (uint32 iter = 0; iter < 2; ++iter)
    {
        for (uint32 c = 0; c < scratchBlock.ContactsSize; ++c)
        {
            Contact& contact = scratchBlock.Contacts[c];
            
            // Relative velocity at contact
            float relVel = Vec2::Dot(contact.Normal, contact.BodyB->Velocity - contact.BodyA->Velocity);

            // Compute corrective impulse
            float lambda = -(relVel + contact.Bias) * contact.EffMass;

            // Accumulate and project (no negative normal impulses)
            float oldImpulse = contact.Impulse;
            contact.Impulse = max(oldImpulse + lambda, 0.0f);
            float change = contact.Impulse - oldImpulse;

            // Apply impulse
            Vec2 p = contact.Normal * change;
            contact.BodyA->Velocity -= p * contact.BodyA->InvMass;
            contact.BodyB->Velocity += p * contact.BodyB->InvMass;
        }   
    }

    // Multi-pass resolve overlaps
    for (uint32 i = 0; i < 2; ++i)
    {
        for (uint32 c = 0; c < scratchBlock.ContactsSize; ++c)
        {
            Contact& contact = scratchBlock.Contacts[c];

            Vec2 v = contact.BodyB->Transform.Position - contact.BodyA->Transform.Position;
            float d = v.Length();
            float rr = contact.BodyA->Radius + contact.BodyB->Radius;
            float pen = rr - d;
            if (pen > 0.01)
            {
                float correction = 0.3f * pen;
                contact.BodyA->Transform.Position -= contact.Normal * correction * contact.BodyA->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);
                contact.BodyB->Transform.Position += contact.Normal * correction * contact.BodyB->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);
            }
        }
    }

    for (uint32 i = 0; i < sortedBodies.size(); ++i)
    {
        BodyComponent* body = sortedBodies[i];

        if (body->Movement == EBodyMovement::Attached)
        {
            body->Transform.Rotation += 10.0f;
        }
        else
        {
            body->Transform.Position += body->Velocity * dt;
        }
    }
}

FeaturePhysics::FeaturePhysics()
{
    WorldBufferBlockArgs blockArgs;
    blockArgs.Name = FeaturePhysicsScratchBlock::StaticName;
    blockArgs.Size = sizeof(FeaturePhysicsScratchBlock);
    blockArgs.BlockType = EWorldBufferBlockType::Scratch;

    WorldFeatureDefinition.Name = StaticName;
    WorldFeatureDefinition.Blocks.push_back(blockArgs);
}

FName FeaturePhysics::GetName() const
{
    return StaticName;
}

FeatureDefinition FeaturePhysics::GetFeatureDefinition()
{
    return WorldFeatureDefinition;
}

void FeaturePhysics::Initialize()
{
    std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();

    TSharedPtr<FeatureECS> featureECS = Session->GetFeatureSet()->GetFeature<FeatureECS>();
    featureECS->RegisterSystem(physicsSystem);
}
