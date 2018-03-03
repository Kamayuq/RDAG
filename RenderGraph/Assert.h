#pragma once
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127)
#endif
inline void checkFunc(bool cond)
{
	if (!(cond))
	{ 
		static int* val = nullptr; 
		*val = 0; 
	}
}
#define check(a) checkFunc(a)
#ifdef _MSC_VER
#pragma warning( pop ) 
#endif

template<bool Premise, typename A, typename B>
struct StaticTypeAssert
{
	StaticTypeAssert()
	{
		A a = B();
		static_assert(false, "Cannot assign collected source table");
	}
};

template<typename A, typename B>
struct StaticTypeAssert<true, A, B>
{

};

#define ASSIGN_TYPE_ASSERT(P, A, B)			\
{											\
	StaticTypeAssert<P, A, B> assert;		\
	(void)assert;							\
}