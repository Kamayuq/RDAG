#pragma once
#include "SharedResources.h"

namespace EAmbientOcclusionType
{
	enum Enum
	{
		DistanceField,
		HorizonBased,
	};
};

namespace RDAG
{
	struct SceneViewInfo : CpuOnlyResourceHandle<SceneViewInfo>
	{
		static constexpr const char* Name = "SceneViewInfo";

		static constexpr const U32 TemporalAAResourceCount = 4;

		explicit SceneViewInfo() {}
		SceneViewInfo(const SceneViewInfo& InViewInfo)
			: DepthFormat(InViewInfo.DepthFormat)
			, SceneWidth(InViewInfo.SceneWidth)
			, SceneHeight(InViewInfo.SceneHeight)
			, ShadowFormat(InViewInfo.ShadowFormat)
			, ShadowCascades(InViewInfo.ShadowCascades)
			, ShadowResolution(InViewInfo.ShadowResolution)
			, TemporalAaEnabled(InViewInfo.TemporalAaEnabled)
			, TransparencyEnabled(InViewInfo.TransparencyEnabled)
			, TransparencySeperateEnabled(InViewInfo.TransparencySeperateEnabled)
			, AmbientOcclusionType(InViewInfo.AmbientOcclusionType)
		{}

		ERenderResourceFormat::Type DepthFormat = ERenderResourceFormat::D32F;
		U32 SceneWidth = 1920;
		U32 SceneHeight = 1080;


		ERenderResourceFormat::Type ShadowFormat = ERenderResourceFormat::D16F;
		U32 ShadowCascades = 4;
		U32 ShadowResolution = 1024;

		bool TemporalAaEnabled = true;
		bool TransparencyEnabled = true;
		bool TransparencySeperateEnabled = true;

		EAmbientOcclusionType::Enum AmbientOcclusionType = EAmbientOcclusionType::HorizonBased;
	};
}