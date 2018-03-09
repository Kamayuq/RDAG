#pragma once
#include <type_traits>

struct Set final
{
	template<typename T>
	struct SetElement
	{};

	template<int Length, typename T, typename TS...>
	struct SetElements : SetElements<Length+1, TS...>, SetElement<T&>
	{
		using BaseType = SetElements<Length+1, TS...>;
		constexpr int Index = Length;
	};
	
	template<typename... TS>
	struct Type final : SetElements<0, TS...>
	{
		template<typename T>
		static constexpr bool Contains()
		{
			return std::is_base_of_v<SetElement<T&>, Type<TS...>>;
		}

		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};
		
		template<typename T>
		static constexpr int GetIndex() const
		{
			static_assert(Contains<T>(), "Set does not contain this type");
			return GetIndexInternal(Type());
		}
		
		template<int I>
		static constexpr auto GetType() -> decltype(GetTypeInternal<I>(Type()))
		{
			static_assert(I < GetSize(), "Set index out off bounds");
			return {};
		}
		
	private:
		template<typename T, int I, typename E, typename ES...>
		static constexpr int GetIndexInternal(const SetElements<I, E, ES...>& elems) const
		{
			if constexpr(std::is_same_v<T, E>)
			{
				return I;
			}
			else
			{
				return GetIndexInternal(static_cast<const typename SetElements<I, E, ES...>::Base&>(elems));
			}
		};
		
		typename<int I, typename E, typename ES...>
		static constexpr auto GetTypeInternal(const SetElements<I, E, ES...>&) -> E;
		
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
