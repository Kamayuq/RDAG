#pragma once

template<typename Lambda>
struct Sequence : Lambda
{
	constexpr Sequence(const Lambda& lambda) : Lambda(lambda) {}
};

template<typename Lambda>
constexpr auto MakeSequence(const Lambda& lambda)
{
	return Sequence<Lambda>(lambda);
}

constexpr auto Seq()
{
	return MakeSequence([=](const auto& s) constexpr { return s; });
}

template<typename X, typename... XS>
constexpr auto Seq(const Sequence<X>& x, const Sequence<XS>&... xs)
{
	return MakeSequence([=](const auto& s) constexpr { return Seq(xs...)(x(s)); });
}

template<typename... ARGS>
struct Seq2
{
	Seq2(const Sequence<ARGS>... Args) : Sequence(Seq(Args...))
	{

	}

	template<typename TableType>
	auto operator()(const TableType &s)
	{
		return Sequence(s);
	}

private:
	using SequenceType = std::decay_t<decltype(Seq(std::declval<Sequence<ARGS>>()...))>;
	SequenceType Sequence;
};
