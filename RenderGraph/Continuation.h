#pragma once
#include <memory>
#include <future>
#include <thread>

#include "Assert.h"
#include "Sequence.h"
#include "LinearAlloc.h"

struct PromiseBase
{
	template<typename L>
	static auto Run(const L& l)
	{
		return l();
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
	virtual void Run(T*, const char*) { check(0); }
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

	void Run(T* Desination, const char* Name) override
	{
		 this->future = std::async(std::launch::async, [=] 
		 { 
			 new (Desination) T(PromiseBase::Run(*static_cast<L*>(this->LambdaPtr)), Name, nullptr);
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

template<typename X, typename... XS>
constexpr auto Async(const X& x, const XS&... xs)
{
	return [=](const auto& s) constexpr 
	{ 
		return MakePromise([=]()
		{
			return Seq(x, xs...)(s);
		});
	};
}
