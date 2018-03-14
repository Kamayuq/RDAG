#pragma once
#include <iostream>
#include "Assert.h"
#include "Types.h"

struct RenderResourceBase;
struct RenderPassBase;

struct RenderContextBase
{
private:
	static constexpr const char* TransitionStr[] = { "Common", "DepthRead", "DepthWrite" };

public:
	void TransitionResource(const struct Texture2d& Tex, EResourceTransition::Type Transition)
	{
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