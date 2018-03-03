#pragma once
#include "Monad.h"

class Sequence : public Monad<Sequence>
{
	typedef Monad<Sequence> BaseType;
	template<typename L>
	using WrappedMonad = typename BaseType::template WrappedMonad<L>;

public:
	template<typename M, typename K>
	constexpr static auto Bind(const WrappedMonad<M>& m, const K& k)
	{
		return BaseType::WrapMonad([m, k](const auto& s) constexpr
		{
			auto p = m(s);
			auto b = BaseType::UnwrapMonad(k(p));
			return b(p);
		});
	}

	template<typename S>
	constexpr static auto Return(const S& s)
	{
		return BaseType::WrapMonad([s](const auto&) constexpr { return s; });
	}

	constexpr static auto Get()
	{
		return BaseType::WrapMonad([](const auto& s) constexpr { return s; });
	}

	template<typename F>
	constexpr static auto Modify(const F& f)
	{
		return BaseType::WrapMonad([f](const auto& s) constexpr { return f(s); });
	}

	template<typename A, typename B>
	constexpr static auto Seq(const A& a, const B& b)
	{
		return Sequence::Bind(a, [b](const auto&) constexpr { return b; });
	}
};

/*template<typename F>
constexpr auto SeqInternal(const F& f)
{
	return Sequence::Modify([f](const auto& s) { return s.Union(f(s)); });
}*/

template<typename F>
constexpr auto SeqInternal(const F& f)
{
	return Sequence::Modify(f);
}

template<typename X>
constexpr auto Seq(const X& x)
{
	return SeqInternal(x);
}

template<typename X, typename... XS>
constexpr auto Seq(const X& x, const XS&... xs)
{
	return Sequence::Bind(SeqInternal(x), [=](const auto&) constexpr { return Seq(xs...); });
}


template<typename Inner>
class SequenceT : public Monad<SequenceT<Inner>>
{
	typedef Monad<SequenceT<Inner>> BaseType;
	template<typename L>
	using  WrappedMonad = typename BaseType::template WrappedMonad<L>;

	typedef Monad<Inner> InnerBaseType;
	template<typename L>
	using  InnerWrappedMonad = typename InnerBaseType::template WrappedMonad<L>;

public:
	template<typename M, typename K>
	constexpr static auto Bind(const WrappedMonad<M>& m, const K& k)
	{
		return BaseType::WrapMonad([m, k](const auto& s) constexpr
		{
			auto ms = m(s);
			return Inner::Bind(ms, [k](const auto& p) constexpr
			{
				auto b = BaseType::UnwrapMonad(k(p));
				return b(p);
			});
		});
	}

	template<typename A>
	constexpr static auto Return(const A& a)
	{
		return BaseType::WrapMonad([a](const auto&) constexpr
		{
			return Inner::Return(a);
		});
	}

	template<typename MA>
	constexpr static auto ReturnM(const InnerWrappedMonad<MA>& ma)
	{
		return BaseType::WrapMonad([ma](const auto& s) constexpr
		{
			return Inner::Bind(ma, [s](const auto&) constexpr
			{
				return Inner::Return(s);
			});
		});
	}

	template<typename S>
	constexpr static auto Get()
	{
		return BaseType::template WrapMonad<S>([](const S& s) constexpr
		{
			return Inner::Return(s);
		});
	}

	template<typename F>
	constexpr static auto Modify(const F& f)
	{
		return BaseType::WrapMonad([f](auto s) constexpr
		{
			return Inner::Return(f(s));
		});
	}

	template<typename A, typename B>
	constexpr static auto Seq(const A& a, const B& b)
	{
		return SequenceT<Inner>::Bind(a, [b](auto) constexpr { return b; });
	}
};
