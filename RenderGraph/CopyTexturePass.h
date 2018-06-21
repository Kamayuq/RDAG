#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(CopyDestination, CopyDestination);
	SIMPLE_TEX_HANDLE(CopySource);
}

struct CopyTexturePass
{
	using CopyTextureInput = ResourceTable<RDAG::CopyDestination, RDAG::CopySource>;
	using CopyTextureResult = ResourceTable<RDAG::CopyDestination>;

	static CopyTextureResult Build(const RenderPassBuilder& Builder, const CopyTextureInput& Input);
};