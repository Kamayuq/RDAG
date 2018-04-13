#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "ForwardPass.h"

namespace RDAG
{
	struct TransparencyTarget : RendertargetResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "TransparencyTarget";
		explicit TransparencyTarget() {}

		explicit TransparencyTarget(const struct SceneColorTexture&) {}
		explicit TransparencyTarget(const struct ForwardRenderTarget&) {}
		explicit TransparencyTarget(const struct BlendDest&) {}
		explicit TransparencyTarget(const struct TemporalAAOutput&) {}
	};
}

struct TransparencyRenderPass
{
	using PassInputType = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};