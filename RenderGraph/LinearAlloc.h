#pragma once
#include <utility>
#include "Types.h"

void* LinearAlloc(U64 InSize);

bool AllocContains(const void* Ptr);

template<typename T>
inline T* LinearAlloc(U32 Count = 1)
{
	if (Count != 0)
	{
		return reinterpret_cast<T*>(LinearAlloc(sizeof(T) * Count));
	}
	else
	{
		return nullptr;
	}
}

template<typename T, typename... ARGS>
inline T* LinearNew(ARGS&&... Args)
{
	return new (LinearAlloc(sizeof(T))) T(std::forward<ARGS>(Args)...);
}

void LinearReset();