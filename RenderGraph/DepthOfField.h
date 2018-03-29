#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "SharedResources.h"
#include "VelocityPass.h"

namespace RDAG
{
	struct DepthOfFieldInput : Texture2dResourceHandle<DepthOfFieldInput>
	{
		static constexpr const char* Name = "DepthOfFieldInput";

		explicit DepthOfFieldInput() {}

		template<typename CRTP>
		explicit DepthOfFieldInput(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct DepthOfFieldOutput : Texture2dResourceHandle<DepthOfFieldOutput>
	{
		static constexpr const char* Name = "DepthOfFieldOutput";

		explicit DepthOfFieldOutput() {}
		explicit DepthOfFieldOutput(const DepthOfFieldInput&) {}
	};
}

struct DepthOfFieldPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo, RDAG::DepthOfFieldInput, RDAG::DepthTexture, RDAG::VelocityVectors>,
		OutputTable<RDAG::DepthOfFieldOutput>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};