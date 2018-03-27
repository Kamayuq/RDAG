#pragma once
#include <type_traits>

struct Set final
{
	/* Wrapper class to hande types that cannot be inherited from */
	template<typename T>
	struct SetElement
	{};
	
	/* Type List of Elements each anotated with its index to be able to retrieve the element */
	template<int Length, typename... TS>
	struct SetElements;

	/* The non empty List */
	template<int Length, typename T, typename... TS>
	struct SetElements<Length, T, TS...> : SetElements<Length+1, TS...>, SetElement<T&>
	{
		using BaseType = SetElements<Length+1, TS...>;
		static constexpr int Index = Length;
	};
	
	/* The empty List */
	template<int Length>
	struct SetElements<Length>
	{
		static constexpr int Index = Length;
	};

	struct Internal
	{
		/* providing an I the compiler chooses the closest match during overload resultion */
		/* as the I's are unique we can extract the type E the compiler will infer for us this way */ 
		template<int I, typename E, typename... ES>
		static constexpr auto GetTypeInternal(const SetElements<I, E, ES...>&)->E;
	};

	/* The type representing a compile time set */
	template<typename... TS>
	struct Type final : SetElements<0, TS...>
	{
		using BaseType = SetElements<0, TS...>;
		
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
		
		/* Given an I what Type is stored at this Index */
		template<int I>
		static constexpr auto GetType() -> decltype(Internal::GetTypeInternal<I>(BaseType()))
		{
			static_assert(I < GetSize(), "Set index out off bounds");
			return {};
		}

	private:
		/* Use the compiler to check if each Type is Unique (Multiple inheritance from the same type is not allowed) */
		struct UniqueElementChecker : SetElement<TS&>... {};
	};

	/* The Union of two Sets */
	template<typename... XS, typename... YS>
	static constexpr auto Union(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<YS...>(), Type<XS...>(), Type<XS...>());
	}

	/* The Intersection of two Sets */
	template<typename... XS, typename... YS>
	static constexpr auto Intersect(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<false>(Type<XS...>(), Type<YS...>(), Type<>());
	}

	/* The left Set without the right one */
	template<typename... XS, typename... YS>
	static constexpr auto LeftDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<XS...>(), Intersect(Type<XS...>(), Type<YS...>()), Type<>());
	}

	/* The right Set without the left one */
	template<typename... XS, typename... YS>
	static constexpr auto RightDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<YS...>(), Intersect(Type<XS...>(), Type<YS...>()), Type<>());
	}

	/* Two Sets combined without their Intersection */
	template<typename... XS, typename... YS>
	static constexpr auto Difference(const Type<XS...>&, const Type<YS...>&)
	{
		return Union(LeftDifference(Type<XS...>(), Type<YS...>()), RightDifference(Type<XS...>(), Type<YS...>()));
	}

private:
	/* Recursively itterate through the first argument checking it against the second and accumulating the results in the third */
	template<bool InverseTest, typename T, typename... TS, typename... XS, typename... RS>
	static constexpr auto Recurse(const Type<T, TS...>&, const Type<XS...>&, const Type<RS...>&)
	{
		constexpr bool TestResult = Type<XS...>::template Contains<T>();
		if constexpr (InverseTest ? !TestResult : TestResult)
		{
			//add the element T to the accumulation 
			return Recurse<InverseTest>(Type<TS...>(), Type<XS...>(), Type<RS..., T>());
		}
		else
		{
			//skip over element T 
			return Recurse<InverseTest>(Type<TS...>(), Type<XS...>(), Type<RS...>());
		}
	}

	/* After itteration return the third parameter which contains the accumulated results */
	template<bool InverseTest, typename... XS, typename... RS>
	static constexpr auto Recurse(const Type<>&, const Type<XS...>&, const Type<RS...>&)
	{
		return Type<RS...>();
	}
};
