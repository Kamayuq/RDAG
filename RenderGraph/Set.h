#pragma once
#include <type_traits>

struct Set final
{
	template<typename T>
	struct ConstexprType
	{};

	template<typename... TS>
	struct Type final : private ConstexprType<TS&>...
	{
		template<typename C>
		static constexpr bool Contains()
		{
			return std::is_base_of_v<ConstexprType<C&>, Type<TS...>>;
		}

		static constexpr size_t GetSize() 
		{ 
			return sizeof...(TS); 
		};
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