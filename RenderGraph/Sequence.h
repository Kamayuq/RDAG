#pragma once
#include <utility>
#include <type_traits>

template<typename F, typename V, typename = std::void_t<>> 
struct IsCallable : std::false_type {};

template<typename F, typename V>
struct IsCallable<F, V, std::void_t<decltype(std::declval<F>()(std::declval<V>()))>> : std::true_type {};

template<typename, typename>
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
			using InputType = std::decay_t<decltype(s0)>;
			CheckIsValidResourceTable(s0);
			if constexpr (IsCallable<X, InputType>::value)
			{
				auto s1 = x(s0);
				auto s2 = s0.Union(s1);
				CheckIsValidResourceTable(s2);
				return s2.Union(Seq(xs...)(s2));
			}
			else
			{
				auto s1 = s0.Union(x(DebugResourceTable(s0, x)));
#ifdef __clang__ //MSVC only prints the depth first static_assert while clang needs this to print the source of the error
				static_assert(sizeof(DebugResourceTable<InputType, X>) == 0, "The Sequence causing the error can be found at the bottom of this template error stack");
#endif
				CheckIsValidResourceTable(s1);
				return s1.Union(Seq(xs...)(s1));
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
Seq(const ARGS&...) -> Seq<decltype(Internal::Seq(std::declval<ARGS>()...)), ARGS...>;

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
		if constexpr (std::is_assignable<ReturnType, decltype(seq(s))>::value)
		{
			ReturnType Ret = seq(s);
			return Ret;
		}
		else
		{
			ReturnType Ret = DebugResourceTable(seq(s), seq);
			(void)Ret;
#ifdef __clang__ //MSVC only prints the depth first static_assert while clang needs this to print the source of the error
			static_assert(sizeof(decltype(seq(s))) == 0, "The seq returned a type that is did not contain it's inputs");
#endif
			return s.Union(seq(s));
		}
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
		if constexpr (std::is_assignable<ExtractionTable, decltype(seq(s))>::value)
		{
			ExtractionTable ExtractedResult = seq(s);
			//the return type will always contain the input (s)
			return s.Union(ExtractedResult);
		}
		else
		{
			ExtractionTable ExtractedResult = DebugResourceTable(seq(s), seq);
			(void)ExtractedResult;
#ifdef __clang__ //MSVC only prints the depth first static_assert while clang needs this to print the source of the error
			static_assert(sizeof(decltype(seq(s))) == 0, "The requested types could not be extracted from the seq");
#endif
			return s.Union(seq(s));
		}
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
		if constexpr (std::is_assignable<SelectionTable, decltype(s)>::value)
		{
			auto SelectionResult = seq(SelectionTable(s));
			//the return type will always contain the input (s)
			return s.Union(SelectionResult);
		}
		else
		{
			auto SelectionResult = seq(SelectionTable(DebugResourceTable(s, seq)));
			(void)SelectionResult;
#ifdef __clang__ //MSVC only prints the depth first static_assert while clang needs this to print the source of the error
			static_assert(sizeof(decltype(seq(s))) == 0, "The requested types could not be found in the input sequence");
#endif
			return s.Union(seq(s));
		}
	});
}