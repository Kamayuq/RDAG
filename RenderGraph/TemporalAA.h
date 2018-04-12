#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "VelocityPass.h"


namespace RDAG
{
	struct TemporalAAHistory : ExternalTexture2dResourceHandle<TemporalAAHistory>
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAHistory";

		explicit TemporalAAHistory() {}
	};

	struct TemporalAAInput : Texture2dResourceHandle<TemporalAAInput>
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAInput";

		explicit TemporalAAInput() {}
		explicit TemporalAAInput(const struct TransparencyTarget&) {}

		template<typename CRTP>
		explicit TemporalAAInput(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct TemporalAAOutput : ExternalUav2dResourceHandle<TemporalAAOutput>
	{
		static constexpr const U32 ResourceCount = RDAG::SceneViewInfo::TemporalAAResourceCount;
		static constexpr const char* Name = "TemporalAAOutput";

		explicit TemporalAAOutput() {}
		explicit TemporalAAOutput(const TemporalAAInput&) {}
	};
}


struct TemporalAARenderPass
{
	using PassInputType = ResourceTable<RDAG::TemporalAAInput, RDAG::DepthTexture, RDAG::VelocityVectors, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::TemporalAAOutput>;
	using PassActionType = ResourceTable<RDAG::TemporalAAOutput, RDAG::TemporalAAInput, RDAG::DepthTexture, RDAG::VelocityVectors, RDAG::SceneViewInfo>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};