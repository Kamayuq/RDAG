#pragma once
#include "ExampleResourceTypes.h"

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
	struct DepthOfFieldSettings
	{
		bool EnabledForegroundLayer = true;
		bool EnabledBackgroundLayer = true;
		bool BokehShapeIsCircle = false;
		bool GatherForeground = true;
		bool EnablePostfilterMethod = true;
		U32 RecombineQuality = 3;
	};

	struct SceneViewInfo : CpuOnlyResourceHandle<SceneViewInfo>
	{
		static constexpr const char* Name = "SceneViewInfo";

		explicit SceneViewInfo() {}
		SceneViewInfo(const SceneViewInfo& InViewInfo) = default;

		ERenderResourceFormat::Type DepthFormat = ERenderResourceFormat::D32F;
		U32 SceneWidth = 1920;
		U32 SceneHeight = 1080;


		ERenderResourceFormat::Type ShadowFormat = ERenderResourceFormat::D16F;
		U32 ShadowCascades = 4;
		U32 ShadowResolution = 1024;

		bool DepthOfFieldEnabled = true;
		bool TemporalAaEnabled = true;
		bool TransparencyEnabled = true;
		bool TransparencySeperateEnabled = true;

		EAmbientOcclusionType::Enum AmbientOcclusionType = EAmbientOcclusionType::HorizonBased;
		
		DepthOfFieldSettings DofSettings;
	};

	struct SceneColorTexture : Texture2dResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "SceneColorTexture";

		explicit SceneColorTexture() {}

		template<typename CRTP>
		explicit SceneColorTexture(const Texture2dResourceHandle<CRTP>&) {}
	}; 
}