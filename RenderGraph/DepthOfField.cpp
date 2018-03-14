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
}

namespace RDAG
{
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
auto HybridScatteringLayerProcessing(const RenderPassBuilder& Builder, Texture2d::Descriptor& GatherColorDesc)
{
	Texture2d::Descriptor ScatteringReduceDesc = GatherColorDesc;
	ScatteringReduceDesc.Name = "ScatteringReduce";

	Texture2d::Descriptor ScatterCompilationDesc = GatherColorDesc;
	ScatterCompilationDesc.Name = "ScatterCompilation";

	using ScatteringReduceData = ResourceTable<InputTable<RDAG::GatherColorSetup>, OutputTable<RDAG::ScatteringReduce>>;
	using ScatterCompilationData = ResourceTable<InputTable<RDAG::ScatteringReduce>, OutputTable<RDAG::ScatterCompilation>>;
	using DOFHybridScatter = ResourceTable<InputTable<RDAG::ScatterCompilation, RDAG::ScatteringBokehLUTOutput, ConvolutionOutputType>, OutputTable<ConvolutionOutputType>>;
	return Seq
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
	);
}

template<typename BokehLUTType>
auto BuildBokehLut(const RenderPassBuilder& Builder)
{
	using BuildBokehLUTData = ResourceTable<InputTable<BokehLUTType>, OutputTable<BokehLUTType>>;
	return Builder.QueueRenderAction("BuildBokehLUTAction", [](RenderContext& Ctx, const BuildBokehLUTData&)
	{
		Ctx.Draw("BuildBokehLUTAction");
	});
}

template<typename ConvolutionGatherType>
auto ConvolutionGatherPass(const RenderPassBuilder& Builder, U32 Index = 0)
{
	using GatherPassData = ResourceTable<InputTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::GatheringBokehLUTOutput, RDAG::ConvolutionOutput>, OutputTable<RDAG::ConvolutionOutput>>;
	return Seq
	(
		Builder.MoveOutputTableEntry<ConvolutionGatherType, RDAG::ConvolutionOutput>(Index, 0),
		Builder.QueueRenderAction("GatherPassDataAction", [](RenderContext& Ctx, const GatherPassData&)
		{
			Ctx.Draw("GatherPassDataAction");
		}),
		Builder.MoveOutputTableEntry<RDAG::ConvolutionOutput, ConvolutionGatherType>(0, Index)
	);
}

template<typename... DofPostfilterElems>
auto DofPostfilterPass(const RenderPassBuilder& Builder)
{
	using DofPostfilterData = ResourceTable<InputTable<DofPostfilterElems...>, OutputTable<DofPostfilterElems...>>;
	return Builder.QueueRenderAction("DOFPostfilterAction", [](RenderContext& Ctx, const DofPostfilterData&)
	{
		Ctx.Draw("DOFPostfilterAction");
	});
}


typename DepthOfFieldPass::PassOutputType DepthOfFieldPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor OutputDesc = Input.GetInputDescriptor<RDAG::DepthOfFieldInput>();
	OutputDesc.Name = "DepthOfFieldOutput";

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

	Texture2d::Descriptor CocTileDesc = GatherColorDesc;
	GatherColorDesc.Name = "CocTileOutput";

	Texture2d::Descriptor ForegroundScatterCompilationDesc = GatherColorDesc;
	ForegroundScatterCompilationDesc.Name = "ForegroundScatterCompilation";

	Texture2d::Descriptor BackgroundScatterCompilationDesc = GatherColorDesc;
	BackgroundScatterCompilationDesc.Name = "BackgroundScatterCompilation";

	Texture2d::Descriptor PrefilterOutputDesc = GatherColorDesc;
	PrefilterOutputDesc.Name = "PrefilterOutputDesc";

	Texture2d::Descriptor ScatteringBokehLUTOutputDesc = GatherColorDesc;
	ScatteringBokehLUTOutputDesc.Name = "ScatteringBokehLUTOutput";

	Texture2d::Descriptor GatheringBokehLUTOutputDesc = GatherColorDesc;
	GatheringBokehLUTOutputDesc.Name = "GatheringBokehLUTOutputDesc";

	Texture2d::Descriptor ForegroundConvolutionOutputDesc = GatherColorDesc;
	ForegroundConvolutionOutputDesc.Name = "ForegroundConvolutionOutput";
	
	Texture2d::Descriptor BackgroundConvolutionOutputDesc[2];
	BackgroundConvolutionOutputDesc[0] = GatherColorDesc;
	BackgroundConvolutionOutputDesc[0].Name = "BackgroundConvolutionOutput0";
	BackgroundConvolutionOutputDesc[1] = GatherColorDesc;
	BackgroundConvolutionOutputDesc[1].Name = "BackgroundConvolutionOutput1";

	Texture2d::Descriptor SlightOutOfFocusConvolutionDesc = GatherColorDesc;
	SlightOutOfFocusConvolutionDesc.Name = "SlightOutOfFocusConvolution";
	

	using DofSetupPassData = ResourceTable<InputTable<RDAG::DepthOfFieldInput, RDAG::VelocityVectors>, OutputTable<RDAG::FullresColorSetup, RDAG::GatherColorSetup>>;
	auto Output = Seq
	(
		Builder.CreateOutputResource<RDAG::DepthOfFieldOutput>({ OutputDesc }),
		Builder.CreateOutputResource<RDAG::FullresColorSetup>({ FullresColorDesc }),
		Builder.CreateOutputResource<RDAG::GatherColorSetup>({ GatherColorDesc }),
		Builder.CreateOutputResource<RDAG::CocTileOutput>({ CocTileDesc }),
		Builder.CreateOutputResource<RDAG::PrefilterOutput>({ PrefilterOutputDesc }),
		Builder.CreateOutputResource<RDAG::ScatteringBokehLUTOutput>({ ScatteringBokehLUTOutputDesc }),
		Builder.CreateOutputResource<RDAG::GatheringBokehLUTOutput>({ GatheringBokehLUTOutputDesc }),
		Builder.CreateOutputResource<RDAG::ForegroundConvolutionOutput>({ ForegroundConvolutionOutputDesc }),
		Builder.CreateOutputResource<RDAG::BackgroundConvolutionOutput>(BackgroundConvolutionOutputDesc),
		Builder.CreateOutputResource<RDAG::SlightOutOfFocusConvolutionOutput>({ SlightOutOfFocusConvolutionDesc }),
		Builder.QueueRenderAction("DOFSetupAction", [](RenderContext& Ctx, const DofSetupPassData&)
		{
			Ctx.Draw("DOFSetupAction");
		})
	)(Input);

	if (!ViewInfo.DofSettings.BokehShapeIsCircle)
	{
		Output = Seq
		(
			BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder),
			BuildBokehLut<RDAG::GatheringBokehLUTOutput>(Builder)
		)(Output);
	}

	if (ViewInfo.TemporalAaEnabled)
	{
		Output = Seq
		(
			Builder.MoveOutputToInputTableEntry<RDAG::GatherColorSetup, RDAG::TemporalAAInput>(),
			Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
			Builder.MoveOutputTableEntry<RDAG::TemporalAAOutput, RDAG::GatherColorSetup>()
		)(Output);
	}

	{
		using CocDilateData = ResourceTable<InputTable<RDAG::GatherColorSetup>, OutputTable<RDAG::CocTileOutput>>;
		Output = Builder.QueueRenderAction("CocDilateAction", [](RenderContext& Ctx, const CocDilateData&)
		{
			Ctx.Draw("CocDilateAction");
		})(Output);
	}

	{
		using PreFilterData = ResourceTable<InputTable<RDAG::GatherColorSetup>, OutputTable<RDAG::PrefilterOutput>>;
		Output = Builder.QueueRenderAction("PreFilterAction", [](RenderContext& Ctx, const PreFilterData&)
		{
			Ctx.Draw("PreFilterAction");
		})(Output);
	}

	if (ViewInfo.DofSettings.GatherForeground)
	{
		Output = ConvolutionGatherPass<RDAG::ForegroundConvolutionOutput>(Builder)(Output);
	}

	for (U32 i = 0; i < RDAG::BackgroundConvolutionOutput::ResourceCount; i++)
	{
		Output = ConvolutionGatherPass<RDAG::BackgroundConvolutionOutput>(Builder, i)(Output);
	}

	if (ViewInfo.DofSettings.RecombineQuality > 0)
	{
		using GatherPassData = ResourceTable<InputTable<RDAG::PrefilterOutput, RDAG::CocTileOutput, RDAG::ScatteringBokehLUTOutput>, OutputTable<RDAG::SlightOutOfFocusConvolutionOutput>>;
		Output = Builder.QueueRenderAction("GatherPassAction", [](RenderContext& Ctx, const GatherPassData&)
		{
			Ctx.Draw("GatherPassAction");
		})(Output);
	}

	if (ViewInfo.DofSettings.EnablePostfilterMethod)
	{
		if (ViewInfo.DofSettings.GatherForeground)
		{
			Output = DofPostfilterPass<RDAG::BackgroundConvolutionOutput>(Builder)(Output);
		}
		else
		{
			Output = DofPostfilterPass<RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput>(Builder)(Output);
		}
	}

	if (ViewInfo.DofSettings.EnabledForegroundLayer)
	{
		Output = HybridScatteringLayerProcessing< RDAG::ForegroundConvolutionOutput>(Builder, GatherColorDesc)(Output);
	}

	if (ViewInfo.DofSettings.EnabledBackgroundLayer)
	{
		Output = HybridScatteringLayerProcessing< RDAG::BackgroundConvolutionOutput>(Builder, GatherColorDesc)(Output);
	}

	{
		using RecombineData = ResourceTable<InputTable<RDAG::FullresColorSetup, RDAG::ForegroundConvolutionOutput, RDAG::BackgroundConvolutionOutput, RDAG::SlightOutOfFocusConvolutionOutput, RDAG::ScatteringBokehLUTOutput>, OutputTable<RDAG::DepthOfFieldOutput>>;
		Output = Seq
		(
			BuildBokehLut<RDAG::ScatteringBokehLUTOutput>(Builder),
			Builder.QueueRenderAction("RecombineAction", [](RenderContext& Ctx, const RecombineData&)
			{
				Ctx.Draw("RecombineAction");
			})
		)(Output);
	}

	return Output;
}