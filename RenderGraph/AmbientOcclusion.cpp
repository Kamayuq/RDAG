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
				Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContextType&, const DFAOTable&)
				{
					//auto AmbientOcclusionResult = FDataSet::GetMutableResource<RDAG::FAmbientOcclusionResult>(RndCtx, Self->PassData);
					//(void)AmbientOcclusionResult;

					//RndCtx.SetRenderPass(Self);
					//RndCtx.BindMutable(AmbientOcclusionResult);
					//RndCtx.SetRenderPass(nullptr);
				})
			)(Input);
		}

		default:
		case EAmbientOcclusionType::HorizonBased:
		{
			AoDescriptor.Name = "HorizonBasedAoTarget";
			using HBAOTable = ResourceTable<InputTable<RDAG::Gbuffer, RDAG::DepthTarget>, OutputTable<RDAG::AmbientOcclusionResult>>;

			return Seq
			(
				Builder.CreateOutputResource<RDAG::AmbientOcclusionResult>({ AoDescriptor }),
				Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContextType&, const HBAOTable&)
				{
					//auto DepthBuffer = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
					//(void)DepthBuffer;
					//auto GbufferB = FDataSet::GetStaticResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 1);
					//(void)GbufferB;
					//auto AmbientOcclusionResult = FDataSet::GetMutableResource<RDAG::FAmbientOcclusionResult>(RndCtx, Self->PassData);
					//(void)AmbientOcclusionResult;

					//RndCtx.SetRenderPass(Self);
					//RndCtx.BindStatic(DepthBuffer);
					//RndCtx.BindStatic(GbufferB);
					//RndCtx.BindMutable(AmbientOcclusionResult);
					//RndCtx.SetRenderPass(nullptr);
				})
			)(Input);
		}
	}
}