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
	using PassInputType = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::HalfResTransparencyResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input)
	{
		auto TransparencyInfo = Input.GetDescriptor<RDAG::TransparencyTarget>();
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
					Builder.RenameEntry<RDAG::DepthTarget, RDAG::DownsampleInput>(),
					Builder.BuildRenderPass("HalfResTransparency_DownsampleRenderPass", DownsampleRenderPass::Build),
					Builder.RenameEntry<RDAG::DownsampleResult, RDAG::DepthTarget>()
				}),
				Builder.CreateResource<RDAG::ForwardRenderTarget>({ HalfResTransparencyDescriptor }),
				Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build, ESortOrder::BackToFront),
				Builder.RenameEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
				Builder.RenameEntry<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
			}),
			Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
			Builder.RenameEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
		}(Input);
	}
};

typename TransparencyRenderPass::PassOutputType TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
{
	auto Output = Input;

	if (ViewInfo.TransparencyEnabled)
	{
		Output = Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build, ESortOrder::BackToFront)(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.RenameEntry<RDAG::TransparencyTarget, RDAG::BlendSource>(0, 0),
			Builder.RenameEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build, EBlendMode::Modulate),
			Builder.RenameEntry<RDAG::BlendDest, RDAG::TransparencyTarget>()
		}(Output);
	}
	return Output;
}
