#include "TransparencyPass.h"
#include "DownSamplePass.h"
#include "BilateralUpsample.h"
#include "SimpleBlendPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(HalfResTransparencyResult);
}

struct HalfResTransparencyRenderPass
{
	using HalfResTransparencyRenderInput = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;
	using HalfResTransparencyRenderResult = ResourceTable<RDAG::HalfResTransparencyResult>;

	static HalfResTransparencyRenderResult Build(const RenderPassBuilder& Builder, const HalfResTransparencyRenderInput& Input)
	{
		const Texture2d::Descriptor& TransparencyInfo = Input.GetDescriptor<RDAG::TransparencyTarget>();
		Texture2d::Descriptor HalfResTransparencyDescriptor;
		HalfResTransparencyDescriptor.Name = "HalfResTransparencyRenderTarget";
		HalfResTransparencyDescriptor.Format = TransparencyInfo.Format;
		HalfResTransparencyDescriptor.Height = TransparencyInfo.Height >> 1;
		HalfResTransparencyDescriptor.Width = TransparencyInfo.Width >> 1;

		return Seq
		{
			Extract<RDAG::HalfResInput, RDAG::HalfResDepth>(Seq
			{
				Scope(Seq
				{
					Builder.AssignEntry<RDAG::DepthTarget, RDAG::DownsampleDepthInput>(),
					Builder.BuildRenderPass("HalfResTransparency_DownsampleDepthRenderPass", DownsampleDepthRenderPass::Build),
					Builder.AssignEntry<RDAG::DownsampleDepthResult, RDAG::DepthTarget>()
				}),
				Builder.CreateResource<RDAG::ForwardRenderTarget>({ HalfResTransparencyDescriptor }),
				Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build, ESortOrder::BackToFront),
				Builder.AssignEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
				Builder.AssignEntry<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
			}),
			Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
			Builder.AssignEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
		}(Input);
	}
};

typename TransparencyRenderPass::TransparencyRenderResult TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const TransparencyRenderInput& Input, const SceneViewInfo& ViewInfo)
{
	TransparencyRenderResult Output = Input;

	if (ViewInfo.TransparencyEnabled)
	{
		Output = Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build, ESortOrder::BackToFront)(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.AssignEntry<RDAG::TransparencyTarget, RDAG::BlendSourceA>(),
			Builder.AssignEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSourceB>(),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build, EBlendMode::Modulate),
			Builder.AssignEntry<RDAG::BlendDest, RDAG::TransparencyTarget>()
		}(Output);
	}
	return Output;
}
