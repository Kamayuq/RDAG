#pragma once
#include <utility>

template<typename F, typename V, typename = std::void_t<>> 
struct IsCallable : std::false_type {};

template<typename F, typename V>
struct IsCallable<F, V, std::void_t<decltype(std::declval<F>()(std::declval<V>()))>> : std::true_type {};

template<typename>
struct FalseType
{
	static constexpr bool value = false;
};

template<typename>
class DebugResourceTable;

struct IResourceTableBase;

template<typename ResourceTableType>
static inline void CheckIsValidResourceTable(const ResourceTableType& Table)
{
	static_assert(std::is_base_of<IResourceTableBase, ResourceTableType>(), "Table is not a ResorceTable");
	Table.CheckAllValid();
}

namespace Internal
{
	/* the empty Sequence just returns the result */
	constexpr auto Seq()
	{
		return [=](const auto& s) constexpr 
		{ 
			CheckIsValidResourceTable(s);
			return s; 
		};
	}

	/* A Sequence recursively applies the input to all the elements in the Sequence (unil the Sequence is empty) */
	template<typename X, typename... XS>
	constexpr auto Seq(const X& x, const XS&... xs)
	{
		return [=](const auto& s0) constexpr 
		{ 
			using InputType = decltype(s0);
			CheckIsValidResourceTable(s0);
			if constexpr (IsCallable<X, InputType>::value)
			{
				auto s1 = s0.Union(x(s0));
				CheckIsValidResourceTable(s1);
				return s1.Union(Seq(xs...)(s1));
			}
			else
			{
				auto s1 = s0.Union(x(DebugResourceTable(s0)));
				CheckIsValidResourceTable(s1);
				auto r = s1.Union(Seq(xs...)(s1));
				//comment out this line to get more detailed information about which handle type is missing on MSVC
				static_assert(FalseType<decltype(x(DebugResourceTable(s0)))>::value, "No suitable conversion between tables, are you maybe missing an entry?");
				return r;
			}
		};
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

/* Scope Filters on Output based on its Input*/
/* this will scope the changes done to the entries passed into the Sequence and where the input type (s) is the same as its return type but changes are are carried on*/
template<typename SequenceType, typename... SequenceArgs>
auto Scope(const Seq<SequenceType, SequenceArgs...>& seq)
{
	//this is a lambda from type of s -> to type of s
	return Seq([=](const auto& s)
	{
		CheckIsValidResourceTable(s);
		//the return type is limited to the input (s)
		using ReturnType = std::decay_t<decltype(s)>;
		ReturnType Ret = seq(s);
		return Ret;
	});
}

template<typename... TS>
class ResourceTable;

/* an Extaction filters on output */
/* this will revert the changes done to the entries passed into the Sequence and only return/extract the changes of the extra specified values in AddedReturnTable */
template<typename... EXTRACTIONS, typename SequenceType, typename... SequenceArgs>
auto Extract(const Seq<SequenceType, SequenceArgs...>& seq)
{
	return Seq([=](const auto& s)
	{
		CheckIsValidResourceTable(s);
		using ExtractionTable = ResourceTable<EXTRACTIONS...>;
		ExtractionTable ExtractedResult = seq(s);
		//the return type will always contain the input (s)
		return s.Union(ExtractedResult);
	});
}

/* a Selection filters on input */
/* this will reduce the input type to the selection before coninuing with the sequence, the removed values will be re-added after the sequence */
template<typename... SELECTIONS, typename SequenceType, typename... SequenceArgs>
auto Select(const Seq<SequenceType, SequenceArgs...>& seq)
{
	return Seq([=](const auto& s)
	{
		CheckIsValidResourceTable(s);
		using SelectionTable = ResourceTable<SELECTIONS...>;
		auto SelectionResult = seq(SelectionTable(s));
		//the return type will always contain the input (s)
		return s.Union(SelectionResult);
	});
}