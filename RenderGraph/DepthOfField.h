#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"
#include "VelocityPass.h"

namespace RDAG
{
	template<int>
	struct DownsamplePyramid;

	struct DepthOfFieldUav: Uav2dResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "DepthOfFieldUav";

		explicit DepthOfFieldUav() {}

		template<int Count>
		explicit DepthOfFieldUav(const DownsamplePyramid<Count>&) {}
	};
}

struct DepthOfFieldPass
{
	using PassInputType = ResourceTable<RDAG::DepthOfFieldUav, RDAG::VelocityVectors, RDAG::DepthTexture, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::DepthOfFieldUav>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};