#pragma once
#include <utility>

namespace Concept_Detail
{
	template<typename F, typename... Args, typename = decltype(std::declval<F>()(std::declval<Args&&>()...))>
	std::true_type IsValidImpl(void*);

	template<typename, typename...>
	std::false_type IsValidImpl(...);

	constexpr auto IsValid = [](auto f)
	{
		using F = decltype(f);
		return [](auto&&... args)
		{
			return decltype(IsValidImpl<F, decltype(args) && ...>(nullptr)){};
		};
	};

	template<typename T>
	struct TypeT 
	{
		using Type = T;
	};

	template<typename T>
	constexpr auto type = TypeT<T>{};

	template<typename T>
	T value(TypeT<T>);
	
	constexpr auto isAssignable = IsValid([](auto&& a, auto&& b) -> decltype(std::decay_t<decltype(a)>(b)){});
}

template<typename A, typename B>
using IsAssignable = decltype(Concept_Detail::isAssignable(std::declval<A>(), std::declval<B>()));