#pragma once
#include <utility>

struct IResourceTableBase;

namespace Internal
{
	static inline void CheckIsResourceTable(const IResourceTableBase&) {}

	/* the empty Sequence just returns the result */
	constexpr auto Seq()
	{
		return [=](const auto& s) constexpr 
		{ 
			CheckIsResourceTable(s);
			return s; 
		};
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
		return [=](const auto& s) constexpr 
		{ 
			CheckIsResourceTable(s);
			return Seq(xs...)(x(s)); 
		};
	}
}

/* Wrapper for a Sequence in a class template */
template<typename SequenceType, typename... ARGS>
struct Seq : SequenceType
{
	Seq(const ARGS&... Args) : SequenceType(Internal::Seq(Args...)) {}
};

//C++17 deduction helper
template<typename... ARGS>
Seq(const ARGS&... Args) -> Seq<decltype(Internal::Seq(std::declval<ARGS>()...)), ARGS...>;

namespace Internal
{
	/* the empty Sequence just returns the result */
	constexpr auto SeqScope()
	{
		return[=](const auto& s) constexpr 
		{
			CheckIsResourceTable(s);
			return s; 
		};
	}

	/* Single Element optimization */
	template<typename X>
	constexpr auto SeqScope(const X& x)
	{
		return x;
	}

	/* sequence where the input type (s) is the same as its return type but changes are are carried on*/
	template<typename X, typename... XS>
	auto SeqScope(const X& x, const XS&... xs)
	{
		return[=](const auto& s) constexpr
		{
			CheckIsResourceTable(s);
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

//C++17 deduction helper
template<typename... ARGS>
SeqScope(const ARGS&... Args) -> SeqScope<decltype(Internal::SeqScope(std::declval<ARGS>()...)), ARGS...>;

/* this Sequence will revert the changes done to the entries passed into the Sequence and only return the changes of the extra specified values in AddedReturnTable */
template<typename AddedReturnTable, typename... ARGS>
auto SeqSelect(const ARGS&... Args)
{
	return[=](const auto& s) constexpr
	{
		Internal::CheckIsResourceTable(s);
		AddedReturnTable ExtraResult = Seq(Args...)(s);
		return s.Union(ExtraResult);
	};
}