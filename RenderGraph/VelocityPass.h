#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"

namespace RDAG
{
	struct VelocityVectors : Texture2dResourceHandle<VelocityVectors>
	{
		static constexpr const char* Name = "VelocityVectors";
		explicit VelocityVectors() {}
	};
}


struct VelocityRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTexture>,
		OutputTable<RDAG::VelocityVectors>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};