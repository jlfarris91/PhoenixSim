
#pragma once

#include <memory>

#include "PhoenixCore.h"

namespace Phoenix
{
    enum class EPropertyValueType
    {
        Unknown,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float,
        Double,
        Bool,
        String,
        Name,
        FixedPoint,
        COUNT
    };

    struct PHOENIXCORE_API IMethodPointer
    {
        virtual ~IMethodPointer() {}
        virtual bool IsStatic() const = 0;
        virtual void Execute(void* obj) const = 0;
        virtual bool CanExecute(void* obj) const = 0;
    };

    struct PHOENIXCORE_API MethodDescriptor
    {
        PHXString Name;
        TSharedPtr<IMethodPointer> MethodPointer;
    };

    template <class T>
    struct MethodPointer : IMethodPointer
    {
        using TExecutePtr = void(T::*)();
        using TCanExecutePtr = bool(T::*)() const;

        MethodPointer(TExecutePtr executePtr, TCanExecutePtr canExecutePtr = nullptr)
            : ExecutePtr(executePtr)
            , CanExecutePtr(canExecutePtr)
        {
        }

        bool IsStatic() const override
        {
            return false;
        }

        void Execute(void* obj) const override
        {
            PHX_ASSERT(CanExecute(obj));
            T* typedObj = static_cast<T*>(obj);
            (typedObj->*ExecutePtr)();
        }

        bool CanExecute(void* obj) const override
        {
            if (!ExecutePtr) return false;
            if (!CanExecutePtr) return true;
            const T* typedObj = static_cast<const T*>(obj);
            return (typedObj->*CanExecutePtr)();
        }

        TExecutePtr ExecutePtr = nullptr;
        TCanExecutePtr CanExecutePtr = nullptr;
    };

    template <class T>
    struct ConstMethodPointer : IMethodPointer
    {
        using TExecutePtr = void(T::*)() const;
        using TCanExecutePtr = bool(T::*)() const;

        ConstMethodPointer(TExecutePtr executePtr, TCanExecutePtr canExecutePtr = nullptr)
            : ExecutePtr(executePtr)
            , CanExecutePtr(canExecutePtr)
        {
        }

        bool IsStatic() const override
        {
            return false;
        }

        void Execute(void* obj) const override
        {
            PHX_ASSERT(CanExecute(obj));
            const T* typedObj = static_cast<const T*>(obj);
            (typedObj->*ExecutePtr)();
        }

        bool CanExecute(void* obj) const override
        {
            if (!ExecutePtr) return false;
            if (!CanExecutePtr) return true;
            const T* typedObj = static_cast<const T*>(obj);
            return (typedObj->*CanExecutePtr)();
        }

        TExecutePtr ExecutePtr = nullptr;
        TCanExecutePtr CanExecutePtr = nullptr;
    };

    struct PHOENIXCORE_API StaticFunctionPointer : IMethodPointer
    {
        using TExecutePtr = void(*)();
        using TCanExecutePtr = bool(*)();

        StaticFunctionPointer(TExecutePtr executePtr, TCanExecutePtr canExecutePtr = nullptr)
            : ExecutePtr(executePtr)
            , CanExecutePtr(canExecutePtr)
        {
        }

        bool IsStatic() const override
        {
            return true;
        }

        void Execute(void* obj) const override
        {
            PHX_ASSERT(obj == nullptr);
            PHX_ASSERT(CanExecute(obj));
            ExecutePtr();
        }

        bool CanExecute(void* obj) const override
        {
            PHX_ASSERT(obj == nullptr);
            if (!ExecutePtr) return false;
            return !CanExecutePtr || CanExecutePtr();
        }

        TExecutePtr ExecutePtr = nullptr;
        TCanExecutePtr CanExecutePtr = nullptr;
    };

    struct PHOENIXCORE_API IPropertyAccessor
    {
        virtual ~IPropertyAccessor() {}
        virtual bool IsReadOnly() const = 0;
        virtual bool IsStatic() const = 0;
        virtual void Get(const void* obj, void* value, size_t len) const = 0;
        virtual void Set(void* obj, const void* value, size_t len) const = 0;
        virtual void Initialize(void* memory) const = 0;

        template <class T>
        T Get(const void* obj) const
        {
            T value;
            Get(obj, &value, sizeof(T));
            return value;
        }

        template <class T>
        void Set(void* obj, const T& value) const
        {
            Set(obj, &value, sizeof(T));
        }
    };

    struct PHOENIXCORE_API PropertyDescriptor
    {
        PHXString Name;
        EPropertyValueType ValueType = EPropertyValueType::Unknown;
        TSharedPtr<IPropertyAccessor> PropertyAccessor;
    };

    template <class T, class TValue>
    struct PropertyAccessor : IPropertyAccessor
    {
        using TGetter = TValue(T::*)() const;
        using TSetter = void(T::*)(const TValue&);

        PropertyAccessor(TGetter getter, TSetter setter = nullptr) : Getter(getter), Setter(setter) {}

        TValue Get(const void* obj) const
        {
            PHX_ASSERT(obj);
            PHX_ASSERT(Getter);
            const T* typedObj = static_cast<const T*>(obj);
            return (typedObj->*Getter)();
        }

        void Set(void* obj, const TValue& val) const
        {
            PHX_ASSERT(obj);
            PHX_ASSERT(Setter);
            T* typedObj = static_cast<T*>(obj);
            return (typedObj->*Setter)(val);
        }

        bool IsReadOnly() const override
        {
            return Setter != nullptr;
        }

        bool IsStatic() const override
        {
            return false;
        }

        void Get(const void* obj, void* value, size_t len) const override
        {
            TValue* typedValue = static_cast<TValue*>(value);
            *typedValue = Get(obj);
        }

        void Set(void* obj, const void* value, size_t len) const override
        {
            const TValue* typedValue = static_cast<const TValue*>(value);
            Set(obj, *typedValue);
        }

        void Initialize(void* memory) const override
        {
            TValue* typedValue = static_cast<TValue*>(memory);
            *typedValue = TValue{};
        }

        TGetter Getter = nullptr;
        TSetter Setter = nullptr;
    };

    template <class TValue>
    struct StaticPropertyAccessor : IPropertyAccessor
    {
        using TGetter = TValue(*)();
        using TSetter = void(*)(const TValue&);

        StaticPropertyAccessor(TGetter getter, TSetter setter = nullptr) : Getter(getter), Setter(setter) {}

        TValue Get() const
        {
            PHX_ASSERT(Getter);
            return (*Getter)();
        }

        void Set(const TValue& val) const
        {
            PHX_ASSERT(Setter);
            return (*Setter)(val);
        }

        bool IsReadOnly() const override
        {
            return Setter != nullptr;
        }

        bool IsStatic() const override
        {
            return true;
        }

        void Get(const void* obj, void* value, size_t len) const override
        {
            PHX_ASSERT(obj == nullptr);
            TValue* typedValue = static_cast<TValue*>(value);
            *typedValue = Get();
        }

        void Set(void* obj, const void* value, size_t len) const override
        {
            PHX_ASSERT(obj == nullptr);
            const TValue* typedValue = static_cast<const TValue*>(value);
            Set(*typedValue);
        }

        void Initialize(void* memory) const override
        {
            TValue* typedValue = static_cast<TValue*>(memory);
            *typedValue = TValue{};
        }

        TGetter Getter = nullptr;
        TSetter Setter = nullptr;
    };

    template <class T, class TValue>
    struct FieldAccessor : IPropertyAccessor
    {
        using TFieldPtr = TValue T::*;

        FieldAccessor(TFieldPtr ptr) : FieldPtr(ptr) {}

        const TValue& Get(const void* obj) const
        {
            PHX_ASSERT(obj);
            PHX_ASSERT(FieldPtr);
            const T* typedObj = reinterpret_cast<const T*>(obj);
            return typedObj->*FieldPtr;
        }

        void Set(void* obj, const TValue& val) const
        {
            PHX_ASSERT(obj);
            PHX_ASSERT(FieldPtr);
            T* typedObj = reinterpret_cast<T*>(obj);
            typedObj->*FieldPtr = val;
        }

        bool IsReadOnly() const override
        {
            return false;
        }

        bool IsStatic() const override
        {
            return false;
        }

        void Get(const void* obj, void* value, size_t len) const override
        {
            TValue* typedValue = static_cast<TValue*>(value);
            *typedValue = Get(obj);
        }

        void Set(void* obj, const void* value, size_t len) const override
        {
            const TValue* typedValue = static_cast<const TValue*>(value);
            Set(obj, *typedValue);
        }

        void Initialize(void* memory) const override
        {
            TValue* typedValue = static_cast<TValue*>(memory);
            *typedValue = TValue{};
        }

        TFieldPtr FieldPtr = nullptr;
    };

    template <class TValue>
    struct StaticFieldAccessor : IPropertyAccessor
    {
        using TFieldPtr = TValue*;

        StaticFieldAccessor(TFieldPtr ptr) : FieldPtr(ptr) {}

        const TValue& Get() const
        {
            PHX_ASSERT(FieldPtr);
            return *FieldPtr;
        }

        void Set(const TValue& val) const
        {
            PHX_ASSERT(FieldPtr);
            *FieldPtr = val;
        }

        bool IsReadOnly() const override
        {
            return false;
        }

        bool IsStatic() const override
        {
            return true;
        }

        void Get(const void* obj, void* value, size_t len) const override
        {
            PHX_ASSERT(obj == nullptr);
            TValue* typedValue = static_cast<TValue*>(value);
            *typedValue = Get();
        }

        void Set(void* obj, const void* value, size_t len) const override
        {
            PHX_ASSERT(obj == nullptr);
            const TValue* typedValue = static_cast<const TValue*>(value);
            Set(*typedValue);
        }

        void Initialize(void* memory) const override
        {
            TValue* typedValue = static_cast<TValue*>(memory);
            *typedValue = TValue{};
        }

        TFieldPtr FieldPtr = nullptr;
    };

    template <class T>
    constexpr EPropertyValueType GetPropertyValueType()
    {
#define TYPE_TO_ENUM_VALUE(type, enum_value) \
        if constexpr (std::is_same_v<T, type>) \
        { \
            return EPropertyValueType::enum_value; \
        }

        TYPE_TO_ENUM_VALUE(int8, Int8)
        TYPE_TO_ENUM_VALUE(uint8, UInt8)
        TYPE_TO_ENUM_VALUE(int16, Int16)
        TYPE_TO_ENUM_VALUE(uint16, UInt16)
        TYPE_TO_ENUM_VALUE(int32, Int32)
        TYPE_TO_ENUM_VALUE(uint32, UInt32)
        TYPE_TO_ENUM_VALUE(int64, Int64)
        TYPE_TO_ENUM_VALUE(uint64, UInt64)
        TYPE_TO_ENUM_VALUE(float, Float)
        TYPE_TO_ENUM_VALUE(double, Double)
        TYPE_TO_ENUM_VALUE(bool, Bool)
        TYPE_TO_ENUM_VALUE(PHXString, String)
        TYPE_TO_ENUM_VALUE(FName, Name)

#undef TYPE_TO_ENUM_VALUE

        return EPropertyValueType::Unknown;
    }

    struct PHOENIXCORE_API BaseDescriptor
    {
        PHXString Name;
        struct TypeDescriptor* Descriptor; 
    };

    struct PHOENIXCORE_API TypeDescriptor
    {
        virtual ~TypeDescriptor() {}

        FName GetName() const { return Name; }
        PHXString GetDisplayName() const { return DisplayName; }

        template <class T>
        void RegisterInterface()
        {
            const auto& baseDescriptor = T::GetStaticTypeDescriptor();
            BaseDescriptor& descriptor = Bases[baseDescriptor.GetName()];
            descriptor.Name = baseDescriptor.GetName();
            descriptor.Descriptor = &baseDescriptor;
        }

        template <class T, class TValue>
        const PropertyDescriptor& RegisterProperty(
            const PHXString& name,
            typename PropertyAccessor<T, TValue>::TGetter getter,
            typename PropertyAccessor<T, TValue>::TSetter setter = nullptr)
        {
            PropertyDescriptor& descriptor = Properties[name];
            descriptor.Name = name;
            descriptor.ValueType = GetPropertyValueType<TValue>();
            descriptor.PropertyAccessor = MakeShared<PropertyAccessor<T, TValue>>(getter, setter);
            return descriptor;
        }

        template <class TValue>
        const PropertyDescriptor& RegisterProperty(
            const PHXString& name,
            typename StaticPropertyAccessor<TValue>::TGetter getter,
            typename StaticPropertyAccessor<TValue>::TSetter setter = nullptr)
        {
            PropertyDescriptor& descriptor = Properties[name];
            descriptor.Name = name;
            descriptor.ValueType = GetPropertyValueType<TValue>();
            descriptor.PropertyAccessor = MakeShared<StaticPropertyAccessor<TValue>>(getter, setter);
            return descriptor;
        }

        template <class T, class TValue>
        const PropertyDescriptor& RegisterProperty(
            const PHXString& name,
            typename FieldAccessor<T, TValue>::TFieldPtr fieldPtr)
        {
            PropertyDescriptor& descriptor = Properties[name];
            descriptor.Name = name;
            descriptor.ValueType = GetPropertyValueType<TValue>();
            descriptor.PropertyAccessor = MakeShared<FieldAccessor<T, TValue>>(fieldPtr);
            return descriptor;
        }

        template <class TValue>
        const PropertyDescriptor& RegisterProperty(
            const PHXString& name,
            typename StaticFieldAccessor<TValue>::TFieldPtr fieldPtr)
        {
            PropertyDescriptor& descriptor = Properties[name];
            descriptor.Name = name;
            descriptor.ValueType = GetPropertyValueType<TValue>();
            descriptor.PropertyAccessor = MakeShared<StaticFieldAccessor<TValue>>(fieldPtr);
            return descriptor;
        }

        template <class T>
        const MethodDescriptor& RegisterMethod(
            const PHXString& name,
            typename MethodPointer<T>::TExecutePtr executePtr,
            typename MethodPointer<T>::TCanExecutePtr canExecutePtr = nullptr)
        {
            MethodDescriptor& descriptor = Methods[name];
            descriptor.Name = name;
            descriptor.MethodPointer = MakeShared<MethodPointer<T>>(executePtr, canExecutePtr);
            return descriptor;
        }

        template <class T>
        const MethodDescriptor& RegisterConstMethod(
            const PHXString& name,
            typename ConstMethodPointer<T>::TExecutePtr executePtr,
            typename ConstMethodPointer<T>::TCanExecutePtr canExecutePtr = nullptr)
        {
            MethodDescriptor& descriptor = Methods[name];
            descriptor.Name = name;
            descriptor.MethodPointer = MakeShared<ConstMethodPointer<T>>(executePtr, canExecutePtr);
            return descriptor;
        }

        const MethodDescriptor& RegisterStaticMethod(
            const PHXString& name,
            StaticFunctionPointer::TExecutePtr executePtr,
            StaticFunctionPointer::TCanExecutePtr canExecutePtr = nullptr)
        {
            MethodDescriptor& descriptor = Methods[name];
            descriptor.Name = name;
            descriptor.MethodPointer = MakeShared<StaticFunctionPointer>(executePtr, canExecutePtr);
            return descriptor;
        }

        void PlacementNew(uint8* data) const
        {
        }

        TMap<PHXString, PropertyDescriptor> Properties;
        TMap<PHXString, MethodDescriptor> Methods;
        TMap<PHXString, BaseDescriptor> Bases;
        FName Name;
        const char* DisplayName;
    };

#define PHX_DECLARE_TYPE_BEGIN(type) \
    public: \
        using ThisType = type; \
        static constexpr FName StaticTypeName = #type##_n; \
    private: \
        struct STypeDescriptor { \
            static constexpr FName StaticName = #type##_n; \
            static constexpr const char* StaticDisplayName = #type; \
            static TypeDescriptor Construct() \
            { \
                TypeDescriptor definition; \
                definition.Name = StaticName; \
                definition.DisplayName = StaticDisplayName; \

#define PHX_DECLARE_TYPE_END() \
                return definition; \
            } \
        }; \
    public: \
        static const TypeDescriptor& GetStaticTypeDescriptor() { static TypeDescriptor sd = STypeDescriptor::Construct(); return sd; } \
        const TypeDescriptor& GetTypeDescriptor() const override { return GetStaticTypeDescriptor(); }

#define PHX_DECLARE_TYPE(type) \
    PHX_DECLARE_TYPE_BEGIN(type) \
    PHX_DECLARE_TYPE_END()

#define PHX_DECLARE_INTERFACE_BEGIN(type) \
    public: \
        using ThisType = type; \
    private: \
        struct STypeDescriptor { \
            static constexpr FName StaticName = #type##_n; \
            static constexpr const char* StaticDisplayName = #type; \
            static TypeDescriptor Construct() \
            { \
                TypeDescriptor definition; \
                definition.Name = StaticName; \
                definition.DisplayName = StaticDisplayName; \

#define PHX_DECLARE_INTERFACE_END() \
                return definition; \
            } \
        }; \
    public: \
        static const TypeDescriptor& GetStaticTypeDescriptor() { static TypeDescriptor sd = STypeDescriptor::Construct(); return sd; } \
        const TypeDescriptor& GetTypeDescriptor() const { return GetStaticTypeDescriptor(); }

#define PHX_DECLARE_INTERFACE(type) \
    PHX_DECLARE_INTERFACE_BEGIN(type) \
    PHX_DECLARE_INTERFACE_END()

#define PHX_REGISTER_FIELD(type, name) definition.RegisterProperty<ThisType, type>(#name, &ThisType::name);
#define PHX_REGISTER_STATIC_FIELD(type, name) definition.RegisterProperty<type>(#name, &ThisType::name);
#define PHX_REGISTER_PROPERTY(type, name) definition.RegisterProperty<ThisType, type>(#name, &ThisType::Get##name, &ThisType::Set##name);
#define PHX_REGISTER_STATIC_PROPERTY(type, name) definition.RegisterProperty<type>(#name, &ThisType::Get##name, &ThisType::Set##name);
#define PHX_REGISTER_METHOD(name) definition.RegisterMethod<ThisType>(#name, &ThisType::##name);
#define PHX_REGISTER_CONST_METHOD(name) definition.RegisterConstMethod<ThisType>(#name, &ThisType::##name);
#define PHX_REGISTER_STATIC_METHOD(name) definition.RegisterStaticMethod(#name, &ThisType::##name);
#define PHX_REGISTER_INTERFACE(name) definition.RegisterInterface<name>();
}
