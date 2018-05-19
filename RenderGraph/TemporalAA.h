#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "VelocityPass.h"

namespace RDAG
{
	struct TemporalAAInput : Texture2dResourceHandle<TemporalAAInput>
	{
		static constexpr const char* Name = "TemporalAAInput";

		explicit TemporalAAInput() {}
		explicit TemporalAAInput(const struct TransparencyTarget&) {}

		template<typename CRTP>
		explicit TemporalAAInput(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct TemporalAAOutput : ExternalUav2dResourceHandle<TemporalAAOutput>
	{
		static constexpr const char* Name = "TemporalAAOutput";

		explicit TemporalAAOutput() {}
		explicit TemporalAAOutput(const TemporalAAInput&) {}
	};
}

struct TemporalAARenderPass
{
	using PassInputType = ResourceTable<RDAG::TemporalAAInput, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::TemporalAAOutput>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};