#include "VelocityPass.h"

namespace RDAG
{
	SIMPLE_RT_HANDLE(VelocityVectorTarget, VelocityVectors);
}

typename VelocityRenderPass::VelocityRenderResult VelocityRenderPass::Build(const RenderPassBuilder& Builder, const VelocityRenderInput& Input)
{
	const Texture2d::Descriptor& DepthInfo = Input.GetDescriptor<RDAG::DepthTarget>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

	using VelocityRenderAction = ResourceTable<RDAG::VelocityVectorTarget, RDAG::DepthTexture>;
	return Seq
	{
		Builder.CreateResource<RDAG::VelocityVectors>( VelocityDescriptor ),
		Builder.QueueRenderAction("VelocityRenderAction", [](RenderContext& Ctx, const VelocityRenderAction&)
		{
			Ctx.Draw("VelocityRenderAction");
		})
	}(Input);
}