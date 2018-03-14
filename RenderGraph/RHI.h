#pragma once
#include <iostream>
#include "Assert.h"
#include "Types.h"

namespace ERenderBackend
{
	enum Type
	{
		Sequence,
		Parallel,
		Vulkan,
	};
};

struct RenderResourceBase;
struct RenderPassBase;

struct RenderContextBase
{
protected:
	//RenderPassBase* RenderPass = nullptr;

public:
	void TransitionResource(const struct Texture2d& Tex, EResourceTransition::Type Transition)
	{
		printf("TransitionTexture: %s to: %d \n", Tex.GetName(), Transition);
	}

	void BindTexture(const struct Texture2d& Tex)
	{
		printf("BindTexture: %s \n", Tex.GetName());
	}
};

struct RenderContext : protected RenderContextBase
{
	using RenderContextBase::BindTexture;
};

struct ImmediateRenderContext final : public RenderContext
{
	using RenderContextBase::TransitionResource;
	using RenderContextBase::BindTexture;
};