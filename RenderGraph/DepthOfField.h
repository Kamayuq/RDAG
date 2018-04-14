#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"
#include "VelocityPass.h"

struct DepthOfFieldPass
{
	using PassInputType = ResourceTable<RDAG::SceneColorTexture, RDAG::VelocityVectors, RDAG::DepthTexture, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::SceneColorTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};