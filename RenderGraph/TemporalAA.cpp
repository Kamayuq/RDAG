#include "TemporalAA.h"


typename TemporalAARenderPass::PassOutputType TemporalAARenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	ExternalTexture2dDescriptor OutputDescriptors[RDAG::SceneViewInfo::TemporalAAResourceCount];
	ExternalTexture2dDescriptor HistoryDescriptors[RDAG::SceneViewInfo::TemporalAAResourceCount];
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
		{
			Builder.CreateOutputResource<RDAG::TemporalAAOutput>(OutputDescriptors),
			Builder.CreateInputResource<RDAG::TemporalAAHistory>(HistoryDescriptors)
		}(Input);

		typedef decltype(MergedTable) MergedTableType;

		return Builder.QueueRenderAction("TemporalAAAction", [](RenderContext& Ctx, const MergedTableType&)
		{
			Ctx.Draw("TemporalAAAction");
		})(MergedTable);
	}
	else
	{
		return Builder.MoveAllInputToOutputTableEntries<RDAG::TemporalAAInput, RDAG::TemporalAAOutput>()(Input);
	}
}