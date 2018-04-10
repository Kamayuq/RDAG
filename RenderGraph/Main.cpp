#include <iostream>
#include <cstdio>
#include <chrono>
#include <stdlib.h>

#include "Plumber.h"
#include "GraphCulling.h"
#include "Graphvis.h"
#include "Renderpass.h"
#include "DeferredRenderingPass.h"
#include "RHI.h"
#include "LinearAlloc.h"
#include "DownSamplePass.h"
#include "PostprocessingPass.h"

int ItterationCount = 10000;

namespace RDAG
{
	struct SimpleResourceHandle : Texture2dResourceHandle<SimpleResourceHandle>
	{
		static constexpr const char* Name = "SimpleResourceHandle";

		explicit SimpleResourceHandle() {}
	};
}

int main(int argc, char* argv[])
{	
	RDAG::SceneViewInfo ViewInfo;
	//ViewInfo.AmbientOcclusionType = EAmbientOcclusionType::DistanceField;
	//ViewInfo.TransparencyEnabled = false;
	//ViewInfo.TransparencySeperateEnabled = false;
	//ViewInfo.TemporalAaEnabled = false;
	//ViewInfo.DepthOfFieldEnabled = false;

	//ViewInfo.DofSettings.EnabledForegroundLayer = false;
	//ViewInfo.DofSettings.EnabledBackgroundLayer = false;
	//ViewInfo.DofSettings.BokehShapeIsCircle = true;
	//ViewInfo.DofSettings.GatherForeground = false;
	//ViewInfo.DofSettings.EnablePostfilterMethod = false;
	ViewInfo.DofSettings.RecombineQuality = 0;

	if (argc > 200)
	{
		check(0);
		char* ViewInfoPtr = (char*)&ViewInfo;
		srand(argv[1][0]);
		for (int i = 0; i < (int)sizeof(RDAG::SceneViewInfo); i++)
		{
			ViewInfoPtr[i] = (char)rand();
		}
	}

	RenderPassBuilder Builder;

	//{
		using PassInputType = ResourceTable<>;
		using PassOutputType = ResourceTable<RDAG::SimpleResourceHandle>;
		auto SimpleRenderPass = [](const RenderPassBuilder& Builder, const PassInputType& Input) -> PassOutputType
		{
			Texture2d::Descriptor TargetDescriptor;
			TargetDescriptor.Name = "RenderTarget";
			TargetDescriptor.Format = ERenderResourceFormat::ARGB8U;
			TargetDescriptor.Height = 32;
			TargetDescriptor.Width = 32;

			return Seq
			{
				Builder.CreateResource<RDAG::SimpleResourceHandle>({ TargetDescriptor }),
				Builder.QueueRenderAction<RDAG::SimpleResourceHandle>("SimpleRenderAction", [](RenderContext& Ctx, const PassOutputType&)
				{
					Ctx.Draw("SimpleRenderAction");
				})
			}(Input);
		};

		auto val = Seq
		{
			Builder.BuildRenderPass<RDAG::SimpleResourceHandle>("SimpleRenderPass", SimpleRenderPass),
			Builder.RenameEntry<RDAG::SimpleResourceHandle, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass<RDAG::DownsamplePyramid<16>>("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass<16>::Build),
			Builder.RenameEntry<RDAG::DownsamplePyramid<16>, RDAG::PostProcessingInput>(2, 0),
			Builder.BuildRenderPass<RDAG::PostProcessingResult>("ToneMappingPass", ToneMappingPass::Build)
		}(Builder.GetEmptyResourceTable());
		(void)val;
	//}

	{
		std::chrono::duration<long long, std::nano> minDuration(std::numeric_limits<long long>::max());
		//minDuration = std::numeric_limits<decltype(minDuration)>::max();
		for (int i = 0; i < ItterationCount; i++)
		{
			auto start = std::chrono::high_resolution_clock::now();

			LinearReset();
			Builder.Reset();

			auto val = Seq
			{
				Builder.CreateResource<RDAG::SceneViewInfo>({}, ViewInfo),
				Builder.BuildRenderPass<RDAG::PostProcessingResult>("MainRenderPass", DeferredRendererPass::Build)
			}(Builder.GetEmptyResourceTable());
			(void)val;

			auto time = std::chrono::high_resolution_clock::now() - start;
			minDuration = std::min(minDuration, time);
		}
		std::cout << "build time: " << (std::chrono::duration_cast<std::chrono::microseconds>(minDuration).count()) << "us\n";
	}
	
	{
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < ItterationCount; i++)
		{
			GraphProcessor GPU;
			GPU.ColorGraphNodes(Builder.GetActionList());
		}

		auto time = std::chrono::high_resolution_clock::now() - start;
		std::cout << "optimizer time: " << std::chrono::duration_cast<std::chrono::microseconds>(time).count() / (double)ItterationCount << "us\n";
	}

	{
		GraphvisNeoWriter Writer("../test.dot", Builder.GetActionList());
	}

	std::cin.get();

	{
		GraphProcessor GPU;
		ImmediateRenderContext RndCtx;
		GPU.ScheduleGraphNodes(RndCtx, Builder.GetActionList());
	}

	std::cin.get();

	return 0;
}
