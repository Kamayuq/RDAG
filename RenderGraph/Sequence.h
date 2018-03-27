#pragma once

/* A Sequence is a function from a value to an result */
template<typename Lambda>
struct Sequence : Lambda
{
	constexpr Sequence(const Lambda& lambda) : Lambda(lambda) {}
};

/* helper function to infer template arguments */
template<typename Lambda>
constexpr auto MakeSequence(const Lambda& lambda)
{
	return Sequence<Lambda>(lambda);
}

/* the empty Sequence just returns the result */
constexpr auto Seq()
{
	return MakeSequence([=](const auto& s) constexpr { return s; });
}

/* A Sequence recursively applies the input to all the elements in the Sequence (unil the Sequence is empty) */ 
template<typename X, typename... XS>
constexpr auto Seq(const Sequence<X>& x, const Sequence<XS>&... xs)
{
	return MakeSequence([=](const auto& s) constexpr { return Seq(xs...)(x(s)); });
}

/* Wrapper for a Sequence in a class template */
template<typename... ARGS>
class Seq2
{
	/* infer the type of the Seq Lambda */
	using SequenceType = std::decay_t<decltype(Seq(std::declval<Sequence<ARGS>>()...))>;
	SequenceType Sequence;

public:
	Seq2(const ::Sequence<ARGS>... Args) : Sequence(Seq(Args...)) {}

	/* execute the sequence given an input */
	template<typename TableType>
	auto operator()(const TableType &s)
	{
		return Sequence(s);
	}
	
	/* wrap the Sequence object back to a base Sequence*/
	operator ::Sequence<SequenceType>() const { return MakeSequence(Sequence); }
};
