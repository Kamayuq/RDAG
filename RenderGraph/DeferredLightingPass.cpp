#include "DeferredLightingPass.h"

template<typename RenderContextType>
typename DeferredLightingPass<RenderContextType>::PassOutputType DeferredLightingPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetInputDescriptor<RDAG::DepthTarget>();
	Texture2d::Descriptor LightingDescriptor;
	LightingDescriptor.Name = "LightingRenderTarget";
	LightingDescriptor.Format = ERenderResourceFormat::ARGB16F;
	LightingDescriptor.Height = DepthInfo.Height;
	LightingDescriptor.Width = DepthInfo.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::LightingResult>({ LightingDescriptor }),
		Builder.QueueRenderAction("DeferredLightingAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto DepthTarget = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto GbufferA = FDataSet::GetStaticResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 0);
			//(void)GbufferA;
			//auto GbufferB = FDataSet::GetStaticResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 1);
			//(void)GbufferB;
			//auto AOBuffer = FDataSet::GetStaticResource<RDAG::FAmbientOcclusionResult>(RndCtx, Self->PassData);
			//(void)AOBuffer;
			//auto LightingResult = FDataSet::GetMutableResource<RDAG::FLightingResult>(RndCtx, Self->PassData);
			//(void)LightingResult;

			/*
			RndCtx.SetRenderPass(Self);
			RndCtx.BindStatic(DepthTarget);
			RndCtx.BindStatic(GbufferA);
			RndCtx.BindStatic(GbufferB);
			RndCtx.BindStatic(AOBuffer);
			for (U32 Index = 0; Index < Config.ShadowConfig.NumCascades; Index++)
			{
			auto ShadowMap = FDataSet::GetStaticResource<RDAG::FShadowMapTextureArray>(RndCtx, Self->PassData, Index);
			RndCtx.BindStatic(ShadowMap);
			}
			RndCtx.BindMutable(LightingResult);
			RndCtx.SetRenderPass(nullptr);
			*/
		})
	)(Input);
}
template DeferredLightingPass<RenderContext>::PassOutputType DeferredLightingPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DeferredLightingPass<ParallelRenderContext>::PassOutputType DeferredLightingPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DeferredLightingPass<VulkanRenderContext>::PassOutputType DeferredLightingPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
