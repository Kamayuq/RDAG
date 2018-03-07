#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"

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
	struct TransparencyResult;
	struct HalfResTransparencyResult;

	struct BlendDest : Texture2dResourceHandle
	{
		static constexpr const char* Name = "BlendDest";

		explicit BlendDest() {}
	};

	struct BlendSource : Texture2dResourceHandle
	{
		static constexpr const U32 ResourceCount = 2;
		static constexpr const char* Name = "BlendSource";

		explicit BlendSource(EBlendMode::Type InBlendMode) : BlendMode(InBlendMode) {}
		explicit BlendSource(const TransparencyResult&) : BlendMode(EBlendMode::Modulate) {}
		explicit BlendSource(const HalfResTransparencyResult&) : BlendMode(EBlendMode::Modulate) {}

		EBlendMode::Type BlendMode;
	};
}


struct SimpleBlendPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::BlendSource>,
		OutputTable<RDAG::BlendDest>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};