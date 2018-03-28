#pragma once

/* A Sequence is a function from a value to an result */
template<typename Lambda>
struct Sequence : Lambda
{
	using LambdaType = Lambda;
	constexpr Sequence(const Lambda& lambda) : Lambda(lambda) {}
};

/* helper function to infer template arguments */
template<typename Lambda>
constexpr auto MakeSequence(const Lambda& lambda)
{
	return Sequence<Lambda>(lambda);
}

namespace Internal
{
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
}

/* Wrapper for a Sequence in a class template */
template<typename... ARGS>
struct Seq : Sequence<std::decay_t<decltype(Internal::Seq(std::declval<Sequence<ARGS>>()...))>>
{
	Seq(const Sequence<ARGS>&... Args) : Sequence<std::decay_t<decltype(Internal::Seq(std::declval<Sequence<ARGS>>()...))>>(Internal::Seq(Args...)) {}
};

template<typename... ARGS>
Seq(const ARGS&... Args) -> Seq<typename ARGS::LambdaType...>;