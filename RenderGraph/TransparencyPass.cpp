#include "TransparencyPass.h"
#include "DownSamplePass.h"
#include "BilateralUpsample.h"
#include "SimpleBlendPass.h"

namespace RDAG
{
	struct HalfResTransparencyResult : Texture2dResourceHandle<HalfResTransparencyResult>
	{
		static constexpr const char* Name = "HalfResTransparencyResult";
		explicit HalfResTransparencyResult() {}
		explicit HalfResTransparencyResult(const struct UpsampleResult&) {}
	};
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
				Builder.CreateResource<RDAG::ForwardRenderTarget>({ HalfResTransparencyDescriptor }, ESortOrder::BackToFront),
				Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build),
				Builder.RenameEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
				Builder.RenameEntry<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
			}),
			Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
			Builder.RenameEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
		}(Input);
	}
};

typename TransparencyRenderPass::PassOutputType TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto Output = Input;

	const RDAG::SceneViewInfo& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	if (ViewInfo.TransparencyEnabled)
	{
		Output = Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build)(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.RenameEntry<RDAG::TransparencyTarget, RDAG::BlendSource>(0, 0),
			Builder.RenameEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build),
			Builder.RenameEntry<RDAG::BlendDest, RDAG::TransparencyTarget>()
		}(Output);
	}
	return Output;
}
