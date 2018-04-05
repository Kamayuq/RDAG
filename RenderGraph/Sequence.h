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

/* this will scope the changes done to the entries passed into the Sequence and where the input type (s) is the same as its return type but changes are are carried on*/
template<typename SequenceType>
auto Scope(const Seq<SequenceType>& seq)
{
	//this is a lambda from type of s -> to type of s
	return[=](const auto& s) constexpr -> decltype(s)
	{
		Internal::CheckIsResourceTable(s);
		return seq(s);
	};
}

/* this will revert the changes done to the entries passed into the Sequence and only return/extract the changes of the extra specified values in AddedReturnTable */
template<typename EXTRACTION, typename SequenceType>
auto Extract(const Seq<SequenceType>& seq)
{
	return[=](const auto& s) constexpr
	{
		Internal::CheckIsResourceTable(s);
		EXTRACTION ExtractedResult = seq(s);
		return s.Union(ExtractedResult);
	};
}
