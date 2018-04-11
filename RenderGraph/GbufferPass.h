#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct GbufferTexture : RendertargetResourceHandle<GbufferTexture>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "GbufferTexture";

		explicit GbufferTexture() {}
		explicit GbufferTexture(const struct GbufferTarget&) {}
	};

	struct GbufferTarget : RendertargetResourceHandle<GbufferTexture>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "GbufferTarget";

		explicit GbufferTarget() {}
	};
}


struct GbufferRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget, RDAG::GbufferTarget>;
	using PassActionType = ResourceTable<RDAG::DepthTarget, RDAG::GbufferTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};