#pragma once
#include "Assert.h"
#include "Types.h"
#include "ExampleResourceTypes.h"
#include <iostream>

struct RenderResourceBase;
struct RenderPassBase;

struct RenderContextBase
{
private:
	static constexpr const char* TransitionStr[] = { "Texture", "Target", "UAV", "DepthTexture", "DepthTarget", "Undefined" };

public:
	void TransitionResource(const struct Texture2d& Tex, EResourceTransition::Type Transition)
	{
		static_assert(sizeofArray(TransitionStr) == EResourceTransition::Undefined + 1, "Array out of bounds check failed");
		EResourceTransition::Type OldState;
		if (Tex.RequiresTransition(OldState, Transition))
		{
			printf("TransitionTexture: %s from %s to: %s \n", Tex.GetName(), TransitionStr[OldState], TransitionStr[Transition]);
		}
	}

	void BindTexture(const struct Texture2d& Tex)
	{
		printf("BindTexture: %s \n", Tex.GetName());
	}

	void BindRenderTarget(const struct Texture2d& Tex)
	{
		printf("BindRenderTarget: %s \n", Tex.GetName());
	}

	void Draw(const char* RenderPass)
	{
		printf("Drawing Renderpass: %s \n", RenderPass);
		printf("/********************************/ \n");
	}
};

struct RenderContext : protected RenderContextBase
{
	using RenderContextBase::BindTexture;
	using RenderContextBase::Draw;
};

struct ImmediateRenderContext final : public RenderContext
{
	using RenderContextBase::TransitionResource;
	using RenderContextBase::BindRenderTarget;
};