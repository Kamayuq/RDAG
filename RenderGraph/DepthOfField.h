#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"
#include "VelocityPass.h"

struct DepthOfFieldPass
{
	using DepthOfFieldInput = ResourceTable<RDAG::SceneColorTexture, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using DepthOfFieldResult = ResourceTable<RDAG::SceneColorTexture>;

	static DepthOfFieldResult Build(const RenderPassBuilder& Builder, const DepthOfFieldInput& Input, const SceneViewInfo& ViewInfo);
};