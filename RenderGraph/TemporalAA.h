#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "VelocityPass.h"


namespace RDAG
{
	struct TransparencyResult;

	struct TemporalAAHistory : ExternalTexture2dResourceHandle
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAHistory";

		explicit TemporalAAHistory() {}
	};

	struct TemporalAAInput : Texture2dResourceHandle
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAInput";

		explicit TemporalAAInput() {}
		explicit TemporalAAInput(const TransparencyResult&) {}
	};

	struct TemporalAAOutput : ExternalTexture2dResourceHandle
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAOutput";

		explicit TemporalAAOutput() {}
		explicit TemporalAAOutput(const TemporalAAInput&) {}
	};
}

template<typename RenderContextType = RenderContext>
struct TemporalAARenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::TemporalAAInput, RDAG::DepthTarget, RDAG::VelocityVectors, RDAG::SceneViewInfo>,
		OutputTable<RDAG::TemporalAAOutput>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};