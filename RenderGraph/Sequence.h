#pragma once

constexpr auto Seq()
{
	return [=](const auto& s) constexpr { return s; };
}

template<typename X, typename... XS>
constexpr auto Seq(const X& x, const XS&... xs)
{
	return [=](const auto& s) constexpr { return Seq(xs...)(x(s)); };
}