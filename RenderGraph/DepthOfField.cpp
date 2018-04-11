#include "DepthOfField.h"
#include "VelocityPass.h"
#include "TemporalAA.h"

#define SIMPLE_TEX_HANDLE(HandleName)								\
struct HandleName : Texture2dResourceHandle<HandleName>				\
{																	\
	static constexpr const char* Name = #HandleName;				\
	explicit HandleName() {}										\
	template<typename CRTP>											\
	explicit HandleName(const Texture2dResourceHandle<CRTP>&) {}	\
};														

#define SIMPLE_UAV_HANDLE(HandleName, Compatible)					\
struct HandleName : Uav2dResourceHandle<Compatible>					\
{																	\
	static constexpr const char* Name = #HandleName;				\
	explicit HandleName() {}										\
	template<typename CRTP>											\
	explicit HandleName(const Texture2dResourceHandle<CRTP>&) {}	\
};	

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

	struct BackgroundConvolutionTexture : Texture2dResourceHandle<BackgroundConvolutionTexture>
	{
		static constexpr const U32 ResourceCount = 2;
		static constexpr const char* Name = "BackgroundConvolutionTexture";

		explicit BackgroundConvolutionTexture() {}
		template<typename CRTP>	
		explicit BackgroundConvolutionTexture(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct BackgroundConvolutionUav : Uav2dResourceHandle<BackgroundConvolutionTexture>
	{
		static constexpr const U32 ResourceCount = 2;
		static constexpr const char* Name = "BackgroundConvolutionUav";

		explicit BackgroundConvolutionUav() {}
		template<typename CRTP>
		explicit BackgroundConvolutionUav(const Texture2dResourceHandle<CRTP>&) {}
	};

	SIMPLE_TEX_HANDLE(SlightOutOfFocusConvolutionTexture);
	SIMPLE_UAV_HANDLE(SlightOutOfFocusConvolutionUav, SlightOutOfFocusConvolutionTexture);
	SIMPLE_TEX_HANDLE(ScatteringReduceTexture);
	SIMPLE_UAV_HANDLE(ScatteringReduceUav, ScatteringReduceTexture);
	SIMPLE_TEX_HANDLE(ScatterCompilationTexture);
	SIMPLE_UAV_HANDLE(ScatterCompilationUav, ScatterCompilationTexture);
};

#undef SIMPLE_TEX_HANDLE
#undef SIMPLE_UAV_HANDLE

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
		
		using ScatteringReduceData = ResourceTable<RDAG::ScatteringReduceUav, RDAG::GatherColorTexture>;
		using ScatterCompilationData = ResourceTable< RDAG::ScatterCompilationUav, RDAG::ScatteringReduceTexture>;
		using DOFHybridScatter = ResourceTable<ConvolutionOutputType, RDAG::ScatterCompilationTexture, RDAG::ScatteringBokehLUTTexture>;

		using ResourceTableType = std::decay_t<decltype(s)>;
		ResourceTableType Result = s;
		if(Enabled)
		{		
			Result = Seq
			{
				Builder.CreateResource<RDAG::ScatteringReduceTexture>({ ScatteringReduceDesc }),
				Builder.QueueRenderAction("ScatteringReduceAction", [](RenderContext& Ctx, const ScatteringReduceData& Resources) -> ResourceTable<RDAG::ScatteringReduceUav>
				{
					Ctx.Draw("ScatteringReduceAction");
					return Resources;
				}),
				Builder.CreateResource<RDAG::ScatterCompilationTexture>({ ScatterCompilationDesc }),
				Builder.QueueRenderAction("ScatterCompilationAction", [](RenderContext& Ctx, const ScatterCompilationData& Resources) -> ResourceTable<RDAG::ScatterCompilationUav>
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
		using GatherPassData = ResourceTable<RDAG::ConvolutionUav, RDAG::GatheringBokehLUTTexture, RDAG::PrefilterTexture, RDAG::CocTileTexture>;

		if (Enabled)
		{
			for (U32 i = 0; i < ConvolutionGatherType::ResourceCount; i++)
			{
				ConvolutionOutputTable = Seq
				{
					Builder.RenameEntry<ConvolutionGatherType, RDAG::ConvolutionUav>(i, 0),
					Builder.QueueRenderAction("GatherPassDataAction", [](RenderContext& Ctx, const GatherPassData& Resources) -> ResourceTable<RDAG::ConvolutionUav>
					{
						Ctx.Draw("GatherPassDataAction");
						return Resources;
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

		auto SlightlyOutOfFocusTable = Builder.CreateResource<RDAG::SlightOutOfFocusConvolutionUav>({ SlightOutOfFocusConvolutionDesc })(s);
		if (ViewInfo.DofSettings.RecombineQuality > 0)
		{
			using GatherPassData = ResourceTable<RDAG::SlightOutOfFocusConvolutionUav, RDAG::ScatteringBokehLUTTexture, RDAG::PrefilterTexture, RDAG::CocTileTexture>;
			SlightlyOutOfFocusTable = Builder.QueueRenderAction("SlightlyOutOfFocusAction", [](RenderContext& Ctx, const GatherPassData& Resources) -> ResourceTable<RDAG::SlightOutOfFocusConvolutionUav>
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
		FullresColorDesc.Name = "FullresColorTexture";
		FullresColorDesc.Format = ERenderResourceFormat::ARGB16F;
		FullresColorDesc.Height = ViewInfo.SceneHeight;
		FullresColorDesc.Width = ViewInfo.SceneWidth;

		Texture2d::Descriptor GatherColorDesc;
		GatherColorDesc.Name = "GatherColorTexture";
		GatherColorDesc.Format = ERenderResourceFormat::ARGB16F;
		GatherColorDesc.Height = ViewInfo.SceneHeight >> 1;
		GatherColorDesc.Width = ViewInfo.SceneWidth >> 1;
		using DofSetupPassData = ResourceTable<RDAG::FullresColorUav, RDAG::GatherColorUav, RDAG::DepthOfFieldInput, RDAG::VelocityVectors>;

		Texture2d::Descriptor CocTileDesc = GatherColorDesc;
		GatherColorDesc.Name = "CocTileTexture";
		using CocDilateData = ResourceTable< RDAG::CocTileUav, RDAG::GatherColorTexture>;

		Texture2d::Descriptor PrefilterOutputDesc = GatherColorDesc;
		PrefilterOutputDesc.Name = "PrefilterOutputDesc";
		using PreFilterData = ResourceTable<RDAG::PrefilterUav, RDAG::GatherColorTexture>;

		Texture2d::Descriptor OutputDesc = Input.GetDescriptor<RDAG::DepthOfFieldInput>();
		OutputDesc.Name = "DepthOfFieldOutput";
		using RecombineData = ResourceTable<RDAG::DepthOfFieldOutput, RDAG::ScatteringBokehLUTTexture, RDAG::SlightOutOfFocusConvolutionTexture, RDAG::ForegroundConvolutionTexture, RDAG::BackgroundConvolutionTexture, RDAG::FullresColorTexture>;

		return Seq
		{
			Builder.CreateResource<RDAG::GatherColorUav>({ GatherColorDesc }),
			Builder.CreateResource<RDAG::FullresColorUav>({ FullresColorDesc }),
			Builder.QueueRenderAction("DOFSetupAction", [](RenderContext& Ctx, const DofSetupPassData& Resources) -> ResourceTable<RDAG::FullresColorUav, RDAG::GatherColorUav>
			{
				Ctx.Draw("DOFSetupAction");
				return Resources;
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
				Builder.QueueRenderAction("CocDilateAction", [](RenderContext& Ctx, const CocDilateData& Resources) -> ResourceTable<RDAG::CocTileUav>
				{
					Ctx.Draw("CocDilateAction");
					return Resources;
				}),
				Builder.CreateResource<RDAG::PrefilterUav>({ PrefilterOutputDesc }),
				Builder.QueueRenderAction("PreFilterAction", [](RenderContext& Ctx, const PreFilterData& Resources) -> ResourceTable<RDAG::PrefilterUav>
				{
					Ctx.Draw("PreFilterAction");
					return Resources;
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
