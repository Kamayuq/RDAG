#pragma once
#include <stdint.h>
#include <limits>
#include <cstring>
#include <type_traits>

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
	template<int I, typename... TS>
	struct TupleList;

	template<int I, typename T, typename... TS>
	struct TupleList<I, T, TS...> : TupleList<I + 1, TS...> {};

	template<int I>
	struct TupleList<I> {};

	template <typename... ARGS>
	struct TupleOperation
	{
		struct TupleType : TupleList<0, ARGS...> {};

		template<int I, typename T, typename... TS>
		static constexpr auto GetElementType2(const TupleList<I, T, TS...>&)->T;

		template<int I>
		static constexpr auto GetElementType() -> decltype(GetElementType2<I>(TupleType()));
	};

	/* function traits are useful to extract the signature of a function when passed as a template argument */
	template <typename T>
	struct function_traits;

	//Base Implementation
	template <typename RET, typename... ARGS>
	struct function_traits<RET(ARGS...)>
	{
		static constexpr size_t arity = sizeof...(ARGS);
		using return_type = RET;

		template <size_t I>
		struct arg
		{
			static_assert(I < arity, "error: index out of bounds");
			using type = decltype(TupleOperation<ARGS...>::template GetElementType<I>());
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


	template<typename F, typename... ARGS>
	class IsCallable
	{
		struct Yes { char val[1]; };
		struct No { char val[2]; };

		static Yes Check(decltype(std::declval<F>()(std::declval<ARGS>()...))*);
		static No Check(...);

	public:
		static constexpr bool value = sizeof(decltype(Check(nullptr))) == sizeof(Yes);
	};
};


namespace EResourceTransition
{
	enum Type
	{
		Texture			= 0,
		Target			= 1,
		UAV				= 2,
		DepthTexture	= 3,
		DepthTarget		= 4,
		Undefined //keep last
	};
};

namespace ERenderResourceFormat
{
	enum Enum
	{
		ARGB8U,
		ARGB16F,
		ARGB16U,
		RG16F,
		L8,
		D16F,
		D32F,
		Structured,
		Invalid,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Invalid) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};

namespace EResourceFlags
{
	enum Enum
	{
		Discard = 0,		//RDAG does ignore those
		Managed = 1 << 0,	//RDAG does manage them
		External = 1 << 1,	//these are materialized from an external source and the user has to provide an implementation
		Default = Managed,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Default) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};
