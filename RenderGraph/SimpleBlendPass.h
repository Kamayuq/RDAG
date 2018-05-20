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
	using PassInputType = ResourceTable<RDAG::BlendSource>;
	using PassOutputType = ResourceTable<RDAG::BlendDest>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, EBlendMode::Type BlendMode);
};