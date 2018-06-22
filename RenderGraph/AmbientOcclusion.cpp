#include "AmbientOcclusion.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(AmbientOcclusionUAV, AmbientOcclusionTexture);
}

typename AmbientOcclusionPass::AmbientOcclusionResult AmbientOcclusionPass::Build(const RenderPassBuilder& Builder, const AmbientOcclusionInput& Input, const SceneViewInfo& ViewInfo)
{
	Texture2d::Descriptor AoDescriptor;
	AoDescriptor.Width = ViewInfo.SceneWidth;
	AoDescriptor.Height = ViewInfo.SceneHeight;
	AoDescriptor.Format = ERenderResourceFormat::L8;

	switch (ViewInfo.AmbientOcclusionType)
	{
		case EAmbientOcclusionType::DistanceField:
		{
			AoDescriptor.Name = "DistanceFieldAoTarget";
			using DFAOAction = ResourceTable<RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>( AoDescriptor ),
				Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContext& Ctx, const DFAOAction&)
				{
					Ctx.Draw("DistancefieldAOAction");
				})
			}(Input);
		}

		default:
		case EAmbientOcclusionType::HorizonBased:
		{
			AoDescriptor.Name = "HorizonBasedAoTarget";
			using HBAOAction = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTexture, RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>( AoDescriptor ),
				Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContext& Ctx, const HBAOAction&)
				{
					Ctx.Draw("HorizonBasedAOAction");
				})
			}(Input);
		}
	}
}