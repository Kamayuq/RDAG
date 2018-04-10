#include "AmbientOcclusion.h"

namespace RDAG
{
	void AmbientOcclusionTexture::OnExecute(ImmediateRenderContext& Ctx, const AmbientOcclusionTexture::ResourceType& Resource) const
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Texture);
		Ctx.BindTexture(Resource);
	}

	struct AmbientOcclusionUAV : Texture2dResourceHandle<AmbientOcclusionTexture>
	{
		static constexpr const char* Name = "AmbientOcclusionUAV";
		explicit AmbientOcclusionUAV() {}
		explicit AmbientOcclusionUAV(const AmbientOcclusionTexture&) {}

		void OnExecute(ImmediateRenderContext& Ctx, const AmbientOcclusionUAV::ResourceType& Resource) const
		{
			Ctx.TransitionResource(Resource, EResourceTransition::UAV);
			Ctx.BindTexture(Resource);
		}
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
				Builder.QueueRenderAction<RDAG::AmbientOcclusionUAV>("DistancefieldAOAction", [](RenderContext& Ctx, const DFAOTable&)
				{
					Ctx.Draw("DistancefieldAOAction");
				})
			}(Input);
		}

		default:
		case EAmbientOcclusionType::HorizonBased:
		{
			AoDescriptor.Name = "HorizonBasedAoTarget";
			using HBAOTable = ResourceTable<RDAG::GbufferTarget, RDAG::DepthTexture, RDAG::AmbientOcclusionUAV>;

			return Seq
			{
				Builder.CreateResource<RDAG::AmbientOcclusionUAV>({ AoDescriptor }),
				Builder.QueueRenderAction<RDAG::AmbientOcclusionUAV>("HorizonBasedAOAction", [](RenderContext& Ctx, const HBAOTable&)
				{
					Ctx.Draw("HorizonBasedAOAction");
				})
			}(Input);
		}
	}
}