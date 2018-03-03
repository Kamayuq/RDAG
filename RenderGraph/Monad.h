#pragma once
#include <type_traits>
#include "Lazy.h"

template<typename MT>
class Monad
{
public:
	template<typename LAMBDA>
	struct WrappedMonad : LAMBDA
	{
		typedef MT MonadType;
		constexpr WrappedMonad(const LAMBDA& Lambda) : LAMBDA(Lambda) {}
	};

protected:
	template<typename LAMBDA>
	constexpr static auto WrapMonad(const LAMBDA& Lambda)
	{
		return WrappedMonad<LAMBDA>(Lambda);
	}

	template<typename LAMBDA>
	constexpr static const LAMBDA& UnwrapMonad(const WrappedMonad<LAMBDA>& Monad)
	{
		return Monad;
	}

private:
	template<typename L>
	constexpr static auto Do(const Lazy<L>& l)
	{
		return MT::Bind(l(), [=](const auto& a) constexpr { return MT::Return(a); });
	};

	template<typename L, typename... XS>
	constexpr static auto Do(const Lazy<L>& l, const XS&... xs)
	{
		return MT::Bind(l(), [=](const auto&) constexpr { return MT::Do(xs...); });
	};

	template<typename L, typename V>
	constexpr static auto Do(const AssignedLazy<L, V>& l)
	{
		return MT::Bind(l(), [=](const auto& a) { *l.Assignment = a; return MT::Return(a); });
	};

	template<typename L, typename V, typename... XS>
	constexpr static auto Do(const AssignedLazy<L, V>& l, const XS&... xs)
	{
		return MT::Bind(l(), [=](const auto& a) { *l.Assignment = a; return MT::Do(xs...); });
	};

public:
	template<typename... XS>
	constexpr static auto Delay(const XS&... xs)
	{
		return MT::Do(xs...);
	}
};

