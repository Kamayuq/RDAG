#include "TemporalAA.h"

namespace RDAG
{
	EXTERNAL_TEX_HANDLE(TemporalAAHistory);
}

typename TemporalAARenderPass::TemporalAARenderResult TemporalAARenderPass::Build(const RenderPassBuilder& Builder, const TemporalAARenderInput& Input, const SceneViewInfo& ViewInfo)
{
	if (ViewInfo.TemporalAaEnabled)
	{
		ExternalTexture2dDescriptor OutputDescriptor = Input.GetDescriptor<RDAG::TemporalAAInput>();
		OutputDescriptor.Index = 1;
		OutputDescriptor.Name = "TemporalAAHistory";
		ExternalTexture2dDescriptor HistoryDescriptor = OutputDescriptor;

		auto MergedTable = Seq
		{
			Builder.CreateResource<RDAG::TemporalAAOutput>(OutputDescriptor),
			Builder.CreateResource<RDAG::TemporalAAHistory>(HistoryDescriptor)
		}(Input);

		typedef decltype(MergedTable) TemporalAAAction;
		return Builder.QueueRenderAction("TemporalAAAction", [](RenderContext& Ctx, const TemporalAAAction&)
		{
			Ctx.Draw("TemporalAAAction");
		})(MergedTable);
	}
	else
	{
		return Builder.AssignEntry<RDAG::TemporalAAInput, RDAG::TemporalAAOutput>()(Input);
	}
}