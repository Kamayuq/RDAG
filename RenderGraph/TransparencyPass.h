#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "ForwardPass.h"

namespace RDAG
{
	struct TransparencyInput : Texture2dResourceHandle<TransparencyInput>
	{
		static constexpr const char* Name = "TransparencyInput";
		explicit TransparencyInput() {}
		explicit TransparencyInput(const struct LightingUAV&) {}
	};

	struct TransparencyResult : Texture2dResourceHandle<TransparencyResult>
	{
		static constexpr const char* Name = "TransparencyResult";
		explicit TransparencyResult() {}
		explicit TransparencyResult(const TransparencyInput&) {}
		explicit TransparencyResult(const struct ForwardRenderTarget&) {}
		explicit TransparencyResult(const struct BlendDest&) {}
		explicit TransparencyResult(const struct TemporalAAOutput&) {}
	};

	struct HalfResTransparencyResult : Texture2dResourceHandle<HalfResTransparencyResult>
	{
		static constexpr const char* Name = "HalfResTransparencyResult";
		explicit HalfResTransparencyResult() {}
		explicit HalfResTransparencyResult(const struct UpsampleResult&) {}
	};
}


struct HalfResTransparencyRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTarget, RDAG::TransparencyInput>,
		OutputTable<RDAG::HalfResTransparencyResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};


struct TransparencyRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTarget, RDAG::TransparencyInput, RDAG::SceneViewInfo>,
		OutputTable<RDAG::DepthTarget, RDAG::TransparencyResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};