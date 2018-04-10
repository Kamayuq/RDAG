#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct UpsampleResult : Texture2dResourceHandle<UpsampleResult>
	{
		static constexpr const char* Name = "UpsampleResult";

		explicit UpsampleResult() {}
	};

	struct HalfResInput : Texture2dResourceHandle<HalfResInput>
	{
		static constexpr const char* Name = "HalfResInput";

		explicit HalfResInput() {}
		explicit HalfResInput(const struct ForwardRenderTarget&) {}
	};

	struct HalfResDepth : Texture2dResourceHandle<HalfResDepth>
	{
		static constexpr const char* Name = "HalfResDepth";

		explicit HalfResDepth() {}
		explicit HalfResDepth(const struct DepthTarget&) {}
	};
}


struct BilateralUpsampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::HalfResInput, RDAG::HalfResDepth, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::UpsampleResult>;
	using PassActionType = ResourceTable<RDAG::UpsampleResult, RDAG::HalfResInput, RDAG::HalfResDepth, RDAG::DepthTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};