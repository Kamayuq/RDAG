#include "TemporalAA.h"


typename TemporalAARenderPass::PassOutputType TemporalAARenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	ExternalTexture2dResourceHandle::Descriptor OutputDescriptors[RDAG::SceneViewInfo::TemporalAAResourceCount];
	ExternalTexture2dResourceHandle::Descriptor HistoryDescriptors[RDAG::SceneViewInfo::TemporalAAResourceCount];
	for (U32 i = 0; i < RDAG::SceneViewInfo::TemporalAAResourceCount; i++)
	{
		HistoryDescriptors[i] = Input.GetInputDescriptor<RDAG::TemporalAAInput>();
		HistoryDescriptors[i].Index = Input.IsValidInput<RDAG::TemporalAAInput>(i) ? i : -1;
		HistoryDescriptors[i].Name = "TemporalAAHistory";
		OutputDescriptors[i] = HistoryDescriptors[i];
	}

	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();

	if (ViewInfo.TemporalAaEnabled)
	{
		auto MergedTable = Seq
		(
			Builder.CreateOutputResource<RDAG::TemporalAAOutput>(OutputDescriptors),
			Builder.CreateInputResource<RDAG::TemporalAAHistory>(HistoryDescriptors)
		)(Input);

		typedef decltype(MergedTable) MergedTableType;

		return Builder.QueueRenderAction("TemporalAAAction", [](RenderContextType&, const MergedTableType&)
		{
			//auto DepthTarget = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto VelocityBuffer = FDataSet::GetStaticResource<RDAG::FVelocityVectors>(RndCtx, Self->PassData);
			//(void)VelocityBuffer;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindStatic(DepthTarget);
			//RndCtx.BindStatic(VelocityBuffer);
			//
			//for (U32 i = 0; i < RDAG::FTemporalAAHistory::ResourceCount; i++)
			//{
			//	if (Self->PassData.template IsValidStatic<RDAG::FTemporalAAHistory>(i))
			//	{
			//		auto History = FDataSet::GetStaticResource<RDAG::FTemporalAAHistory>(RndCtx, Self->PassData, i);
			//		RndCtx.BindStatic(History);
			//
			//		auto Input = FDataSet::GetStaticResource<RDAG::FTemporalAAInput>(RndCtx, Self->PassData, i);
			//		RndCtx.BindStatic(Input);
			//
			//		auto Output = FDataSet::GetMutableResource<RDAG::FTemporalAAOutput>(RndCtx, Self->PassData, i);
			//		RndCtx.BindMutable(Output);
			//	}
			//}
			//RndCtx.SetRenderPass(nullptr);
		})(MergedTable);
	}
	else
	{
		return Builder.MoveAllInputToOutputTableEntries<RDAG::TemporalAAInput, RDAG::TemporalAAOutput>()(Input);
	}
}