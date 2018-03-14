#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
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
		explicit DepthTexture(const DepthTarget&) : ResourceTransition(EResourceTransition::DepthWriteToDepthRead) {}

		void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource) const;

	private:
		EResourceTransition::Type ResourceTransition = EResourceTransition::None;
	};

	struct DepthTarget : DepthTexture 
	{
		static constexpr const char* Name = "DepthTarget";
		explicit DepthTarget() {}
		explicit DepthTarget(const DepthTexture&) : ResourceTransition(EResourceTransition::DepthReadToDepthWrite) {}
		explicit DepthTarget(const DownsampleResult&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTarget::ResourceType& Resource) const;

	private:
		EResourceTransition::Type ResourceTransition = EResourceTransition::None;
	};

	struct HalfResDepthTarget : Texture2dResourceHandle<DepthTarget>
	{
		static constexpr const char* Name = "HalfResDepthTarget";
		explicit HalfResDepthTarget() {}
	};
}


struct DepthRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo>,
		OutputTable<RDAG::DepthTarget>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};