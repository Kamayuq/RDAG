#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "SharedResources.h"

namespace RDAG
{
	struct DownsampleResult;
	struct DepthTarget;

	struct DepthTexture : Texture2dResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTexture";
		explicit DepthTexture() {}
		explicit DepthTexture(const DepthTarget&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource) const;
	};

	struct DepthTarget : RendertargetResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTarget";
		explicit DepthTarget() {}
		explicit DepthTarget(const DepthTexture&) {}
		explicit DepthTarget(const DownsampleResult&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTarget::ResourceType& Resource) const;
	};

	struct HalfResDepthTarget : Texture2dResourceHandle<DepthTarget>
	{
		static constexpr const char* Name = "HalfResDepthTarget";
		explicit HalfResDepthTarget() {}
	};
}


struct DepthRenderPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};