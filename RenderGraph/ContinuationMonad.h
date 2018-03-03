#pragma once
#include <memory>
#include <future>
#include <thread>

#include "Assert.h"
#include "Monad.h"
#include "SequenceMonad.h"
#include "LinearAlloc.h"

class Continuation : public Monad<Continuation>
{
	typedef Monad<Continuation> BaseType;
	template<typename L>
	using  WrappedMonad = typename BaseType::template WrappedMonad<L>;

public:
	template<typename M, typename K>
	constexpr static auto Bind(const  WrappedMonad<M>& m, const K& k)
	{
		return BaseType::template WrapMonad([m, k](const auto& c /* (c -> r) -> r */) constexpr
		{
			auto l = [c, k](const auto& a) constexpr
			{
				return k(a)(c); /* (c -> r) -> r */
			};

			return m(l);
		});
	}
	template<typename A>
	constexpr static auto Return(const A& a)
	{
		return BaseType::template WrapMonad([a](const auto& k /* 'a->'r */) constexpr
		{
			return k(a);
		});
	}

	template<typename M>
	constexpr static auto Run(M&& m)
	{
		return m([](auto& a) { return a; });
	}

	template<typename F>
	constexpr static auto Modify(const F& f)
	{
		return BaseType::template WrapMonad([f](const auto& k) constexpr 
		{ 
			return k([f](const auto& s) constexpr
			{
				return f(s);
			});
		});
	}
	//callCC :: ((a -> Cont r b) -> Cont r a) -> Cont r a
	//callCC f = Cont (\k -> runCont (f (\a -> Cont (\_ -> k a))) k)
	/*	template<typename L>
	constexpr static auto CallCC(const L& f)
	{
		return BaseType::WrapMonad([f](auto k) constexpr
		{
			auto l = [k](auto a) constexpr
			{
				return BaseType::WrapMonad([k, a](auto) constexpr
				{
					return k(a);
				});
			};
			auto m = f(l);
			return m(k);
		});
	}//*/
};

struct PromiseBase
{
	template<typename L>
	static auto Run(const L& l)
	{
		return l([](const auto& a) { return a; });
	}
};

template<typename T>
struct Promise : PromiseBase
{
	typedef T ReturnType;

	Promise() = default;

	Promise(Promise& Other)
	{
		char Local[sizeof(Promise)];
		memcpy_s(Local, sizeof(Promise), this, sizeof(Promise));
		memcpy_s(this, sizeof(Promise), &Other, sizeof(Promise));
		memcpy_s(&Other, sizeof(Promise), Local, sizeof(Promise));
	}

	Promise(Promise&& Other)
	{
		char Local[sizeof(Promise)];
		memcpy_s(Local, sizeof(Promise), this, sizeof(Promise));
		memcpy_s(this, sizeof(Promise), &Other, sizeof(Promise));
		memcpy_s(&Other, sizeof(Promise), Local, sizeof(Promise));
	}

	Promise& operator=(Promise&& Other)
	{
		char Local[sizeof(Promise)];
		memcpy_s(Local, sizeof(Promise), this, sizeof(Promise));
		memcpy_s(this, sizeof(Promise), &Other, sizeof(Promise));
		memcpy_s(&Other, sizeof(Promise), Local, sizeof(Promise));
		return *this;
	}


	virtual ~Promise() { };
	virtual void Run(T*, const char*, const class IResourceTableInfo*) { check(0); }
	virtual T Get() { check(0); return *static_cast<T*>(LambdaPtr); };

protected:
	std::future<T*> future;
	void* ResultPtr = nullptr;
	void* LambdaPtr = nullptr;
};

template<typename L, typename T = decltype(PromiseBase::Run(std::declval<L>()))>
struct PromiseImpl final : Promise<T>
{
	typedef Promise<T> BaseType;

	~PromiseImpl()
	{
		static_assert(sizeof(Promise<T>) == sizeof(PromiseImpl), "For slicing sizes must match");
		delete static_cast<L*>(this->LambdaPtr);
	}

	PromiseImpl(const L& InLambda) 
	{
		this->LambdaPtr = new (LinearAlloc<L>()) L(InLambda);
	}

	void Run(T* Desination, const char* Name, const IResourceTableInfo* CurrentRenderPassData) override
	{
		 this->future = std::async(std::launch::async, [=] 
		 { 
			 new (Desination) T(PromiseBase::Run(*static_cast<L*>(this->LambdaPtr)), Name, CurrentRenderPassData, nullptr);
			 this->ResultPtr = Desination;
			 return Desination;
		 });
	}

	T Get() override
	{
		if (this->ResultPtr != nullptr)
		{
			return *static_cast<T*>(this->ResultPtr);
		}
		else if (this->future.valid())
		{
			this->future.wait();
			this->ResultPtr = this->future.get();
			return *static_cast<T*>(this->ResultPtr);
		}
		else
		{
			return PromiseBase::Run(*static_cast<L*>(this->LambdaPtr));
		}
	}
};

template<typename L>
typename PromiseImpl<L>::BaseType MakePromise(const L& l)
{
	return static_cast<typename PromiseImpl<L>::BaseType&&>(PromiseImpl<L>(l));
}

template<typename X>
constexpr auto AsyncInternal(const X& x)
{
	//return Continuation::Bind(Continuation::Return(x), [=](const auto& a) constexpr { return Continuation::Return(a); });
	return SequenceT<Continuation>::Modify(x);
}

template<typename X, typename... XS>
constexpr auto AsyncInternal(const X& x, const XS&... xs)
{
	return SequenceT<Continuation>::Bind(SequenceT<Continuation>::Modify(x), [=](const auto&) constexpr
	{ 
		return AsyncInternal(xs...);
	});
	//return Continuation::Bind(Async(xs...), [=](const auto& uxs) constexpr {  return Continuation::Return(uxs(x)); });
}

template<typename... XS>
constexpr auto Async(const XS&... xs)
{
	return [=](const auto& s) constexpr
	{
		return MakePromise(AsyncInternal(xs...)(s));
	};
	//return Continuation::Bind(Async(xs...), [=](const auto& uxs) constexpr {  return Continuation::Return(uxs(x)); });
}