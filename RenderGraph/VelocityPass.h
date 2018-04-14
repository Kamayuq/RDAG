#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"

namespace RDAG
{
	struct VelocityVectors : Texture2dResourceHandle<VelocityVectors>
	{
		static constexpr const char* Name = "VelocityVectors";
		explicit VelocityVectors() {}
		explicit VelocityVectors(const struct VelocityVectorTarget&) {}
	};
}

struct VelocityRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::VelocityVectors>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};