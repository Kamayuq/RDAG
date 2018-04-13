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
		explicit VelocityVectors(const struct VelocityVectorTarget&) {}
	};

	struct VelocityVectorTarget : RendertargetResourceHandle<VelocityVectors>
	{
		static constexpr const char* Name = "VelocityVectorTarget";
		explicit VelocityVectorTarget() {}
		explicit VelocityVectorTarget(const VelocityVectors&) {}
	};
}


struct VelocityRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::VelocityVectorTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};