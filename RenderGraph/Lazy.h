#pragma once

template<typename LAMBDA>
class Lazy : public LAMBDA
{
	typedef Lazy<LAMBDA> ThisType;

public:
	constexpr Lazy(LAMBDA&& Lambda) : LAMBDA(std::forward<LAMBDA>(Lambda))
	{}
};

template<typename LAMBDA, typename VALUE>
class AssignedLazy : public Lazy<LAMBDA>
{
	template<typename TYPE>
	friend class Monad;

	typedef AssignedLazy<LAMBDA, VALUE> ThisType;

	VALUE* Assignment;

public:
	constexpr AssignedLazy(LAMBDA&& Lambda, VALUE& Assignment) : Lazy<LAMBDA>(std::forward<LAMBDA>(Lambda)), Assignment(&Assignment)
	{}
};

template<typename VALUE, typename LAMBDA>
constexpr decltype(auto) operator <<= (VALUE& a, Lazy<LAMBDA>&& l)
{
	return AssignedLazy<LAMBDA, VALUE>(std::forward<Lazy<LAMBDA>>(l), a);
}

template<typename LAMBDA>
constexpr auto MakeLazy(LAMBDA&& Lambda)
{
	return Lazy<LAMBDA>(std::forward<LAMBDA>(Lambda));
};

#define LAZY(...) (MakeLazy([&](){ return ( __VA_ARGS__ ); }))
#define MEMO(...) (MakeLazy([&](){ static auto memo = ( __VA_ARGS__ ); return memo; }))