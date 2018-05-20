#include "DepthOfField.h"
#include "VelocityPass.h"
#include "TemporalAA.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(DepthOfFieldUav, SceneColorTexture);
	SIMPLE_TEX_HANDLE(FullresColorTexture);
	SIMPLE_UAV_HANDLE(FullresColorUav, FullresColorTexture);
	SIMPLE_TEX_HANDLE(GatherColorTexture);
	SIMPLE_UAV_HANDLE(GatherColorUav, GatherColorTexture);
	SIMPLE_TEX_HANDLE(CocTileTexture);
	SIMPLE_UAV_HANDLE(CocTileUav, CocTileTexture);
	SIMPLE_TEX_HANDLE(PrefilterTexture);
	SIMPLE_UAV_HANDLE(PrefilterUav, PrefilterTexture);
	SIMPLE_TEX_HANDLE(ScatteringBokehLUTTexture);
	SIMPLE_UAV_HANDLE(ScatteringBokehLUTUav, ScatteringBokehLUTTexture);
	SIMPLE_TEX_HANDLE(GatheringBokehLUTTexture);
	SIMPLE_UAV_HANDLE(GatheringBokehLUTUav, GatheringBokehLUTTexture);
	SIMPLE_TEX_HANDLE(ConvolutionTexture);
	SIMPLE_UAV_HANDLE(ConvolutionUav, ConvolutionTexture);
	SIMPLE_TEX_HANDLE(ForegroundConvolutionTexture);
	SIMPLE_UAV_HANDLE(ForegroundConvolutionUav, ForegroundConvolutionTexture);
	SIMPLE_TEX_HANDLE(SlightOutOfFocusConvolutionTexture);
	SIMPLE_UAV_HANDLE(SlightOutOfFocusConvolutionUav, SlightOutOfFocusConvolutionTexture);
	SIMPLE_TEX_HANDLE(ScatteringReduceTexture);
	SIMPLE_UAV_HANDLE(ScatteringReduceUav, ScatteringReduceTexture);
	SIMPLE_TEX_HANDLE(ScatterCompilationTexture);
	SIMPLE_UAV_HANDLE(ScatterCompilationUav, ScatterCompilationTexture);
	SIMPLE_TEX_HANDLE(BackgroundConvolutionTexture);
	SIMPLE_UAV_HANDLE(BackgroundConvolutionUav, BackgroundConvolutionTexture);
};

template<typename ConvolutionOutputType>
auto HybridScatteringLayerProcessing(const RenderPassBuilder& Builder, const SceneViewInfo& ViewInfo, bool Enabled)
{
	return Seq([&Builder, &ViewInfo, Enabled](const auto& s)
	{
		CheckIsValidResourceTable(s);

		Texture2d::Descriptor ScatteringReduceDesc;
		ScatteringReduceDesc.Name = "ScatteringReduceTexture";
		ScatteringReduceDesc.Format = ERenderResourceFormat::ARGB16F;
		ScatteringReduceDesc.Height = ViewInfo.SceneHeight;
		ScatteringReduceDesc.Width = ViewInfo.SceneWidth;
		
		Texture2d::Descriptor ScatterCompilationDesc;
		ScatterCompilationDesc.Name = "ScatterCompilationTexture";
		ScatterCompilationDesc.Format = ERenderResourceFormat::ARGB16F;
		ScatterCompilationDesc.Height = ViewInfo.SceneHeight;
		ScatterCompilationDesc.Width = ViewInfo.SceneWidth;
		
		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if(Enabled)
		{
			using ScatteringReduceData = ResourceTable<RDAG::ScatteringReduceUav, RDAG::GatherColorTexture>;
			using ScatterCompilationData = ResourceTable< RDAG::ScatterCompilationUav, RDAG::ScatteringReduceTexture>;
			using DOFHybridScatter = ResourceTable<ConvolutionOutputType, RDAG::ScatterCompilationTexture, RDAG::ScatteringBokehLUTTexture>;
			Result = Seq
			{
				Builder.CreateResource<RDAG::ScatteringReduceTexture>({ ScatteringReduceDesc }),
				Builder.QueueRenderAction("ScatteringReduceAction", [](RenderContext& Ctx, const ScatteringReduceData&)
				{
					Ctx.Draw("ScatteringReduceAction");
				}),
				Builder.CreateResource<RDAG::ScatterCompilationTexture>({ ScatterCompilationDesc }),
				Builder.QueueRenderAction("ScatterCompilationAction", [](RenderContext& Ctx, const ScatterCompilationData&)
				{
					Ctx.Draw("ScatterCompilationAction");
				}),
				Builder.QueueRenderAction("DOFHybridScatterAction", [](RenderContext& Ctx, const DOFHybridScatter&)
				{
					Ctx.Draw("DOFHybridScatterAction");
				})
			}(Result);
		}
		return Result;
	});
}

template<typename BokehLUTType>
auto BuildBokehLut(const RenderPassBuilder& Builder, const SceneViewInfo& ViewInfo)
{
	return Seq([&Builder, &ViewInfo](const auto& s)
	{
		CheckIsValidResourceTable(s);

		typename BokehLUTType::DescriptorType LutOutputDesc;
		LutOutputDesc.Name = BokehLUTType::Name;
		LutOutputDesc.Format = ERenderResourceFormat::ARGB16F;
		LutOutputDesc.Height = ViewInfo.SceneHeight;
		LutOutputDesc.Width = ViewInfo.SceneWidth;

		auto LutOutputTable = Builder.CreateResource<BokehLUTType>({ LutOutputDesc })(s);

		if (!ViewInfo.DofSettings.BokehShapeIsCircle)
		{
			using BuildBokehLUTData = ResourceTable<BokehLUTType>;
			LutOutputTable = Builder.QueueRenderAction("BuildBokehLUTAction", [](RenderContext& Ctx, const BuildBokehLUTData&)
			{
				Ctx.Draw("BuildBokehLUTAction");
			})(LutOutputTable);
		}
		return LutOutputTable;
	});
}

template<typename ConvolutionGatherType, unsigned ResourceCount>
auto ConvolutionGatherPass(const RenderPassBuilder& Builder, const SceneViewInfo& ViewInfo, bool Enabled = true)
{
	return Seq([&Builder, &ViewInfo, Enabled](const auto& s)
	{
		CheckIsValidResourceTable(s);

		typename ConvolutionGatherType::DescriptorType ConvolutionOutputDesc[ResourceCount];
		for (U32 i = 0; i < ResourceCount; i++)
		{
			ConvolutionOutputDesc[i].Name = ConvolutionGatherType::Name;
			ConvolutionOutputDesc[i].Format = ERenderResourceFormat::ARGB16F;
			ConvolutionOutputDesc[i].Height = ViewInfo.SceneHeight;
			ConvolutionOutputDesc[i].Width = ViewInfo.SceneWidth;
		}
		auto ConvolutionOutputTable = Builder.CreateResource<ConvolutionGatherType>(ConvolutionOutputDesc)(s);

		if (Enabled)
		{
			for (U32 i = 0; i < ResourceCount; i++)
			{
				using GatherPassData = ResourceTable<RDAG::ConvolutionUav, RDAG::GatheringBokehLUTTexture, RDAG::PrefilterTexture, RDAG::CocTileTexture>;
				ConvolutionOutputTable = Seq
				{
					Builder.RenameEntry<ConvolutionGatherType, RDAG::ConvolutionUav>(i, 0),
					Builder.QueueRenderAction("GatherPassDataAction", [](RenderContext& Ctx, const GatherPassData&)
					{
						Ctx.Draw("GatherPassDataAction");
					}),
					Builder.RenameEntry<RDAG::ConvolutionUav, ConvolutionGatherType>(0, i)
				}(ConvolutionOutputTable);
			}
		}
		return ConvolutionOutputTable;
	});
}

template<typename... DofPostfilterElems>
auto DofPostfilterPass(const RenderPassBuilder& Builder, const SceneViewInfo& ViewInfo, bool GatherForeGround)
{
	return Seq([&Builder, &ViewInfo, GatherForeGround](const auto& s)
	{
		CheckIsValidResourceTable(s);

		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if (ViewInfo.DofSettings.EnablePostfilterMethod && GatherForeGround)
		{
			using DofPostfilterData = ResourceTable<DofPostfilterElems...>;
			Result = Builder.QueueRenderAction("DOFPostfilterAction", [](RenderContext& Ctx, const DofPostfilterData&)
			{
				Ctx.Draw("DOFPostfilterAction");
			})(Result);
		}
		return Result;
	});
}

auto SlightlyOutOfFocusPass(const RenderPassBuilder& Builder, const SceneViewInfo& ViewInfo)
{
	return Seq([&Builder, &ViewInfo](const auto& s)
	{
		CheckIsValidResourceTable(s);

		Texture2d::Descriptor SlightOutOfFocusConvolutionDesc;
		SlightOutOfFocusConvolutionDesc.Name = "SlightOutOfFocusConvolution";
		SlightOutOfFocusConvolutionDesc.Format = ERenderResourceFormat::ARGB16F;
		SlightOutOfFocusConvolutionDesc.Height = ViewInfo.SceneHeight;
		SlightOutOfFocusConvolutionDesc.Width = ViewInfo.SceneWidth;

		auto SlightlyOutOfFocusTable = Builder.CreateResource<RDAG::SlightOutOfFocusConvolutionUav>({ SlightOutOfFocusConvolutionDesc })(s);
		if (ViewInfo.DofSettings.RecombineQuality > 0)
		{
			using GatherPassData = ResourceTable<RDAG::SlightOutOfFocusConvolutionUav, RDAG::ScatteringBokehLUTTexture, RDAG::PrefilterTexture, RDAG::CocTileTexture>;
			SlightlyOutOfFocusTable = Builder.QueueRenderAction("SlightlyOutOfFocusAction", [](RenderContext& Ctx, const GatherPassData&)
			{
				Ctx.Draw("SlightlyOutOfFocusAction");
			})(SlightlyOutOfFocusTable);
		}

		return SlightlyOutOfFocusTable;
	});
};

typename DepthOfFieldPass::PassOutputType DepthOfFieldPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
{
	PassOutputType Output = Input;

	if (ViewInfo.DepthOfFieldEnabled)
	{
		Texture2d::Descriptor DofResultDesc = Input.GetDescriptor<RDAG::SceneColorTexture>();
		DofResultDesc.Name = "DofResultTexture";

		Texture2d::Descriptor FullresColorDesc;
		FullresColorDesc.Name = "FullresColorTexture";
		FullresColorDesc.Format = ERenderResourceFormat::ARGB16F;
		FullresColorDesc.Height = ViewInfo.SceneHeight;
		FullresColorDesc.Width = ViewInfo.SceneWidth;

		Texture2d::Descriptor GatherColorDesc;
		GatherColorDesc.Name = "GatherColorTexture";
		GatherColorDesc.Format = ERenderResourceFormat::ARGB16F;
		GatherColorDesc.Height = ViewInfo.SceneHeight >> 1;
		GatherColorDesc.Width = ViewInfo.SceneWidth >> 1;
		using DofSetupPassData = ResourceTable<RDAG::FullresColorUav, RDAG::GatherColorUav, RDAG::SceneColorTexture, RDAG::VelocityVectors>;

		Texture2d::Descriptor CocTileDesc = GatherColorDesc;
		GatherColorDesc.Name = "CocTileTexture";
		using CocDilateData = ResourceTable< RDAG::CocTileUav, RDAG::GatherColorTexture>;

		Texture2d::Descriptor PrefilterOutputDesc = GatherColorDesc;
		PrefilterOutputDesc.Name = "PrefilterOutputDesc";
		using PreFilterData = ResourceTable<RDAG::PrefilterUav, RDAG::GatherColorTexture>;
		using RecombineData = ResourceTable<RDAG::DepthOfFieldUav, RDAG::ScatteringBokehLUTTexture, RDAG::SlightOutOfFocusConvolutionTexture, RDAG::ForegroundConvolutionTexture, RDAG::BackgroundConvolutionTexture, RDAG::FullresColorTexture>;

		Output = Seq
		{
			Builder.CreateResource<RDAG::GatherColorUav>({ GatherColorDesc }),
			Builder.CreateResource<RDAG::FullresColorUav>({ FullresColorDesc }),
			Builder.QueueRenderAction("DOFSetupAction", [](RenderContext& Ctx, const DofSetupPassData&)
			{
				Ctx.Draw("DOFSetupAction");
			}),
			Scope(Seq
			{
				Builder.RenameEntry<RDAG::GatherColorTexture, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build, ViewInfo),
				Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::GatherColorTexture>()
			}),
			Select<RDAG::GatherColorTexture>(
			Extract<RDAG::SlightOutOfFocusConvolutionTexture, RDAG::ForegroundConvolutionTexture, RDAG::BackgroundConvolutionTexture>(Seq
			{
				Builder.CreateResource<RDAG::CocTileUav>({ CocTileDesc }),
				Builder.QueueRenderAction("CocDilateAction", [](RenderContext& Ctx, const CocDilateData&)
				{
					Ctx.Draw("CocDilateAction");
				}),
				Builder.CreateResource<RDAG::PrefilterUav>({ PrefilterOutputDesc }),
				Builder.QueueRenderAction("PreFilterAction", [](RenderContext& Ctx, const PreFilterData&)
				{
					Ctx.Draw("PreFilterAction");
				}),
				BuildBokehLut<RDAG::ScatteringBokehLUTUav>(Builder, ViewInfo),
				BuildBokehLut<RDAG::GatheringBokehLUTUav>(Builder, ViewInfo),
				ConvolutionGatherPass<RDAG::BackgroundConvolutionTexture, 2>(Builder, ViewInfo),
				ConvolutionGatherPass<RDAG::ForegroundConvolutionTexture, 1>(Builder, ViewInfo, ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::BackgroundConvolutionUav>(Builder, ViewInfo, !ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::ForegroundConvolutionUav, RDAG::BackgroundConvolutionTexture>(Builder, ViewInfo, ViewInfo.DofSettings.GatherForeground),
				HybridScatteringLayerProcessing<RDAG::BackgroundConvolutionUav>(Builder, ViewInfo, ViewInfo.DofSettings.EnabledBackgroundLayer),
				HybridScatteringLayerProcessing<RDAG::ForegroundConvolutionUav>(Builder, ViewInfo, ViewInfo.DofSettings.EnabledForegroundLayer),
				SlightlyOutOfFocusPass(Builder, ViewInfo)
			})),
			BuildBokehLut<RDAG::ScatteringBokehLUTUav>(Builder, ViewInfo),
			Builder.CreateResource<RDAG::DepthOfFieldUav>({ DofResultDesc }), //recreation is equal to fully overwrite re-setting transition state
			Builder.QueueRenderAction("RecombineAction", [](RenderContext& Ctx, const RecombineData&)
			{
				Ctx.Draw("RecombineAction");
			})
		}(Input);
	}

	return Output;
}
