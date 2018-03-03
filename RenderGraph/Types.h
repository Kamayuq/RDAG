#pragma once
#include <stdint.h>
#include <limits>

#if _DEBUG
#define checked_cast dynamic_cast
#else
#define checked_cast static_cast
#endif

typedef uint8_t		U8;
typedef uint16_t	U16;
typedef uint32_t	U32;
typedef uint64_t	U64;
typedef int8_t		I8;
typedef int16_t		I16;
typedef int32_t		I32;
typedef int64_t		I64;
typedef intptr_t	IntPtr;
typedef uintptr_t	UintPtr;

template<class T, U64 N>
constexpr U64 sizeofArray(T(&)[N]) { return N; }

template<typename Enum, typename Type, typename INTT = U32>
struct SafeEnum
{
private:
	Enum e;

public:
	template<typename T>
	SafeEnum(const T& e) : e(Enum(e)) {}

	//Type& operator= (const Enum &oe) { e = oe;  return Type(*this); };

	friend bool operator! (const Type &a) { return !INTT(a.e); };
	friend Type operator~ (const Type &a) { return Type(~INTT(a.e)); };

	friend bool All(const Type &a, const Type &b) { return (INTT(a.e) & INTT(b.e)) == INTT(a.e); };
	friend bool Any(const Type &a, const Type &b) { return (INTT(a.e) & INTT(b.e)) != INTT(0); };

	friend bool operator== (const Type &a, const Type &b) { return INTT(a.e) == INTT(b.e); };
	friend bool operator!= (const Type &a, const Type &b) { return INTT(a.e) != INTT(b.e); };

	friend Type  operator|  (const Type &a, const Type &b) { return Type(Enum(INTT(a.e) | INTT(b.e))); }
	friend Type  operator&  (const Type &a, const Type &b) { return Type(Enum(INTT(a.e) & INTT(b.e))); }
	friend Type  operator^  (const Type &a, const Type &b) { return Type(Enum(INTT(a.e) ^ INTT(b.e))); }

	friend Type& operator|= (Type &a, const Type &b) { a.e = Enum(INTT(a) | INTT(b)); return a; }
	friend Type& operator&= (Type &a, const Type &b) { a.e = Enum(INTT(a) & INTT(b)); return a; }
	friend Type& operator^= (Type &a, const Type &b) { a.e = Enum(INTT(a) ^ INTT(b)); return a; }

	Enum GetEnum() const { return e; }
};

namespace Traits
{
	template <typename T>
	struct function_traits;

	//Base Implementation
	template <typename RET, typename... ARGS>
	struct function_traits<RET(ARGS...)>
	{
		static constexpr size_t arity = sizeof...(ARGS);

		typedef RET return_type;

		template <size_t I>
		struct arg
		{
			static_assert(I < arity, "error: index out of bounds");
			typedef typename std::tuple_element<I, std::tuple<ARGS...>>::type type;
		};
	};

	//Lambda functions:
	template <typename L>
	struct function_traits : function_traits<decltype(&L::operator())> {};

	//const member Function Pointers:
	template <typename CLASS, typename RET, typename... ARGS>
	struct function_traits<RET(CLASS::*)(ARGS...) const> : function_traits<RET(ARGS...)> {};
	
	//member Function Pointers:
	template <typename CLASS, typename RET, typename... ARGS>
	struct function_traits<RET(CLASS::*)(ARGS...)> : function_traits<RET(ARGS...)> {};

	//Function Pointers:
	template <typename RET, typename... ARGS>
	struct function_traits<RET(*)(ARGS...)> : function_traits<RET(ARGS...)> {};
};
