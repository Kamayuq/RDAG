#pragma once
#include <utility>

struct IResourceTableBase;

namespace Internal
{
	template<typename ResourceTableType>
	static inline void CheckIsResourceTable(const ResourceTableType& Table)
	{
		static_assert(std::is_base_of<IResourceTableBase, ResourceTableType>(), "Table is not a ResorceTable");
		Table.CheckAllValid();
	}

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

/* Scope Filters on Output based on its Input*/
/* this will scope the changes done to the entries passed into the Sequence and where the input type (s) is the same as its return type but changes are are carried on*/
template<typename SequenceType, typename... SequenceArgs>
auto Scope(const Seq<SequenceType, SequenceArgs...>& seq)
{
	//this is a lambda from type of s -> to type of s
	return Seq([=](const auto& s)
	{
		Internal::CheckIsResourceTable(s);
		//the return type is limited to the input (s)
		using ReturnType = std::decay_t<decltype(s)>;
		ReturnType Ret = seq(s);
		return Ret;
	});
}

/* an Extaction filters on output */
/* this will revert the changes done to the entries passed into the Sequence and only return/extract the changes of the extra specified values in AddedReturnTable */
template<typename EXTRACTION, typename SequenceType, typename... SequenceArgs>
auto Extract(const Seq<SequenceType, SequenceArgs...>& seq)
{
	return Seq([=](const auto& s)
	{
		Internal::CheckIsResourceTable(s);
		EXTRACTION ExtractedResult = seq(s);
		//the return type will always contain the input (s)
		return s.Union(ExtractedResult);
	});
}

/* a Selection filters on input */
/* this will reduce the input type to the selection before coninuing with the sequence, the removed values will be re-added after the sequence */
template<typename SELECTION, typename SequenceType, typename... SequenceArgs>
auto Select(const Seq<SequenceType, SequenceArgs...>& seq)
{
	return Seq([=](const auto& s)
	{
		Internal::CheckIsResourceTable(s);
		auto SelectionResult = seq(SELECTION(s));
		//the return type will always contain the input (s)
		return s.Union(SelectionResult);
	});
}