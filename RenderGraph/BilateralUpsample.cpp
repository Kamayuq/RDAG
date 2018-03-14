#include "BilateralUpsample.h"


typename BilateralUpsampleRenderPass::PassOutputType BilateralUpsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthTarget = Input.GetInputDescriptor<RDAG::DepthTexture>();
	auto ColorTarget = Input.GetInputDescriptor<RDAG::HalfResInput>();
	Texture2d::Descriptor UpsampleDescriptor;
	UpsampleDescriptor.Name = "UpsampleResult";
	UpsampleDescriptor.Format = ColorTarget.Format;
	UpsampleDescriptor.Height = DepthTarget.Height;
	UpsampleDescriptor.Width = DepthTarget.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::UpsampleResult>({ UpsampleDescriptor }),
		Builder.QueueRenderAction("BilateralUpsampleAction", [](RenderContext&, const PassOutputType&)
		{
			//auto UpsampleInput = FDataSet::GetStaticResource<RDAG::FHalfResInput>(RndCtx, Self->PassData);
			//(void)UpsampleInput;
			//auto UpsampleInputHalfResDepth = FDataSet::GetStaticResource<RDAG::FHalfResDepth>(RndCtx, Self->PassData);
			//(void)UpsampleInputHalfResDepth;
			//auto UpsampleInputDepth = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)UpsampleInputDepth;
			//auto DownsampleOutput = FDataSet::GetMutableResource<RDAG::FUpsampleResult>(RndCtx, Self->PassData);
			//(void)DownsampleOutput;
		
			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindStatic(UpsampleInput);
			//RndCtx.BindStatic(UpsampleInputHalfResDepth);
			//RndCtx.BindStatic(UpsampleInputDepth);
			//RndCtx.BindMutable(DownsampleOutput);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}