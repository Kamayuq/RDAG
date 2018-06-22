#include "TemporalAA.h"

namespace RDAG
{
	EXTERNAL_TEX_HANDLE(TemporalAAHistory);
}

typename TemporalAARenderPass::TemporalAARenderResult TemporalAARenderPass::Build(const RenderPassBuilder& Builder, const TemporalAARenderInput& Input, const SceneViewInfo& ViewInfo, const char* TaaKey)
{
	if (ViewInfo.TemporalAaEnabled)
	{
		Texture2d::Descriptor OutputDescriptor = Input.GetDescriptor<RDAG::TemporalAAInput>();
		OutputDescriptor.Name = TaaKey;
		Texture2d::Descriptor HistoryDescriptor = OutputDescriptor;

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