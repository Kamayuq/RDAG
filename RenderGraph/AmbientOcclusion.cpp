#include "AmbientOcclusion.h"

template<typename RenderContextType>
typename DistanceFieldAORenderPass<RenderContextType>::PassOutputType DistanceFieldAORenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor AoDescriptor;
	AoDescriptor.Width = ViewInfo.SceneWidth;
	AoDescriptor.Height = ViewInfo.SceneHeight;
	AoDescriptor.Name = "DistanceFieldAoTarget";
	AoDescriptor.Format = ERenderResourceFormat::L8;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::AmbientOcclusionResult>({ AoDescriptor }),
		Builder.QueueRenderAction("DistancefieldAOAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto AmbientOcclusionResult = FDataSet::GetMutableResource<RDAG::FAmbientOcclusionResult>(RndCtx, Self->PassData);
			//(void)AmbientOcclusionResult;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(AmbientOcclusionResult);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template DistanceFieldAORenderPass<RenderContext>::PassOutputType DistanceFieldAORenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DistanceFieldAORenderPass<ParallelRenderContext>::PassOutputType DistanceFieldAORenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DistanceFieldAORenderPass<VulkanRenderContext>::PassOutputType DistanceFieldAORenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);

template<typename RenderContextType>
typename HorizonBasedAORenderPass<RenderContextType>::PassOutputType HorizonBasedAORenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor AoDescriptor;
	AoDescriptor = Input.GetInputDescriptor<RDAG::DepthTarget>();
	AoDescriptor.Name = "HorizonBasedAoTarget";
	AoDescriptor.Format = ERenderResourceFormat::L8;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::AmbientOcclusionResult>({ AoDescriptor }),
		Builder.QueueRenderAction("HorizonBasedAOAction", [](RenderContextType&, const PassOutputType&)
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
template HorizonBasedAORenderPass<RenderContext>::PassOutputType HorizonBasedAORenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template HorizonBasedAORenderPass<ParallelRenderContext>::PassOutputType HorizonBasedAORenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template HorizonBasedAORenderPass<VulkanRenderContext>::PassOutputType HorizonBasedAORenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);


template<typename RenderContextType>
typename AmbientOcclusionSelectionPass<RenderContextType>::PassOutputType AmbientOcclusionSelectionPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	switch (ViewInfo.AmbientOcclusionType)
	{
		case EAmbientOcclusionType::DistanceField:
			return Builder.BuildRenderPass("DistanceFieldAORenderPass", DistanceFieldAORenderPass<RenderContextType>::Build)(Input);

		default:
		case EAmbientOcclusionType::HorizonBased:
			return Builder.BuildRenderPass("HorizonBasedAORenderPass", HorizonBasedAORenderPass<RenderContextType>::Build)(Input);
	}
}
template AmbientOcclusionSelectionPass<RenderContext>::PassOutputType AmbientOcclusionSelectionPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template AmbientOcclusionSelectionPass<ParallelRenderContext>::PassOutputType AmbientOcclusionSelectionPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template AmbientOcclusionSelectionPass<VulkanRenderContext>::PassOutputType AmbientOcclusionSelectionPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
