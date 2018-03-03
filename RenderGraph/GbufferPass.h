#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct Gbuffer : Texture2dResourceHandle
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "Gbuffer";

		explicit Gbuffer() {}
	};
}

template<typename RenderContextType = RenderContext>
struct GbufferRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTarget>,
		OutputTable<RDAG::DepthTarget, RDAG::Gbuffer>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};