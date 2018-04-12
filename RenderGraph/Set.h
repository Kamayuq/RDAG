#pragma once
#include <type_traits>

struct Set final
{
	/* Wrapper class to hande types that cannot be inherited from */
	template<typename T>
	struct SetElement
	{};
	
	/* The type representing a compile time set */
	template<typename... TS>
	struct Type final : SetElement<TS&>...
	{
		/* Does this Set contain this type? */
		template<typename T>
		static constexpr bool Contains()
		{
			return std::is_base_of_v<SetElement<T&>, Type<TS...>>;
		}

		/* Query the size of a Set */
		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};
	};

	/* The Union of two Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Union(const Type<XS...>&, const Type<YS...>&)
	{
		if constexpr(sizeof...(YS) < sizeof...(XS))
		{
			return Recurse<ContainsOp<true, XS...>, ConstructorType>(Type<YS...>(), Type<XS...>(), Type<>());
		}
		else
		{
			return Recurse<ContainsOp<true, YS...>, ConstructorType>(Type<XS...>(), Type<>(), Type<YS...>());
		}
	}

	/* The Intersection of two Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Intersect(const Type<XS...>&, const Type<YS...>&)
	{
		if constexpr(sizeof...(XS) < sizeof...(YS))
		{
			return Recurse<ContainsOp<false, YS...>, ConstructorType>(Type<XS...>(), Type<>(), Type<>());
		}
		else
		{
			return Recurse<ContainsOp<false, XS...>, ConstructorType>(Type<YS...>(), Type<>(), Type<>());
		}
	}

	/* The left Set without the right one */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto LeftDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<ContainsOp<true, YS...>, ConstructorType>(Type<XS...>(), Type<>(), Type<>());
	}

	/* The right Set without the left one */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto RightDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<ContainsOp<true, XS...>, ConstructorType>(Type<YS...>(), Type<>(), Type<>());
	}

	/* Two Sets combined without their Intersection */
	template<template<typename...> class ConstructorType = Set::Type, typename... XS, typename... YS>
	static constexpr auto Difference(const Type<XS...>&, const Type<YS...>&)
	{
		return Meld<ConstructorType>(LeftDifference(Type<XS...>(), Type<YS...>()), RightDifference(Type<XS...>(), Type<YS...>()));
	}

	/* Filter one set based on a compile time predicate */
	template<typename FilterOp, template<typename...> class ConstructorType = Set::Type, typename... XS>
	static constexpr auto Filter(const Type<XS...>&)
	{
		return Recurse<FilterOp, ConstructorType>(Type<XS...>(), Type<>(), Type<>());
	}

private:
	template<bool Invert, typename... TS>
	struct ContainsOp
	{
		template<typename T>
		static constexpr bool Test()
		{
			constexpr bool Result = Type<TS...>::template Contains<T>();
			return Invert ? !Result : Result;
		}
	};

	/* meld 2 disjunct Sets */
	template<template<typename...> class ConstructorType = Set::Type, typename... RS, typename... LS>
	static constexpr auto Meld(const Type<LS...>&, const Type<RS...>&)
	{
		return Type<LS..., RS...>();
	}

	/* Recursively itterate through the first argument checking it against the second and accumulating the results in the third */
	template<typename FilterOp, template<typename...> class ConstructorType = Set::Type, typename T, typename... TS, typename... RS, typename... LS>
	static constexpr auto Recurse(const Type<T, TS...>&, const Type<RS...>&, const Type<LS...>&)
	{
		if constexpr (FilterOp::template Test<T>())
		{
			//add the element T to the accumulation 
			return Recurse<FilterOp>(Type<TS...>(), Type<RS...>(), Type<LS..., T>());
		}
		else
		{
			//skip over element T 
			return Recurse<FilterOp>(Type<TS...>(), Type<RS...>(), Type<LS...>());
		}
	}

	/* After itteration return the third parameter which contains the accumulated results */
	template<typename FilterOp, template<typename...> class ConstructorType = Set::Type, typename... RS, typename... LS>
	static constexpr auto Recurse(const Type<>&, const Type<RS...>&, const Type<LS...>&) -> ConstructorType<LS..., RS...>
	{
		return {};
	}
};
