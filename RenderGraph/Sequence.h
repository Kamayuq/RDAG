#pragma once
#include <utility>

namespace Internal
{
	/* the empty Sequence just returns the result */
	constexpr auto Seq()
	{
		return [=](const auto& s) constexpr { return s; };
	}

	/* Single Element optimization */
	template<typename X>
	constexpr auto Seq(const X& x)
	{
		return x;
	}

	/* A Sequence recursively applies the input to all the elements in the Sequence (unil the Sequence is empty) */
	template<typename X, typename... XS>
	constexpr auto Seq(const X& x, const XS&... xs)
	{
		return [=](const auto& s) constexpr { return Seq(xs...)(x(s)); };
	}
}

/* Wrapper for a Sequence in a class template */
template<typename SequenceType, typename... ARGS>
struct Seq : SequenceType
{
	Seq(const ARGS&... Args) : SequenceType(Internal::Seq(Args...)) {}
};

template<typename... ARGS>
Seq(const ARGS&... Args) -> Seq<decltype(Internal::Seq(std::declval<ARGS>()...)), ARGS...>;

namespace Internal
{
	/* the empty Sequence just returns the result */
	constexpr auto SeqScope()
	{
		return[=](const auto& s) constexpr { return s; };
	}

	/* Single Element optimization */
	template<typename X>
	constexpr auto SeqScope(const X& x)
	{
		return x;
	}

	template<typename X, typename... XS>
	auto SeqScope(const X& x, const XS&... xs)
	{
		return[=](const auto& s) constexpr
		{
			using ReturnType = decltype(s);
			ReturnType TempResult = Seq(x, xs...)(s);
			return TempResult;
		};
	}
}


/* A scoped Sequence will revert added entries inside the seq and only return the Type passed into the Sequence with changes done to those elements */
template<typename SequenceType, typename... ARGS>
struct SeqScope : SequenceType
{
	SeqScope(const ARGS&... Args) : SequenceType(Internal::SeqScope(Args...)) {}
};

template<typename... ARGS>
SeqScope(const ARGS&... Args) -> SeqScope<decltype(Internal::SeqScope(std::declval<ARGS>()...)), ARGS...>;

/* this Sequence will revert the changes done to the entries passed into the Sequence and return the extra specified values */
template<typename AddedReturnTable, typename... ARGS>
auto SeqSelect(const ARGS&... Args)
{
	return[=](const auto& s) constexpr
	{
		AddedReturnTable ExtraResult = Seq(Args...)(s);
		return s.Union(ExtraResult);
	};
}