#include "DepthOfField.h"
#include "VelocityPass.h"
#include "TemporalAA.h"

namespace RDAG
{
	struct FullresColorSetup : Texture2dResourceHandle<FullresColorSetup>
	{
		static constexpr const char* Name = "FullresColorSetup";

		explicit FullresColorSetup() {}
	};

	struct GatherColorSetup : Texture2dResourceHandle<GatherColorSetup>
	{
		static constexpr const char* Name = "GatherColorSetup";

		explicit GatherColorSetup() {}
		explicit GatherColorSetup(const TemporalAAOutput&) {};
	};

	struct CocTileOutput : Texture2dResourceHandle<CocTileOutput>
	{
		static constexpr const char* Name = "CocTileOutput";

		explicit CocTileOutput() {}
	};

	struct PrefilterOutput : Texture2dResourceHandle<PrefilterOutput>
	{
		static constexpr const char* Name = "PrefilterOutput";

		explicit PrefilterOutput() {}
	};


	struct ScatteringBokehLUTOutput : Texture2dResourceHandle<ScatteringBokehLUTOutput>
	{
		static constexpr const char* Name = "ScatteringBokehLUTOutput";

		explicit ScatteringBokehLUTOutput() {}
	};

	struct GatheringBokehLUTOutput : Texture2dResourceHandle<GatheringBokehLUTOutput>
	{
		static constexpr const char* Name = "GatheringBokehLUTOutput";

		explicit GatheringBokehLUTOutput() {}
	};

	struct ConvolutionOutput : Texture2dResourceHandle<ConvolutionOutput>
	{
		static constexpr const char* Name = "ConvolutionOutput";

		explicit ConvolutionOutput() {}
		explicit ConvolutionOutput(const struct ForegroundConvolutionOutput&) {}
		explicit ConvolutionOutput(const struct BackgroundConvolutionOutput&) {}
	};

	struct ForegroundConvolutionOutput : Texture2dResourceHandle<ForegroundConvolutionOutput>
	{
		static constexpr const char* Name = "ForegroundConvolutionOutput";

		explicit ForegroundConvolutionOutput() {}
		explicit ForegroundConvolutionOutput(const ConvolutionOutput&) {}
	};


	struct BackgroundConvolutionOutput : Texture2dResourceHandle<BackgroundConvolutionOutput>
	{
		static constexpr const U32 ResourceCount = 2;
		static constexpr const char* Name = "BackgroundConvolutionOutput";

		explicit BackgroundConvolutionOutput() {}
		explicit BackgroundConvolutionOutput(const ConvolutionOutput&) {}
	};

	struct SlightOutOfFocusConvolutionOutput : Texture2dResourceHandle<SlightOutOfFocusConvolutionOutput>
	{
		static constexpr const char* Name = "SlightOutOfFocusConvolutionOutput";

		explicit SlightOutOfFocusConvolutionOutput() {}
	};

	struct ScatteringReduce : Texture2dResourceHandle<ScatteringReduce>
	{
		static constexpr const char* Name = "ScatteringReduce";

		explicit ScatteringReduce() {}
	};

	struct ScatterCompilation : Texture2dResourceHandle<ScatterCompilation>
	{
		static constexpr const char* Name = "ScatterCompilation";

		explicit ScatterCompilation() {}
		explicit ScatterCompilation(const struct ScatteringReduce&) {}
	};
};

template<typename ConvolutionOutputType>
auto HybridScatteringLayerProcessing(const RenderPassBuilder& Builder, bool Enabled)
{
	return [&Builder, Enabled](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();
		Texture2d::Descriptor ScatteringReduceDesc;
		ScatteringReduceDesc.Name = "ScatteringReduce";
		ScatteringReduceDesc.Format = ERenderResourceFormat::ARGB16F;
		ScatteringReduceDesc.Height = ViewInfo.SceneHeight;
		ScatteringReduceDesc.Width = ViewInfo.SceneWidth;
		
		Texture2d::Descriptor ScatterCompilationDesc;
		ScatterCompilationDesc.Name = "ScatterCompilation";
		ScatterCompilationDesc.Format = ERenderResourceFormat::ARGB16F;
		ScatterCompilationDesc.Height = ViewInfo.SceneHeight;
		ScatterCompilationDesc.Width = ViewInfo.SceneWidth;
		
		using ScatteringReduceData = ResourceTable<RDAG::GatherColorSetup, RDAG::ScatteringReduce>;
		using ScatterCompilationData = ResourceTable<RDAG::ScatteringReduce, RDAG::ScatterCompilation>;
		using DOFHybridScatter = ResourceTable<RDAG::ScatterCompilation, RDAG::ScatteringBokehLUTOutput, ConvolutionOutputType>;

		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if(Enabled)
		{		
			Result = Seq
			{
				Builder.CreateResource<RDAG::ScatteringReduce>({ ScatteringReduceDesc }),
				Builder.QueueRenderAction("ScatteringReduceAction", [](RenderContext& Ctx, const ScatteringReduceData& Resources) -> ResourceTable<RDAG::ScatteringReduce>
				{
					Ctx.Draw("ScatteringReduceAction");
					return Resources;
				}),
				Builder.CreateResource<RDAG::ScatterCompilation>({ ScatterCompilationDesc }),
				Builder.QueueRenderAction("ScatterCompilationAction", [](RenderContext& Ctx, const ScatterCompilationData& Resources) -> ResourceTable<RDAG::ScatterCompilation>
				{
					Ctx.Draw("ScatterCompilationAction");
					return Resources;
				}),
				Builder.QueueRenderAction("DOFHybridScatterAction", [](RenderContext& Ctx, const DOFHybridScatter& Resources) -> ResourceTable<ConvolutionOutputType>
				{
					Ctx.Draw("DOFHybridScatterAction");
					return Resources;
				})
			}(Result);
		}
		return Result;
	};
}

template<typename BokehLUTType>
auto BuildBokehLut(const RenderPassBuilder& Builder)
{
	return [&Builder](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();
		typename BokehLUTType::DescriptorType LutOutputDesc[BokehLUTType::ResourceCount];
		for (U32 i = 0; i < BokehLUTType::ResourceCount; i++)
		{
			LutOutputDesc[i].Name = BokehLUTType::Name;
			LutOutputDesc[i].Format = ERenderResourceFormat::ARGB16F;
			LutOutputDesc[i].Height = ViewInfo.SceneHeight;
			LutOutputDesc[i].Width = ViewInfo.SceneWidth;
		}

		auto LutOutputTable = Builder.CreateResource<BokehLUTType>(LutOutputDesc)(s);
		using BuildBokehLUTData = ResourceTable<BokehLUTType>;

		if (!ViewInfo.DofSettings.BokehShapeIsCircle)
		{
			for (U32 i = 0; i < BokehLUTType::ResourceCount; i++)
			{
				LutOutputTable = Builder.QueueRenderAction("BuildBokehLUTAction", [](RenderContext& Ctx, const BuildBokehLUTData& Resources) -> ResourceTable<BokehLUTType>
				{
					Ctx.Draw("BuildBokehLUTAction");
					return Resources;
				})(LutOutputTable);
			}
		}
		return LutOutputTable;
	};
}

template<typename ConvolutionGatherType>
auto ConvolutionGatherPass(const RenderPassBuilder& Builder, bool Enabled = true)
{
	return [&Builder, Enabled](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();
		typename ConvolutionGatherType::DescriptorType ConvolutionOutputDesc[ConvolutionGatherType::ResourceCount];
		for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
		{
			ConvolutionOutputDesc[i].Name = ConvolutionGatherType::Name;
			ConvolutionOutputDesc[i].Format = ERenderResourceFormat::ARGB16F;
			ConvolutionOutputDesc[i].Height = ViewInfo.SceneHeight;
			ConvolutionOutputDesc[i].Width = ViewInfo.SceneWidth;
		}
		auto ConvolutionOutputTable = Builder.CreateResource<ConvolutionGatherType>(ConvolutionOutputDesc)(s);
		using GatherPassData = ResourceTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::GatheringBokehLUTOutput, RDAG::ConvolutionOutput>;

		if (Enabled)
		{
			for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
			{
				ConvolutionOutputTable = Seq
				{
					Builder.RenameEntry<ConvolutionGatherType, RDAG::ConvolutionOutput>(i, 0),
					Builder.QueueRenderAction("GatherPassDataAction", [](RenderContext& Ctx, const GatherPassData& Resources) -> ResourceTable<RDAG::ConvolutionOutput>
					{
						Ctx.Draw("GatherPassDataAction");
						return Resources;
					}),
					Builder.RenameEntry<RDAG::ConvolutionOutput, ConvolutionGatherType>(0, i)
				}(ConvolutionOutputTable);
			}
		}
		return ConvolutionOutputTable;
	};
}

template<typename... DofPostfilterElems>
auto DofPostfilterPass(const RenderPassBuilder& Builder, bool GatherForeGround)
{
	using DofPostfilterData = ResourceTable<DofPostfilterElems...>;

	return [&Builder, GatherForeGround](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();

		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if (ViewInfo.DofSettings.EnablePostfilterMethod && GatherForeGround)
		{
			Result = Builder.QueueRenderAction("DOFPostfilterAction", [](RenderContext& Ctx, const DofPostfilterData& Resources) -> ResourceTable<DofPostfilterElems...>
			{
				Ctx.Draw("DOFPostfilterAction");
				return Resources;
			})(Result);
		}
		return Result;
	};
}

auto SlightlyOutOfFocusPass(const RenderPassBuilder& Builder)
{
	return [&Builder](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();
		Texture2d::Descriptor SlightOutOfFocusConvolutionDesc;
		SlightOutOfFocusConvolutionDesc.Name = "SlightOutOfFocusConvolution";
		SlightOutOfFocusConvolutionDesc.Format = ERenderResourceFormat::ARGB16F;
		SlightOutOfFocusConvolutionDesc.Height = ViewInfo.SceneHeight;
		SlightOutOfFocusConvolutionDesc.Width = ViewInfo.SceneWidth;

		auto SlightlyOutOfFocusTable = Builder.CreateResource<RDAG::SlightOutOfFocusConvolutionOutput>({ SlightOutOfFocusConvolutionDesc })(s);
		if (ViewInfo.DofSettings.RecombineQuality > 0)
		{
			using GatherPassData = ResourceTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::ScatteringBokehLUTOutput, RDAG::SlightOutOfFocusConvolutionOutput>;
			SlightlyOutOfFocusTable = Builder.QueueRenderAction("SlightlyOutOfFocusAction", [](RenderContext& Ctx, const GatherPassData& Resources) -> ResourceTable<RDAG::SlightOutOfFocusConvolutionOutput>
			{
				Ctx.Draw("SlightlyOutOfFocusAction");
				return Resources;
			})(SlightlyOutOfFocusTable);
		}

		return SlightlyOutOfFocusTable;
	};
};

typename DepthOfFieldPass::PassOutputType DepthOfFieldPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	if (ViewInfo.DepthOfFieldEnabled)
	{
		Texture2d::Descriptor FullresColorDesc;
		FullresColorDesc.Name = "FullresColorSetup";
		FullresColorDesc.Format = ERenderResourceFormat::ARGB16F;
		FullresColorDesc.Height = ViewInfo.SceneHeight;
		FullresColorDesc.Width = ViewInfo.SceneWidth;

		Texture2d::Descriptor GatherColorDesc;
		GatherColorDesc.Name = "GatherColorSetup";
		GatherColorDesc.Format = ERenderResourceFormat::ARGB16F;
		GatherColorDesc.Height = ViewInfo.SceneHeight >> 1;
		GatherColorDesc.Width = ViewInfo.SceneWidth >> 1;
		using DofSetupPassData = ResourceTable<RDAG::DepthOfFieldInput, RDAG::VelocityVectors, RDAG::FullresColorSetup, RDAG::GatherColorSetup>;

		Texture2d::Descriptor CocTileDesc = GatherColorDesc;
		GatherColorDesc.Name = "CocTileOutput";
		using CocDilateData = ResourceTable<RDAG::GatherColorSetup, RDAG::CocTileOutput>;

		Texture2d::Descriptor PrefilterOutputDesc = GatherColorDesc;
		PrefilterOutputDesc.Name = "PrefilterOutputDesc";
		using PreFilterData = ResourceTable<RDAG::GatherColorSetup, RDAG::PrefilterOutput>;

		Texture2d::Descriptor OutputDesc = Input.GetDescriptor<RDAG::DepthOfFieldInput>();
		OutputDesc.Name = "DepthOfFieldOutput";
		using RecombineData = ResourceTable<RDAG::FullresColorSetup, RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput, RDAG::SlightOutOfFocusConvolutionOutput, RDAG::ScatteringBokehLUTOutput, RDAG::DepthOfFieldOutput>;

		return Seq
		{
			Builder.CreateResource<RDAG::FullresColorSetup>({ FullresColorDesc }),
			Builder.CreateResource<RDAG::GatherColorSetup>({ GatherColorDesc }),
			Builder.QueueRenderAction("DOFSetupAction", [](RenderContext& Ctx, const DofSetupPassData& Resources) -> ResourceTable<RDAG::FullresColorSetup, RDAG::GatherColorSetup>
			{
				Ctx.Draw("DOFSetupAction");
				return Resources;
			}),
			Select<RDAG::SceneViewInfo, RDAG::DepthTexture, RDAG::VelocityVectors, RDAG::GatherColorSetup>(
			Extract<RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput, RDAG::SlightOutOfFocusConvolutionOutput>(Seq
			{
				Scope(Seq
				{
					Builder.RenameEntry<RDAG::GatherColorSetup, RDAG::TemporalAAInput>(),
					Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
					Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::GatherColorSetup>()
				}),
				Builder.CreateResource<RDAG::CocTileOutput>({ CocTileDesc }),
				Builder.QueueRenderAction("CocDilateAction", [](RenderContext& Ctx, const CocDilateData& Resources) -> ResourceTable<RDAG::CocTileOutput>
				{
					Ctx.Draw("CocDilateAction");
					return Resources;
				}),
				Builder.CreateResource<RDAG::PrefilterOutput>({ PrefilterOutputDesc }),
				Builder.QueueRenderAction("PreFilterAction", [](RenderContext& Ctx, const PreFilterData& Resources) -> ResourceTable<RDAG::PrefilterOutput>
				{
					Ctx.Draw("PreFilterAction");
					return Resources;
				}),
				BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder),
				BuildBokehLut<RDAG::GatheringBokehLUTOutput>(Builder),
				ConvolutionGatherPass<RDAG::BackgroundConvolutionOutput>(Builder),
				ConvolutionGatherPass<RDAG::ForegroundConvolutionOutput>(Builder, ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::BackgroundConvolutionOutput>(Builder, !ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput>(Builder, ViewInfo.DofSettings.GatherForeground),
				HybridScatteringLayerProcessing<RDAG::ForegroundConvolutionOutput>(Builder, ViewInfo.DofSettings.EnabledForegroundLayer),
				HybridScatteringLayerProcessing<RDAG::BackgroundConvolutionOutput>(Builder, ViewInfo.DofSettings.EnabledBackgroundLayer),
				SlightlyOutOfFocusPass(Builder)
			})),
			BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder),
			Builder.CreateResource<RDAG::DepthOfFieldOutput>({ OutputDesc }),
			Builder.QueueRenderAction("RecombineAction", [](RenderContext& Ctx, const RecombineData& Resources) -> PassOutputType
			{
				Ctx.Draw("RecombineAction");
				return Resources;
			})
		}(Input);
	}
	else
	{
		return Builder.RenameEntry<RDAG::DepthOfFieldInput, RDAG::DepthOfFieldOutput>()(Input);
	}
}
