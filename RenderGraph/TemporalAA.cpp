#include "TemporalAA.h"

namespace RDAG
{
	EXTERNAL_TEX_HANDLE(TemporalAAHistory);
}

typename TemporalAARenderPass::TemporalAARenderResult TemporalAARenderPass::Build(const RenderPassBuilder& Builder, const TemporalAARenderInput& Input, const SceneViewInfo& ViewInfo)
{
	if (ViewInfo.TemporalAaEnabled)
	{
		std::vector<ExternalTexture2dDescriptor> OutputDescriptors;
		std::vector<ExternalTexture2dDescriptor> HistoryDescriptors;
		for (U32 i = 0; i < Input.GetResourceCount<RDAG::TemporalAAInput>(); i++)
		{
			HistoryDescriptors.push_back(ExternalTexture2dDescriptor(Input.GetDescriptor<RDAG::TemporalAAInput>(i)));
			HistoryDescriptors[i].Index = i;
			HistoryDescriptors[i].Name = "TemporalAAHistory";
			OutputDescriptors.push_back(HistoryDescriptors[i]);
		}

		auto MergedTable = Seq
		{
			Builder.CreateResource<RDAG::TemporalAAOutput>(OutputDescriptors),
			Builder.CreateResource<RDAG::TemporalAAHistory>(HistoryDescriptors)
		}(Input);

		typedef decltype(MergedTable) TemporalAAAction;
		return Builder.QueueRenderAction("TemporalAAAction", [](RenderContext& Ctx, const TemporalAAAction&)
		{
			Ctx.Draw("TemporalAAAction");
		})(MergedTable);
	}
	else
	{
		return Builder.RenameAllEntries<RDAG::TemporalAAInput, RDAG::TemporalAAOutput>()(Input);
	}
}