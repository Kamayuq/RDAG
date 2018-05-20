#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "SharedResources.h"

namespace RDAG
{
	struct DepthTexture : Texture2dResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTexture";
		static void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource);
	};

	struct DepthTarget : RendertargetResourceHandle<DepthTexture>
	{
		static constexpr const char* Name = "DepthTarget";
		static void OnExecute(ImmediateRenderContext&, const DepthTarget::ResourceType& Resource);
	};
}


struct DepthRenderPass
{
	using PassInputType = ResourceTable<>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};