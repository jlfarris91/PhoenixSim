
#include "ImGuiPropertyGrid.h"

#include "imgui.h"
#include <ranges>

using namespace Phoenix;

void DrawPropertyEditor(void* obj, const Phoenix::PropertyDescriptor& propertyDesc)
{
#define NUMERIC_EDITOR(type, imgui_type) \
    { \
        type v_min = std::numeric_limits<type>::min(), v_max = std::numeric_limits<type>::max(); \
        ImGui::SetNextItemWidth(-FLT_MIN); \
        type v = propertyDesc.PropertyAccessor->Get<type>(obj); \
        if (ImGui::DragScalar("##Editor", imgui_type, &v, 1.0f, &v_min, &v_max)) \
        { \
            propertyDesc.PropertyAccessor->Set(obj, v); \
        } \
    }

    switch (propertyDesc.ValueType)
    {
        case EPropertyValueType::Int8:      NUMERIC_EDITOR(int8, ImGuiDataType_S8) break;
        case EPropertyValueType::UInt8:     NUMERIC_EDITOR(uint8, ImGuiDataType_U8) break;
        case EPropertyValueType::Int16:     NUMERIC_EDITOR(int16, ImGuiDataType_S16) break;
        case EPropertyValueType::UInt16:    NUMERIC_EDITOR(uint16, ImGuiDataType_U16) break;
        case EPropertyValueType::Int32:     NUMERIC_EDITOR(int32, ImGuiDataType_S32) break;
        case EPropertyValueType::UInt32:    NUMERIC_EDITOR(uint32, ImGuiDataType_U32) break;
        case EPropertyValueType::Int64:     NUMERIC_EDITOR(int64, ImGuiDataType_S64) break;
        case EPropertyValueType::UInt64:    NUMERIC_EDITOR(uint64, ImGuiDataType_U64) break;
        case EPropertyValueType::Float:     NUMERIC_EDITOR(float, ImGuiDataType_Float) break;
        case EPropertyValueType::Double:    NUMERIC_EDITOR(double, ImGuiDataType_Double) break;
        case EPropertyValueType::Bool:
            {
                bool v = propertyDesc.PropertyAccessor->Get<bool>(obj);
                if (ImGui::Checkbox("##Editor", &v))
                {
                    propertyDesc.PropertyAccessor->Set(obj, v);
                }
                break;
            }
        case EPropertyValueType::String:
            {
                PHXString v = propertyDesc.PropertyAccessor->Get<PHXString>(obj);
                
                char buff[MAX_PATH];
                strcpy_s(buff, MAX_PATH, v.data());

                if (ImGui::InputText("##Editor", buff, MAX_PATH))
                {
                    v = buff;
                    propertyDesc.PropertyAccessor->Set(obj, v);
                }
                break;
            }
        case EPropertyValueType::Name:          break;
        case EPropertyValueType::FixedPoint:    break;
        default: break;
    }

#undef NUMERIC_EDITOR
}

void DrawPropertyGrid(void* obj, const StructDescriptor& descriptor)
{
    if (ImGui::BeginTable("##properties", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f); // Default twice larger

        for (const auto& methodDesc : descriptor.Methods | std::views::values)
        {
            ImGui::TableNextRow();
            ImGui::PushID(methodDesc.Name.c_str());
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(methodDesc.Name.c_str());
            ImGui::TableNextColumn();

            void* actualObj = methodDesc.MethodPointer->IsStatic() ? nullptr : obj;

            ImGui::BeginDisabled(!methodDesc.MethodPointer->CanExecute(actualObj));
            if (ImGui::Button(methodDesc.Name.c_str()))
            {
                methodDesc.MethodPointer->Execute(actualObj);
            }
            ImGui::EndDisabled();

            ImGui::PopID();
        }

        for (const auto& propertyDesc : descriptor.Properties | std::views::values)
        {
            ImGui::TableNextRow();
            ImGui::PushID(propertyDesc.Name.c_str());
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(propertyDesc.Name.c_str());
            ImGui::TableNextColumn();

            void* actualObj = propertyDesc.PropertyAccessor->IsStatic() ? nullptr : obj;
            DrawPropertyEditor(actualObj, propertyDesc);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}
