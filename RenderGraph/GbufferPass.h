#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct GbufferTarget : Texture2dResourceHandle<GbufferTarget>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "GbufferTarget";

		explicit GbufferTarget() {}

		void OnExecute(ImmediateRenderContext& Ctx, const GbufferTarget::ResourceType& Resource) const
		{
			Ctx.TransitionResource(Resource, EResourceTransition::Target);
			Ctx.BindRenderTarget(Resource);
		}
	};
}


struct GbufferRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget, RDAG::GbufferTarget>;
	using PassActionType = ResourceTable<RDAG::DepthTarget, RDAG::GbufferTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};