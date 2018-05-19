#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"

namespace ESortOrder
{
	enum Enum
	{
		FrontToBack,
		BackToFront,
		Irrelevant,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Irrelevant) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};


namespace RDAG
{
	struct ForwardRenderTarget : RendertargetResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "ForwardRenderTarget";
		
		explicit ForwardRenderTarget() {}
		explicit ForwardRenderTarget(const struct TransparencyTarget&) {}
	};
}


struct ForwardRenderPass
{
	using PassInputType = ResourceTable<RDAG::ForwardRenderTarget, RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::ForwardRenderTarget, RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, ESortOrder::Type SortOrder);
};