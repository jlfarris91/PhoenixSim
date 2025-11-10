
#pragma once

#include "EntityId.h"
#include "EntityTag.h"
#include "Containers/FixedArray.h"

namespace Phoenix
{
    namespace ECS
    {
        template <size_t N>
        class FixedTagList
        {
        public:

            constexpr size_t Num() const
            {
                return Tags.Num();
            }

            bool HasTag(const Entity& entity, const FName& tagName) const
            {
                bool foundTag = false;
                ForEachTag(entity, [&](const EntityTag& tag, int32)
                {
                    if (tag.TagName == tagName)
                    {
                        foundTag = true;
                        return false;
                    }
                    return true;
                });
                return foundTag;
            }

            bool AddTag(Entity& entity, const FName& tagName)
            {
                int32 newTagIndex = INDEX_NONE;

                // Find the next available tag index
                {
                    for (int32 i = 1; i < N; ++i)
                    {
                        if (Tags[i].TagName == FName::None)
                        {
                            newTagIndex = i;
                            break;
                        }
                    }

                    if (newTagIndex == INDEX_NONE)
                    {
                        return false;
                    }
                }

                if (entity.TagHead == INDEX_NONE)
                {
                    entity.TagHead = newTagIndex;
                }
                else
                {
                    // Find the tail tag and update it's next
                    int32 tagIter = entity.TagHead;
                    while (Tags[tagIter].Next != INDEX_NONE)
                    {
                        // Entity already has the tag, don't add duplicates
                        if (Tags[tagIter].TagName == tagName)
                        {
                            return false;
                        }

                        tagIter = Tags[tagIter].Next;
                    }
                    Tags[tagIter].Next = newTagIndex;
                }

                // Make room for the new tag if necessary
                if (!Tags.IsValidIndex(newTagIndex))
                {
                    Tags.SetNum(newTagIndex + 1);
                }

                EntityTag& tag = Tags[newTagIndex];
                tag.Next = INDEX_NONE;
                tag.TagName = tagName;

                return true;
            }

            bool RemoveTag(Entity& entity, FName tagName)
            {
                int32 prevTagIndex = INDEX_NONE;
                int32 tagIndex = INDEX_NONE;
                ForEachTag(entity, [&, tagName](const EntityTag& tag, uint32 index)
                {
                    tagIndex = index;
                    if (tag.TagName == tagName)
                    {
                        return false;
                    }
                    prevTagIndex = tagIndex;
                    return true;
                });

                if (tagIndex == INDEX_NONE)
                {
                    return false;
                }

                EntityTag& tagToRemove = Tags[tagIndex];

                if (prevTagIndex != INDEX_NONE)
                {
                    EntityTag& prevTag = Tags[prevTagIndex];
                    prevTag.Next = tagToRemove.Next;
                }

                if (entity.TagHead == tagIndex)
                {
                    entity.TagHead = prevTagIndex;
                }

                // Reset tag data
                tagToRemove.TagName = FName::None;
                tagToRemove.Next = INDEX_NONE;

                return true;
                
            }

            uint32 RemoveAllTags(Entity& entity)
            {
                int32 tagIndex = entity.TagHead;

                uint32 numTagsRemoved = 0;
                while (tagIndex != INDEX_NONE)
                {
                    EntityTag& tag = Tags[tagIndex];

                    tagIndex = tag.Next;
                    numTagsRemoved++;

                    // Reset tag data
                    tag.TagName = FName::None;
                    tag.Next = INDEX_NONE;
                }

                entity.TagHead = INDEX_NONE;

                return numTagsRemoved;
            }

            template <class TCallback>
            void ForEachTag(const Entity& entity, const TCallback& callback) const
            {
                int32 tagIndex = entity.TagHead;
                while (tagIndex != INDEX_NONE)
                {
                    const EntityTag& tag = Tags[tagIndex];
                    if (!callback(tag, tagIndex))
                    {
                        return;
                    }
                    tagIndex = tag.Next;
                }
            }

        private:

            TFixedArray<EntityTag, N> Tags;
        };
    }
}
