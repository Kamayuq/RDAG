#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"

#define ASYNC_VELOCITY 1

namespace RDAG
{
	struct VelocityVectors : Texture2dResourceHandle
	{
		static constexpr const char* Name = "VelocityVectors";
		explicit VelocityVectors() {}
	};
}

template<typename RenderContextType = RenderContext>
struct VelocityRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTarget>,
		OutputTable<RDAG::VelocityVectors>
	);

#if ASYNC_VELOCITY
	typedef Promise<PassOutputType> PromiseType;
	using ReturnType=PromiseType;
#else
	using ReturnType=PassOutputType;
#endif

	static ReturnType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};