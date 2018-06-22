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

#ifndef _DEBUG
#undef check
#define check(a) { (void)(a); }
#endif