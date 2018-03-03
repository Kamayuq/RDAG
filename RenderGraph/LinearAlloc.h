#pragma once
#include <utility>
#include "Types.h"

void* LinearAlloc(U64 InSize);

bool AllocContains(const void* Ptr);

template<typename T>
T* LinearAlloc()
{
	return reinterpret_cast<T*>(LinearAlloc(sizeof(T)));
}

template<typename T, typename... ARGS>
T* LinearNew(ARGS&&... Args)
{
	return new (LinearAlloc(sizeof(T))) T(std::forward<ARGS>(Args)...);
}

void LinearReset();