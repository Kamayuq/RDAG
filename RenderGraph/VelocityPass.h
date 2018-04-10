#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"

namespace RDAG
{
	struct VelocityVectors : Texture2dResourceHandle<VelocityVectors>
	{
		static constexpr const char* Name = "VelocityVectors";
		explicit VelocityVectors() {}
	};
}


struct VelocityRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::VelocityVectors>;
	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};