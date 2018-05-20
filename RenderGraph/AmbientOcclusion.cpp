#include "AmbientOcclusion.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(AmbientOcclusionUAV, AmbientOcclusionTexture);
}

typename AmbientOcclusionPass::PassOutputType AmbientOcclusionPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
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
			using DFAOTable = ResourceTable<RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>({ AoDescriptor }),
				Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContext& Ctx, const DFAOTable&)
				{
					Ctx.Draw("DistancefieldAOAction");
				})
			}(Input);
		}

		default:
		case EAmbientOcclusionType::HorizonBased:
		{
			AoDescriptor.Name = "HorizonBasedAoTarget";
			using HBAOTable = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTexture, RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>({ AoDescriptor }),
				Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContext& Ctx, const HBAOTable&)
				{
					Ctx.Draw("HorizonBasedAOAction");
				})
			}(Input);
		}
	}
}