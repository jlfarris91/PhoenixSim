

#include "FixedLDS.h"

using namespace nlohmann;

namespace Phoenix::LDS
{
    bool FromJson(
        const json& json,
        ELDSValueType type,
        LDSValue& outValue)
    {
        switch (type)
        {
        case ELDSValueType::Int32:
            if (json.is_number_integer())
            {
                outValue.Int32 = json.get<int32>();
                return true;
            }
            break;
        case ELDSValueType::UInt32:
            if (json.is_number_integer())
            {
                outValue.UInt32 = json.get<uint32>();
                return true;
            }
            break;
        case ELDSValueType::Name:
            if (json.is_string())
            {
                const auto& str = json.get<PHXString>();
                outValue.Name = Hashing::FNV1A32(str.data(), str.length());
                return true;
            }
            if (json.is_number_integer())
            {
                outValue.Name = json.get<hash32_t>();
                return true;
            }
            break;
        case ELDSValueType::Value:
            if (json.is_number_float())
            {
                outValue.Value = json.get<double>();
                return true;
            }
            break;
        case ELDSValueType::Distance:
            if (json.is_number_float())
            {
                outValue.Distance = json.get<double>();
                return true;
            }
            break;
        case ELDSValueType::Degrees:
            if (json.is_number_float())
            {
                outValue.Degrees = json.get<double>();
                return true;
            }
            break;
        case ELDSValueType::Speed:
            if (json.is_number_float())
            {
                outValue.Speed = json.get<double>();
                return true;
            }
            break;
        case ELDSValueType::Bool:
            if (json.is_boolean())
            {
                outValue.Bool = json.get<bool>();
                return true;
            }
            break;
        case ELDSValueType::Object:
            break;
        }

        return false;
    }
    
    bool FromJson(
        const TFixedLDS<64>& lds,
        const TFixedLDS<64>& metadata,
        const json& json,
        const char* pointerStr,
        uint32 pointerFirst,
        uint32 pointerLast,
        LDSTypedValue& outValue)
    {
        const char* subPointer = pointerStr + pointerFirst;
        uint32 subPointerLen = pointerLast - pointerFirst;

        auto recordId = Hashing::FNV1A32(subPointer, subPointerLen);
        auto typeRecordId = Hashing::FN1VA32Combine(recordId, "/type");
        
        const LDSRecord* record1 = metadata.FindRecord(recordId);
        const LDSRecord* record2 = metadata.FindRecord(typeRecordId);


        return true;
    }

    bool FromJson(
        const TFixedLDS<64>& lds,
        const TFixedLDS<64>& metadata,
        const json& json,
        const json::json_pointer& pointer,
        LDSTypedValue& outValue)
    {
        PHXString parts[64];
        uint32 partsLen = 0;
        
        json::json_pointer p = pointer;
        while (!p.empty() && partsLen < _countof(parts))
        {
            parts[partsLen++] = p.back();
            p = p.parent_pointer();
        }

        if (partsLen == _countof(parts))
        {
            // Path is too long
            return false;
        }

        PHXString pointerStr = pointer.to_string();
        uint32 end = static_cast<uint32>(pointerStr.find_first_of('/', 1));
        if (end == Index<uint32>::None)
            end = static_cast<uint32>(pointerStr.length());
        return FromJson(lds, metadata, json, pointerStr.data(), 0, end, outValue);
    }

    bool ProcessType(TFixedLDS<64>& metadata, const json& typeJson)
    {
        auto idIter = typeJson.find("id");
        if (idIter == typeJson.end())
        {
            // Error bad data
            return false;
        }

        PHXString typeIdStr = idIter->get<PHXString>();

        FName typeId = FName(typeIdStr.data(), typeIdStr.length());

        auto typeIter = typeJson.find("type");
        if (typeIter == typeJson.end())
        {
            // Error bad data
            return false;
        }

        PHXString typeStr = typeIter->get<PHXString>();

        // Referencing another defined object
        if (typeStr[0] == '#')
        {
            
        }

        // Defining a new object inline
        if (typeStr == "Object")
        {
            const json& props = typeJson["properties"];
            for (auto && [propName, propValue] : props.items())
            {
                PHXString propPtr = "/" + propName;

                for (auto && [metaName, metaValue] : propValue.items())
                {
                    PHXString fieldPtr = propPtr + "/" + metaName;
                    FName fieldId = FName(fieldPtr.data(), fieldPtr.length());

                    if (metaName == "type")
                    {
                        ELDSValueType type = ELDSValueType::Int32;
                        bool success = TryParse(metaValue.get<PHXString>(), type);
                        if (success)
                        {
                            LDSTypedValue value = { LDSValue((uint32)type), ELDSValueType::UInt32 };
                            metadata.EmplaceRecord_GetRef(typeId, fieldId, value);
                        }
                    }

                    if (metaName == "min")
                    {
                        int32 minVal = metaValue.get<int32>();
                        LDSTypedValue value = { LDSValue(minVal), ELDSValueType::Int32 };
                        metadata.EmplaceRecord_GetRef(typeId, fieldId, value);
                    }

                    if (metaName == "max")
                    {
                        LDSTypedValue value;
                        value.Type = ELDSValueType::Int32;
                        value.Value.Int32 = metaValue.get<int32>();
                        metadata.EmplaceRecord_GetRef(typeId, fieldId, value);
                    }

                    if (metaName == "default")
                    {
                        ELDSValueType type = ELDSValueType::Int32;
                        bool success = TryParse(propValue["type"].get<PHXString>(), type);
                        if (success)
                        {
                            LDSTypedValue value = { {}, type };
                            success = FromJson(metaValue, type, value.Value);
                            PHX_ASSERT(success);

                            metadata.EmplaceRecord_GetRef(typeId, fieldId, value);   
                        }
                    }
                }
            }
        }

        if (typeStr == "String")
        {
            
        }

        return true;
    }

    void Test()
    {
        TFixedLDS<64> lds;
        TFixedLDS<64> metadata;

        json colorJson = R"(
        {
            "id": "color",
            "type": "String",
            "format": "((?:[A-F]|[0-9])(?:[A-F]|[0-9])){3,4}",
            "min_len": 6,
            "max_len": 8
        }
        )"_json;

        json unitJson = R"(
        {
            "id": "Unit",
            "type": "Object",
            "properties": {
                "testInt": {
                    "type": "Int32",
                    "default": 256,
                    "min": 128,
                    "max": 512
                },
                "testArray": {
                    "type": "Array",
                    "items": {
                        "type": "#color"
                    },
                    "max_items": 10
                },
                "testObjInline": {
                    "type": "Object",
                    "properties": {
                        "testSubInt": {
                            "type": "Int32",
                            "default": 100
                        }
                    }
                },
                "testObjRef": {
                    "type": "#color",
                    "default": "FFAABB"
                }
            }
        }
        )"_json;

        TArray<json> types;
        types.emplace_back(colorJson);
        types.emplace_back(unitJson);

        for (const auto& typeJson : types)
        {
            ProcessType(metadata, typeJson);
        }

        json baseUnitJson = R"(
        {
            "id": "BaseUnit",
            "based_on": "UnitData",
            "testInt": 456
        }
        )"_json;

        json lancerJson = R"(
        {
            "id": "Lancer",
            "based_on": "BaseUnit",
            "testInt": 123
        }
        )"_json;

        TArray<json> archetypes;
        archetypes.emplace_back(baseUnitJson);
        archetypes.emplace_back(lancerJson);

        for (const auto& archetypeJson : archetypes)
        {
            auto idIter = archetypeJson.find("id");
            if (idIter == archetypeJson.end())
            {
                continue;
            }

            PHXString archetypeIdStr = idIter->get<PHXString>();

            auto basedOnIter = archetypeJson.find("based_on");
            if (basedOnIter == archetypeJson.end())
            {
                continue;
            }

            PHXString basedOnStr = basedOnIter->get<PHXString>();

            FName archetypeId = FName(archetypeIdStr.data(), archetypeIdStr.length());
            json flat = archetypeJson.flatten();

            for (auto && [name, valueJson] : flat.items())
            {
                FName fieldId = FName(name.data(), name.length());

                LDSTypedValue value;
                if (!FromJson(lds, metadata, valueJson, json::json_pointer(name), value))
                {
                    continue;
                }
                
                lds.EmplaceRecord_GetRef(archetypeId, fieldId, value);
            }
        }
    }
}