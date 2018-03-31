#include "LinearAlloc.h"
#include <malloc.h>
#include "Assert.h"


#define USE_ATOMICS 0
#if USE_ATOMICS
#include <atomic>
#endif

struct SimpleLinearAllocator
{
	SimpleLinearAllocator(U64 InSize, U64 InAlignment = 8) : Offset(0), Size(InSize), Alignment(InAlignment)
	{
		Base = reinterpret_cast<U8*>(_mm_malloc(Size, Alignment));
	}

	~SimpleLinearAllocator()
	{
		_mm_free(Base);
	}

	void* Alloc(U64 InSize)
	{
		U64 AllocSize = RoundUp(InSize, Alignment);
#if USE_ATOMICS
		U64 AllocOffset = Offset.fetch_add(AllocSize);
#else
		U64 AllocOffset = Offset;
		Offset += AllocSize;
#endif
		check(AllocOffset + AllocSize < Size);
		return Base + AllocOffset;
	}

	void Reset()
	{
		Offset = 0;
	}

	bool Contains(const void* Ptr)
	{
		if (Ptr < Base)
			return false;
		if (Ptr >= (Base + Size))
			return false;

		return true;
	}

private:
	inline static U64 RoundUp(U64 size, U64 align)
	{
		return (size + align - 1) & ~(align - 1);
	}

	U8* Base; 
#if USE_ATOMICS
	std::atomic<U64> Offset;
#else
	U64 Offset;
#endif
	const U64 Size;
	const U64 Alignment;
};

static SimpleLinearAllocator* LinearAllocator = new SimpleLinearAllocator(4 * 1024 * 1024);
void* LinearAlloc(U64 InSize)
{
	return LinearAllocator->Alloc(InSize);
}

void LinearReset()
{
	 LinearAllocator->Reset();
}

bool AllocContains(const void* Ptr)
{
	return LinearAllocator->Contains(Ptr);
}