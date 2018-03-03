#pragma once
#include <type_traits>

struct Set final
{
	template<typename... TS>
	struct Type final
	{
		constexpr Type()
		{
#if __clang__
			static_assert(!(... || ContainsDupe<TS>()), "a set must have unique elements");
#endif
		}

		template<typename C>
		static constexpr bool Contains()
		{
			return ContainsInternal<false, C, TS...>();
		}

		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};

	private:
		template<typename C>
		static constexpr bool ContainsDupe()
		{
			return ContainsInternal<true, C, TS...>();
		}

		template<bool DetectDupe, typename C, typename X, typename... XS>
		static constexpr bool ContainsInternal()
		{
			if constexpr (std::is_same_v<C, X>)
			{
				if constexpr(!DetectDupe)
					return true;
				else
					return ContainsInternal<false, C, XS...>();
			}
			else
			{
				return ContainsInternal<DetectDupe, C, XS...>();
			}
		}

		template<bool DetectDupe, typename C>
		static constexpr bool ContainsInternal()
		{
			return false;
		}
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