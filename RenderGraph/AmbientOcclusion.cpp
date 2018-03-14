#include "AmbientOcclusion.h"


typename AmbientOcclusionPass::PassOutputType AmbientOcclusionPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const auto& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor AoDescriptor;
	AoDescriptor.Width = ViewInfo.SceneWidth;
	AoDescriptor.Height = ViewInfo.SceneHeight;
	AoDescriptor.Format = ERenderResourceFormat::L8;

	switch (ViewInfo.AmbientOcclusionType)
	{
		case EAmbientOcclusionType::DistanceField:
		{
			AoDescriptor.Name = "DistanceFieldAoTarget";
			using DFAOTable = ResourceTable<InputTable<RDAG::SceneViewInfo>, OutputTable<RDAG::AmbientOcclusionResult>>;

			return Seq
			(
				Builder.CreateOutputResource<RDAG::AmbientOcclusionResult>({ AoDescriptor }),
				Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContext& Ctx, const DFAOTable&)
				{
					Ctx.Draw("DistancefieldAOAction");
				})
			)(Input);
		}

		default:
		case EAmbientOcclusionType::HorizonBased:
		{
			AoDescriptor.Name = "HorizonBasedAoTarget";
			using HBAOTable = ResourceTable<InputTable<RDAG::Gbuffer, RDAG::DepthTexture>, OutputTable<RDAG::AmbientOcclusionResult>>;

			return Seq
			(
				Builder.CreateOutputResource<RDAG::AmbientOcclusionResult>({ AoDescriptor }),
				Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContext& Ctx, const HBAOTable&)
				{
					Ctx.Draw("HorizonBasedAOAction");
				})
			)(Input);
		}
	}
}