#include "DepthOfField.h"
#include "VelocityPass.h"
#include "TemporalAA.h"

#define SIMPLE_TEX_HANDLE_ARRAY(HandleName, Num)					\
struct HandleName : Texture2dResourceHandle<HandleName>				\
{																	\
	static constexpr const U32 ResourceCount = Num;					\
	static constexpr const char* Name = #HandleName;				\
	explicit HandleName() {}										\
	template<typename CRTP>											\
	explicit HandleName(const Texture2dResourceHandle<CRTP>&) {}	\
};														
#define SIMPLE_TEX_HANDLE(HandleName) SIMPLE_TEX_HANDLE_ARRAY(HandleName, 1)

#define SIMPLE_UAV_HANDLE_ARRAY(HandleName, Compatible, Num)		\
struct HandleName : Uav2dResourceHandle<Compatible>					\
{																	\
	static constexpr const U32 ResourceCount = Num;					\
	static constexpr const char* Name = #HandleName;				\
	explicit HandleName() {}										\
	template<typename CRTP>											\
	explicit HandleName(const Texture2dResourceHandle<CRTP>&) {}	\
};	
#define SIMPLE_UAV_HANDLE(HandleName, Compatible) SIMPLE_UAV_HANDLE_ARRAY(HandleName, Compatible, 1)

namespace RDAG
{
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
	SIMPLE_TEX_HANDLE_ARRAY(BackgroundConvolutionTexture, 2);
	SIMPLE_UAV_HANDLE_ARRAY(BackgroundConvolutionUav, BackgroundConvolutionTexture, 2);
};
#undef SIMPLE_TEX_HANDLE
#undef SIMPLE_TEX_HANDLE_ARRAY
#undef SIMPLE_UAV_HANDLE
#undef SIMPLE_UAV_HANDLE_ARRAY

template<typename ConvolutionOutputType>
auto HybridScatteringLayerProcessing(const RenderPassBuilder& Builder, bool Enabled)
{
	return [&Builder, Enabled](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();
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

		if (!ViewInfo.DofSettings.BokehShapeIsCircle)
		{
			for (U32 i = 0; i < BokehLUTType::ResourceCount; i++)
			{
				using BuildBokehLUTData = ResourceTable<BokehLUTType>;
				LutOutputTable = Builder.QueueRenderAction("BuildBokehLUTAction", [](RenderContext& Ctx, const BuildBokehLUTData&)
				{
					Ctx.Draw("BuildBokehLUTAction");
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

		if (Enabled)
		{
			for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
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
	};
}

template<typename... DofPostfilterElems>
auto DofPostfilterPass(const RenderPassBuilder& Builder, bool GatherForeGround)
{
	return [&Builder, GatherForeGround](const auto& s)
	{
		const RDAG::SceneViewInfo& ViewInfo = s.template GetHandle<RDAG::SceneViewInfo>();

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
	};
};

typename DepthOfFieldPass::PassOutputType DepthOfFieldPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	PassOutputType Output = Input;

	if (ViewInfo.DepthOfFieldEnabled)
	{
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
		using DofSetupPassData = ResourceTable<RDAG::FullresColorUav, RDAG::GatherColorUav, RDAG::DepthOfFieldUav, RDAG::VelocityVectors>;

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
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
				Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::GatherColorTexture>()
			}),
			Select<RDAG::GatherColorTexture, RDAG::SceneViewInfo>(
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
				BuildBokehLut<RDAG::ScatteringBokehLUTUav>(Builder),
				BuildBokehLut<RDAG::GatheringBokehLUTUav>(Builder),
				ConvolutionGatherPass<RDAG::BackgroundConvolutionTexture>(Builder),
				ConvolutionGatherPass<RDAG::ForegroundConvolutionTexture>(Builder, ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::BackgroundConvolutionUav>(Builder, !ViewInfo.DofSettings.GatherForeground),
				DofPostfilterPass<RDAG::ForegroundConvolutionUav, RDAG::BackgroundConvolutionTexture>(Builder, ViewInfo.DofSettings.GatherForeground),
				HybridScatteringLayerProcessing<RDAG::BackgroundConvolutionUav>(Builder, ViewInfo.DofSettings.EnabledBackgroundLayer),
				HybridScatteringLayerProcessing<RDAG::ForegroundConvolutionUav>(Builder, ViewInfo.DofSettings.EnabledForegroundLayer),
				SlightlyOutOfFocusPass(Builder)
			})),
			BuildBokehLut<RDAG::ScatteringBokehLUTUav>(Builder),
			Builder.QueueRenderAction("RecombineAction", [](RenderContext& Ctx, const RecombineData&)
			{
				Ctx.Draw("RecombineAction");
			})
		}(Input);
	}

	return Output;
}
