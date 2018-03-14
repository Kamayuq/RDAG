#pragma once
#include <type_traits>

struct Set final
{
	template<typename T>
	struct SetElement
	{};

	template<int Length, typename... TS>
	struct SetElements;

	template<int Length, typename T, typename... TS>
	struct SetElements<Length, T, TS...> : SetElements<Length+1, TS...>, SetElement<T&>
	{
		using BaseType = SetElements<Length+1, TS...>;
		static constexpr int Index = Length;
	};
	
	template<int Length>
	struct SetElements<Length>
	{
		static constexpr int Index = Length;
	};

	struct Internal
	{
		template<int I, typename E, typename... ES>
		static constexpr auto GetTypeInternal(const SetElements<I, E, ES...>&)->E;
	};

	template<typename... TS>
	struct Type final : SetElements<0, TS...>
	{
		using BaseType = SetElements<0, TS...>;
		template<typename T>
		static constexpr bool Contains()
		{
			return std::is_base_of_v<SetElement<T&>, Type<TS...>>;
		}

		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};
		
		template<int I>
		static constexpr auto GetType() -> decltype(Internal::GetTypeInternal<I>(BaseType()))
		{
			static_assert(I < GetSize(), "Set index out off bounds");
			return {};
		}

	private:
		struct UniqueElementChecker : SetElement<TS&>... {};
	};

	template<typename... XS, typename... YS>
	static constexpr auto Union(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<YS...>(), Type<XS...>(), Type<XS...>());
	}

	template<typename... XS, typename... YS>
	static constexpr auto Intersect(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<false>(Type<XS...>(), Type<YS...>(), Type<>());
	}

	template<typename... XS, typename... YS>
	static constexpr auto LeftDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<XS...>(), Intersect(Type<XS...>(), Type<YS...>()), Type<>());
	}

	template<typename... XS, typename... YS>
	static constexpr auto RightDifference(const Type<XS...>&, const Type<YS...>&)
	{
		return Recurse<true>(Type<YS...>(), Intersect(Type<XS...>(), Type<YS...>()), Type<>());
	}

	template<typename... XS, typename... YS>
	static constexpr auto Difference(const Type<XS...>&, const Type<YS...>&)
	{
		return Union(LeftDifference(Type<XS...>(), Type<YS...>()), RightDifference(Type<XS...>(), Type<YS...>()));
	}

private:
	template<bool InverseTest, typename T, typename... TS, typename... XS, typename... RS>
	static constexpr auto Recurse(const Type<T, TS...>&, const Type<XS...>&, const Type<RS...>&)
	{
		constexpr bool TestResult = Type<XS...>::template Contains<T>();
		if constexpr (InverseTest ? !TestResult : TestResult)
		{
			return Recurse<InverseTest>(Type<TS...>(), Type<XS...>(), Type<RS..., T>());
		}
		else
		{
			return Recurse<InverseTest>(Type<TS...>(), Type<XS...>(), Type<RS...>());
		}
	}

	template<bool InverseTest, typename... XS, typename... RS>
	static constexpr auto Recurse(const Type<>&, const Type<XS...>&, const Type<RS...>&)
	{
		return Type<RS...>();
	}
};
