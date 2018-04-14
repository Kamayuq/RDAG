#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "SharedResources.h"

namespace RDAG
{
	struct DepthTexture : Texture2dResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTexture";
		explicit DepthTexture() {}
		explicit DepthTexture(const struct DepthTarget&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource) const;
	};

	struct DepthTarget : RendertargetResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTarget";
		explicit DepthTarget() {}
		explicit DepthTarget(const DepthTexture&) {}
		explicit DepthTarget(const struct DownsampleResult&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTarget::ResourceType& Resource) const;
	};
}


struct DepthRenderPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};