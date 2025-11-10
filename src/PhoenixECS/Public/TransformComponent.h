
#pragma once

#include "Component.h"
#include "DLLExport.h"
#include "FixedPoint/FixedTransform.h"

namespace Phoenix
{
    namespace ECS
    {
        struct PHOENIXECS_API TransformComponent : IComponent
        {
        public:
            using ThisType = TransformComponent; 
            using BaseType = IComponent; 
            static constexpr FName StaticTypeName = "TransformComponent"_n; 
            static const TypeDescriptor& GetStaticTypeDescriptor()
            {
                static TypeDescriptor sd = ThisType::STypeDescriptor::Construct(); 
                return sd; 
            }
            const TypeDescriptor& GetTypeDescriptor() const override
            {
                return ThisType::GetStaticTypeDescriptor(); 
            }
        private:
            struct STypeDescriptor
            {
                static constexpr FName StaticName = "TransformComponent"_n; 
                static constexpr const char* StaticDisplayName = "TransformComponent"; 
                static TypeDescriptor Construct()
                {
                    TypeDescriptor definition; 
                    definition.CName = "TransformComponent"; 
                    definition.Name = StaticName; 
                    definition.DisplayName = StaticDisplayName; 
                    definition.DefaultConstructFunc = &TTypeHelper<TransformComponent>::DefaultConstruct; 
                    definition.DestructFunc = &TTypeHelper<TransformComponent>::Destruct; 
                    definition.Size = sizeof(ThisType); 
                    definition.RegisterBase<IComponent>(); 
                    return definition; 
                }
            };
        public:

            // The id of another entity that the owning entity is attached to.
            // Note that this cannot be the entity that owns the body component.
            EntityId AttachParent = EntityId::Invalid;

            // The relative transform of the entity.
            // Relative to the origin if not attached to another entity.
            Transform2D Transform;

            // Morton z-code used for spacial sorting of entities.
            uint64 ZCode = 0;
        };

        struct PHOENIXECS_API EntityTransform
        {
            EntityId EntityId;
            TransformComponent* TransformComponent;
            uint64 ZCode = 0;
        };
    }
}
