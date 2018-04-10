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
	using PassInputType = ResourceTable<RDAG::DepthOfFieldInput, RDAG::SceneViewInfo, RDAG::DepthTexture, RDAG::VelocityVectors>;
	using PassOutputType = ResourceTable<RDAG::DepthOfFieldOutput>;
	using PassActionType = ResourceTable<RDAG::DepthOfFieldOutput, RDAG::DepthOfFieldInput, RDAG::SceneViewInfo, RDAG::DepthTexture, RDAG::VelocityVectors>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};