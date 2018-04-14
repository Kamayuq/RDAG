#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct GbufferTexture : Texture2dResourceHandle<GbufferTexture>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "GbufferTexture";

		explicit GbufferTexture() {}
		explicit GbufferTexture(const struct GbufferTarget&) {}
	};
}

struct GbufferRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};