#include "AmbientOcclusion.h"

namespace RDAG
{
	struct AmbientOcclusionUAV : Uav2dResourceHandle<AmbientOcclusionTexture>
	{
		static constexpr const char* Name = "AmbientOcclusionUAV";
		explicit AmbientOcclusionUAV() {}
		explicit AmbientOcclusionUAV(const AmbientOcclusionTexture&) {}
	};
}

typename AmbientOcclusionPass::PassOutputType AmbientOcclusionPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const auto& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor AoDescriptor;
	AoDescriptor.Width = ViewInfo.SceneWidth;
	AoDescriptor.Height = ViewInfo.SceneHeight;
	AoDescriptor.Format = ERenderResourceFormat::L8;

	switch (ViewInfo.AmbientOcclusionType)
	{
		case EAmbientOcclusionType::DistanceField:
		{
			AoDescriptor.Name = "DistanceFieldAoTarget";
			using DFAOTable = ResourceTable<RDAG::SceneViewInfo, RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>({ AoDescriptor }),
				Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContext& Ctx, const DFAOTable& Resources) -> PassOutputType
				{
					Ctx.Draw("DistancefieldAOAction");
					return Resources;
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
				Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContext& Ctx, const HBAOTable& Resources) -> PassOutputType
				{
					Ctx.Draw("HorizonBasedAOAction");
					return Resources;
				})
			}(Input);
		}
	}
}