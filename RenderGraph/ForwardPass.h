#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"

namespace ESortOrder
{
	enum Enum
	{
		FrontToBack,
		BackToFront,
		Irrelevant,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Irrelevant) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};


namespace RDAG
{
	struct TransparencyResult;
	struct HalfResTransparencyResult; 

	struct ForwardRender : Texture2dResourceHandle
	{
		static constexpr const char* Name = "ForwardRender";
		
		explicit ForwardRender(ESortOrder::Type InSortOrder) : SortOrder(InSortOrder) {}
		explicit ForwardRender(const HalfResTransparencyResult&) : SortOrder(ESortOrder::BackToFront) {}
		explicit ForwardRender(const TransparencyResult&) : SortOrder(ESortOrder::BackToFront) {}
		
		ESortOrder::Type SortOrder;
	};
}


struct ForwardRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTarget, RDAG::ForwardRender>,
		OutputTable<RDAG::DepthTarget, RDAG::ForwardRender>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};