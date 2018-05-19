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
	struct BlendDest : Uav2dResourceHandle<BlendDest>
	{
		static constexpr const char* Name = "BlendDest";

		explicit BlendDest() {}
	};

	struct BlendSource : Texture2dResourceHandle<BlendSource>
	{
		static constexpr const char* Name = "BlendSource";

		explicit BlendSource() {}
		explicit BlendSource(const struct TransparencyTarget&){}
		explicit BlendSource(const struct HalfResTransparencyResult&){}
	};
}

struct SimpleBlendPass
{
	using PassInputType = ResourceTable<RDAG::BlendSource>;
	using PassOutputType = ResourceTable<RDAG::BlendDest>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, EBlendMode::Type BlendMode);
};