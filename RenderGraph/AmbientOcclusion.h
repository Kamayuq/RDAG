#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	struct AmbientOcclusionResult : Texture2dResourceHandle
	{
		static constexpr const char* Name = "AmbientOcclusionResult";

		explicit AmbientOcclusionResult() {}
	};
}

template<typename RenderContextType = RenderContext>
struct DistanceFieldAORenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo>,
		OutputTable<RDAG::AmbientOcclusionResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

template<typename RenderContextType = RenderContext>
struct HorizonBasedAORenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::Gbuffer, RDAG::DepthTarget>,
		OutputTable<RDAG::AmbientOcclusionResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

template<typename RenderContextType = RenderContext>
struct AmbientOcclusionSelectionPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo, RDAG::Gbuffer, RDAG::DepthTarget>,
		OutputTable<RDAG::AmbientOcclusionResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};