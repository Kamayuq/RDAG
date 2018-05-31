#pragma once
#include <type_traits>

struct Set final
{
	/* The type representing a compile time set */
	template<typename... TS>
	struct Type final
	{
	public:
		/* Does this Set contain this type? */
		template<typename T>
		static constexpr bool Contains()
		{
			return std::is_base_of_v<SetElement<T>, UniquenessTest>;
		}

		/* Query the size of a Set */
		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};

		template<typename T>
		static constexpr int GetIndex()
		{
			return GetIndexInternal<T>(IndexedElements<0, TS...>());
		};

	private:
		/* Wrapper class to hande types that cannot be inherited from */
		template<typename T>
		struct SetElement
		{};

		struct UniquenessTest final : SetElement<TS>...
		{};

		template<typename X, int I>
		struct IndexedElement
		{};

		template<int I, typename X, typename... XS>
		struct IndexedElements : IndexedElements<I + 1, XS...>, IndexedElement<X, I>
		{};

		template<int I, typename X>
		struct IndexedElements<I, X> : IndexedElement<X, I>
		{};

		template<typename X, int I>
		static constexpr int GetIndexInternal(const IndexedElement<X, I>&)
		{
			return I;
		};
	};

private:
	template<bool Invert, typename... TS>
	struct ContainsOp
	{
		template<typename T>
		static constexpr bool Filter()
		{
			constexpr bool Result = Type<TS...>::template Contains<T>();
			return Invert ? !Result : Result;
		}
	};

	/* meld 2 disjunct Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... LS, typename... RS>
	static constexpr ConstructorType<LS..., RS...> Meld(const Type<LS...>&, const Type<RS...>&);

	/* Recursively itterate through the first argument checking it against the second and accumulating the results in the third */
	template<typename FilterOp, typename T, typename... TS, typename... LS>
	static constexpr auto Recurse(const Type<T, TS...>&, const Type<LS...>&)
	{
		if constexpr (FilterOp::template Filter<T>())
		{
			//add the element T to the accumulation 
			return Recurse<FilterOp>(Type<TS...>(), Type<LS..., T>());
		}
		else
		{
			//skip over element T 
			return Recurse<FilterOp>(Type<TS...>(), Type<LS...>());
		}
	}

	/* After itteration return the third parameter which contains the accumulated results */
	template<typename FilterOp, typename... LS>
	static constexpr auto Recurse(const Type<>&, const Type<LS...>&)
	{
		return Type<LS...>{};
	}

public:
	/* The Union of two Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Union(const Type<XS...>&, const Type<YS...>&)
		-> decltype(Meld<ConstructorType>(Recurse<ContainsOp<true, XS...>>(Type<YS...>(), Type<>()), Type<XS...>()));

	/* The Intersection of two Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Intersect(const Type<XS...>&, const Type<YS...>&)
		-> decltype(Meld<ConstructorType>(Recurse<ContainsOp<false, YS...>>(Type<XS...>(), Type<>()), Type<>()));

	/* The left Set without the right one */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto LeftDifference(const Type<XS...>&, const Type<YS...>&)
		-> decltype(Meld<ConstructorType>(Recurse<ContainsOp<true, YS...>>(Type<XS...>(), Type<>()), Type<>()));

	/* The right Set without the left one */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto RightDifference(const Type<XS...>&, const Type<YS...>&)
		-> decltype(Meld<ConstructorType>(Recurse<ContainsOp<true, XS...>>(Type<YS...>(), Type<>()), Type<>()));

	/* Two Sets combined without their Intersection */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Difference(const Type<XS...>&, const Type<YS...>&)
		-> decltype(Meld<ConstructorType>(LeftDifference(Type<XS...>(), Type<YS...>()), RightDifference(Type<XS...>(), Type<YS...>())));

	/* Filter one set based on a compile time predicate */
	template<typename FilterOp, template<typename...> class ConstructorType = Set::Type, typename... XS>
	static constexpr auto Filter(const Type<XS...>&) 
		-> decltype(Meld<ConstructorType>(Recurse<FilterOp>(Type<XS...>(), Type<>()), Type<>()));
};
