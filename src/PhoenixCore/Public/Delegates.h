
#pragma once

#include "Platform.h"

namespace Phoenix
{
    template <class TFunc, class ...TArgs>
    auto Invoke(TFunc&& func, TArgs&&... args)
    {
        return Forward<TFunc>(func)(Forward<TArgs>(args)...);
    }

    template <class TFunc, class ...TArgs, class ...TParams>
    auto Invoke(TFunc&& func, TArgs&&... args, const TTuple<TParams>& params)
    {
        return Forward<TFunc>(func)(Forward<TArgs>(args)...);
    }

    struct DelegateHandle
    {
        uint64 ID = 0;
    };

    template <bool bConst, class T, class TFunc>
    struct TMemberFuncPtr;

    template <class T, class TRet, class ...TArgs>
    struct TMemberFuncPtr<false, T, TRet(TArgs...)>
    {
        using type = TRet(T::*)(TArgs...);
    };

    template <class T, class TRet, class ...TArgs>
    struct TMemberFuncPtr<true, T, TRet(TArgs...)>
    {
        using type = TRet(T::*)(TArgs...) const;
    };

    template <class T>
    struct TPayload;

    template <class TRet, class ...TArgs>
    struct TPayload<TRet(TArgs...)>
    {
        TTuple<TArgs..., TRet> Values;

        template <class ...TArgs2>
        explicit TPayload(TArgs2&&... args)
            : Values(Forward(args...), TRet())
        {
        }

        TRet& GetResult()
        {
            return Values.template Get<sizeof...(TArgs)>();
        }
    };

    template <class ...TArgs>
    struct TPayload<void(TArgs...)>
    {
        TTuple<TArgs...> Values;

        template <class ...TArgs2>
        explicit TPayload(TArgs2&&... args)
            : Values(Forward(args...))
        {
        }

        void GetResult()
        {
        }
    };

    template <class TRet, class ...TArgs>
    struct IDelegateInstance
    {
        virtual ~TDelegateInstance() {}
        virtual TRet Execute(TArgs...) const = 0;
        virtual DelegateHandle GetHandle() const = 0;
    };

    template <class TFunc, class ...TPayloadArgs>
    struct TDelegateInstanceBase : IDelegateInstance<TFunc>
    {

        DelegateHandle GetHandle() const final
        {
            return Handle;
        }

    protected:
        TPayload<TPayloadArgs...> Payload;
        DelegateHandle Handle;
    };

    template <bool bConst, class T, class TRet, class ...TArgs, class ...TPayloadArgs>
    struct TSPMemberDelegateInstance<bConst, T, TRet(TArgs...), TPayloadArgs...> : TDelegateInstanceBase<TRet(TArgs...), TPayloadArgs...>
    {
        using TMethodPtr = class TMemberFuncPtr<bConst, T, TRet(TArgs..., TPayloadArgs...)>;

        TRet Execute(TArgs... args) const override
        {
            TSharedPtr<T> sharedPtr = WeakPtr.lock();
            PHX_ASSERT(sharedPtr.get());

            return Invoke(MethodPtr, sharedPtr.get(), Forward<TArgs>(args)...);
        }

        TWeakPtr<T> WeakPtr;
        TMethodPtr MethodPtr;
    };

    template <class ...TArgs>
    struct TMulticastDelegate
    {
        
    };
}
