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

	struct ForwardRenderTarget : Texture2dResourceHandle<ForwardRenderTarget>
	{
		static constexpr const char* Name = "ForwardRenderTarget";
		
		explicit ForwardRenderTarget(ESortOrder::Type InSortOrder) : SortOrder(InSortOrder) {}
		explicit ForwardRenderTarget(const TransparencyResult&) : SortOrder(ESortOrder::BackToFront) {}
		
		void OnExecute(ImmediateRenderContext& Ctx, const ForwardRenderTarget::ResourceType& Resource) const
		{
			Ctx.TransitionResource(Resource, EResourceTransition::Target);
			Ctx.BindRenderTarget(Resource);
		}

		ESortOrder::Type SortOrder;
	};
}


struct ForwardRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget, RDAG::ForwardRenderTarget>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget, RDAG::ForwardRenderTarget>;
	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};