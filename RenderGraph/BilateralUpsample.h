#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	struct DepthTarget;
	struct ForwardRender;

	struct UpsampleResult : Texture2dResourceHandle
	{
		static constexpr const char* Name = "UpsampleResult";

		explicit UpsampleResult() {}
	};

	struct HalfResInput : Texture2dResourceHandle
	{
		static constexpr const char* Name = "HalfResInput";

		explicit HalfResInput() {}
		explicit HalfResInput(const ForwardRender&) {}
	};

	struct HalfResDepth : Texture2dResourceHandle
	{
		static constexpr const char* Name = "HalfResDepth";

		explicit HalfResDepth() {}
		explicit HalfResDepth(const DepthTarget&) {}
	};
}


struct BilateralUpsampleRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::HalfResInput, RDAG::HalfResDepth, RDAG::DepthTarget>,
		OutputTable<RDAG::UpsampleResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};