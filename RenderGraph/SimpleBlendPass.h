#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"

namespace EBlendMode
{
	enum Enum
	{
		Additive,
		Modulate,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Modulate) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};

namespace RDAG
{
	SIMPLE_UAV_HANDLE(BlendDest, BlendDest);
	SIMPLE_TEX_HANDLE(BlendSource);
}

struct SimpleBlendPass
{
	using SimpleBlendInput = ResourceTable<RDAG::BlendSource>;
	using SimpleBlendResult = ResourceTable<RDAG::BlendDest>;

	static SimpleBlendResult Build(const RenderPassBuilder& Builder, const SimpleBlendInput& Input, EBlendMode::Type BlendMode);
};