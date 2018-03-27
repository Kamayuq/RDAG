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
auto HybridScatteringLayerProcessing(const RenderPassBuilder& Builder, Texture2d::Descriptor& GatherColorDesc, bool Enabled)
{
	return [&Builder, GatherColorDesc, Enabled](const auto& s)
	{
		Texture2d::Descriptor ScatteringReduceDesc = GatherColorDesc;
		ScatteringReduceDesc.Name = "ScatteringReduce";

		Texture2d::Descriptor ScatterCompilationDesc = GatherColorDesc;
		ScatterCompilationDesc.Name = "ScatterCompilation";

		using ScatteringReduceData = ResourceTable
		<
			InputTable<RDAG::GatherColorSetup>,
			OutputTable<RDAG::ScatteringReduce>
		>;
		using ScatterCompilationData = ResourceTable
		<
			InputTable<RDAG::ScatteringReduce>,
			OutputTable<RDAG::ScatterCompilation>
		>;
		using DOFHybridScatter = ResourceTable
		<
			InputTable<RDAG::ScatterCompilation, RDAG::ScatteringBokehLUTOutput, ConvolutionOutputType>,
			OutputTable<ConvolutionOutputType>
		>;

		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if(Enabled)
		{		
			Result = Seq
			(
				Builder.CreateOutputResource<RDAG::ScatteringReduce>({ ScatteringReduceDesc }),
				Builder.QueueRenderAction("ScatteringReduceAction", [](RenderContext& Ctx, const ScatteringReduceData&)
				{
					Ctx.Draw("ScatteringReduceAction");
				}),
				Builder.CreateOutputResource<RDAG::ScatterCompilation>({ ScatterCompilationDesc }),
				Builder.QueueRenderAction("ScatterCompilationAction", [](RenderContext& Ctx, const ScatterCompilationData&)
				{
					Ctx.Draw("ScatterCompilationAction");
				}),
				Builder.QueueRenderAction("DOFHybridScatterAction", [](RenderContext& Ctx, const DOFHybridScatter&)
				{
					Ctx.Draw("DOFHybridScatterAction");
				})
			)(Result);
		}
		return Result;
	};
}

template<typename BokehLUTType>
auto BuildBokehLut(const RenderPassBuilder& Builder, const typename BokehLUTType::DescriptorType& Descriptor, bool Enabled = true)
{
	return [&Builder, &Descriptor, Enabled](const auto& s)
	{
		typename BokehLUTType::DescriptorType LutOutputDesc[BokehLUTType::ResourceCount];
		for (U32 i = 0; i < BokehLUTType::ResourceCount; i++)
		{
			LutOutputDesc[i] = Descriptor;
			LutOutputDesc[i].Name = BokehLUTType::Name;
		}

		auto LutOutputTable = Builder.CreateOutputResource<BokehLUTType>(LutOutputDesc)(s);
		using BuildBokehLUTData = ResourceTable
		<
			InputTable<>, 
			OutputTable<BokehLUTType>
		>;

		if (Enabled)
		{
			for (U32 i = 0; i < BokehLUTType::ResourceCount; i++)
			{
				LutOutputTable = Seq
				(
					Builder.QueueRenderAction("BuildBokehLUTAction", [](RenderContext& Ctx, const BuildBokehLUTData&)
					{
						Ctx.Draw("BuildBokehLUTAction");
					})
				)(LutOutputTable);
			}
		}
		return LutOutputTable;
	};
}

template<typename ConvolutionGatherType>
auto ConvolutionGatherPass(const RenderPassBuilder& Builder, const typename ConvolutionGatherType::DescriptorType& Descriptor, bool Enabled = true)
{
	return [&Builder, &Descriptor, Enabled](const auto& s)
	{
		typename ConvolutionGatherType::DescriptorType ConvolutionOutputDesc[ConvolutionGatherType::ResourceCount];
		for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
		{
			ConvolutionOutputDesc[i] = Descriptor;
			ConvolutionOutputDesc[i].Name = ConvolutionGatherType::Name;
		}
		auto ConvolutionOutputTable = Builder.CreateOutputResource<ConvolutionGatherType>(ConvolutionOutputDesc)(s);
		using GatherPassData = ResourceTable
		<
			InputTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::GatheringBokehLUTOutput>, 
			OutputTable<RDAG::ConvolutionOutput>
		>;

		if (Enabled)
		{
			for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
			{
				ConvolutionOutputTable = Seq
				(
					Builder.MoveOutputTableEntry<ConvolutionGatherType, RDAG::ConvolutionOutput>(i, 0),
					Builder.QueueRenderAction("GatherPassDataAction", [](RenderContext& Ctx, const GatherPassData&)
					{
						Ctx.Draw("GatherPassDataAction");
					}),
					Builder.MoveOutputTableEntry<RDAG::ConvolutionOutput, ConvolutionGatherType>(0, i)
				)(ConvolutionOutputTable);
			}
		}
		return ConvolutionOutputTable;
	};
}

template<typename... DofPostfilterElems>
auto DofPostfilterPass(const RenderPassBuilder& Builder, bool Enabled)
{
	using DofPostfilterData = ResourceTable
	<
		InputTable<DofPostfilterElems...>, 
		OutputTable<DofPostfilterElems...>
	>;

	return [&Builder, Enabled](const auto& s)
	{
		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if (Enabled)
		{
			Result = Builder.QueueRenderAction("DOFPostfilterAction", [](RenderContext& Ctx, const DofPostfilterData&)
			{
				Ctx.Draw("DOFPostfilterAction");
			})(Result);
		}
		return Result;
	};
}

auto SlightlyOutOfFocusPass(const RenderPassBuilder& Builder, const typename Texture2d::Descriptor& Descriptor, bool Enabled)
{
	return [&Builder, &Descriptor, Enabled](const auto& s)
	{
		Texture2d::Descriptor SlightOutOfFocusConvolutionDesc = Descriptor;
		SlightOutOfFocusConvolutionDesc.Name = "SlightOutOfFocusConvolution";

		auto SlightlyOutOfFocusTable = Builder.CreateOutputResource<RDAG::SlightOutOfFocusConvolutionOutput>({ SlightOutOfFocusConvolutionDesc })(s);
		if (Enabled)
		{
			using GatherPassData = ResourceTable<InputTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::ScatteringBokehLUTOutput>, OutputTable<RDAG::SlightOutOfFocusConvolutionOutput>>;
			SlightlyOutOfFocusTable = Builder.QueueRenderAction("SlightlyOutOfFocusAction", [](RenderContext& Ctx, const GatherPassData&)
			{
				Ctx.Draw("SlightlyOutOfFocusAction");
			})(SlightlyOutOfFocusTable);
		}

		return SlightlyOutOfFocusTable;
	};
};

typename DepthOfFieldPass::PassOutputType DepthOfFieldPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor FullresColorDesc;
	FullresColorDesc.Name = "FullresColorSetup";
	FullresColorDesc.Format = ERenderResourceFormat::RG16F;
	FullresColorDesc.Height = ViewInfo.SceneHeight;
	FullresColorDesc.Width = ViewInfo.SceneWidth;

	Texture2d::Descriptor GatherColorDesc;
	GatherColorDesc.Name = "GatherColorSetup";
	GatherColorDesc.Format = ERenderResourceFormat::RG16F;
	GatherColorDesc.Height = ViewInfo.SceneHeight >> 1;
	GatherColorDesc.Width = ViewInfo.SceneWidth >> 1;
	using DofSetupPassData = ResourceTable
	<
		InputTable<RDAG::DepthOfFieldInput, RDAG::VelocityVectors>, 
		OutputTable<RDAG::FullresColorSetup, RDAG::GatherColorSetup>
	>;

	Texture2d::Descriptor CocTileDesc = GatherColorDesc;
	GatherColorDesc.Name = "CocTileOutput";
	using CocDilateData = ResourceTable
	<
		InputTable<RDAG::GatherColorSetup>, 
		OutputTable<RDAG::CocTileOutput>
	>;

	Texture2d::Descriptor PrefilterOutputDesc = GatherColorDesc;
	PrefilterOutputDesc.Name = "PrefilterOutputDesc";
	using PreFilterData = ResourceTable
	<
		InputTable<RDAG::GatherColorSetup>, 
		OutputTable<RDAG::PrefilterOutput>
	>;

	Texture2d::Descriptor OutputDesc = Input.GetInputDescriptor<RDAG::DepthOfFieldInput>();
	OutputDesc.Name = "DepthOfFieldOutput";
	using RecombineData = ResourceTable
	<
		InputTable<RDAG::FullresColorSetup, RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput, RDAG::SlightOutOfFocusConvolutionOutput, RDAG::ScatteringBokehLUTOutput>, 
		OutputTable<RDAG::DepthOfFieldOutput>
	>;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::FullresColorSetup>({ FullresColorDesc }),
		Builder.CreateOutputResource<RDAG::GatherColorSetup>({ GatherColorDesc }),
		Builder.QueueRenderAction("DOFSetupAction", [](RenderContext& Ctx, const DofSetupPassData&)
		{
			Ctx.Draw("DOFSetupAction");
		}),
		Builder.MoveOutputToInputTableEntry<RDAG::GatherColorSetup, RDAG::TemporalAAInput>(),
		Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
		Builder.MoveOutputTableEntry<RDAG::TemporalAAOutput, RDAG::GatherColorSetup>(),
		Builder.CreateOutputResource<RDAG::CocTileOutput>({ CocTileDesc }),
		Builder.QueueRenderAction("CocDilateAction", [](RenderContext& Ctx, const CocDilateData&)
		{
			Ctx.Draw("CocDilateAction");
		}),
		Builder.CreateOutputResource<RDAG::PrefilterOutput>({ PrefilterOutputDesc }),
		Builder.QueueRenderAction("PreFilterAction", [](RenderContext& Ctx, const PreFilterData&)
		{
			Ctx.Draw("PreFilterAction");
		}),
		BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder, GatherColorDesc, !ViewInfo.DofSettings.BokehShapeIsCircle),
		BuildBokehLut<RDAG::GatheringBokehLUTOutput>(Builder, GatherColorDesc, !ViewInfo.DofSettings.BokehShapeIsCircle),
		ConvolutionGatherPass<RDAG::BackgroundConvolutionOutput>(Builder, GatherColorDesc),
		ConvolutionGatherPass<RDAG::ForegroundConvolutionOutput>(Builder, GatherColorDesc, ViewInfo.DofSettings.GatherForeground),
		DofPostfilterPass<RDAG::BackgroundConvolutionOutput>(Builder, ViewInfo.DofSettings.EnablePostfilterMethod && !ViewInfo.DofSettings.GatherForeground),
		DofPostfilterPass<RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput>(Builder, ViewInfo.DofSettings.EnablePostfilterMethod && ViewInfo.DofSettings.GatherForeground),
		HybridScatteringLayerProcessing<RDAG::ForegroundConvolutionOutput>(Builder, GatherColorDesc, ViewInfo.DofSettings.EnabledForegroundLayer),
		HybridScatteringLayerProcessing<RDAG::BackgroundConvolutionOutput>(Builder, GatherColorDesc, ViewInfo.DofSettings.EnabledBackgroundLayer),
		SlightlyOutOfFocusPass(Builder, GatherColorDesc, ViewInfo.DofSettings.RecombineQuality > 0),
		BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder, GatherColorDesc),
		Builder.CreateOutputResource<RDAG::DepthOfFieldOutput>({ OutputDesc }),
		Builder.QueueRenderAction("RecombineAction", [](RenderContext& Ctx, const RecombineData&)
		{
			Ctx.Draw("RecombineAction");
		})
	)(Input);
}