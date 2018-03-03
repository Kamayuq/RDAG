#pragma once
#include <iostream>
#include "Assert.h"

namespace ERenderBackend
{
	enum Type
	{
		Sequence,
		Parallel,
		Vulkan,
	};
};

struct RenderResourceBase;
struct RenderPassBase;

struct RenderContextBase
{
protected:
	//RenderPassBase* RenderPass = nullptr;

public:
	void BindStatic(const char* /*TypeName*/, U32 /*Offset*/, const RenderResourceBase* /*Resource*/)
	{
		//check(RenderPass != nullptr);
		//printf("DefaultContext: %s Setting Static: %s (Last Modified by: %s) in Slot %s%i \n", RenderPass->GetName(), Resource->GetName(), Resource->LastModifiedBy, TypeName, Offset);
	}

	void BindMutable(const char* /*TypeName*/, U32 /*Offset*/, const RenderResourceBase* /*Resource*/)
	{
		//check(RenderPass != nullptr);
		//printf("DefaultContext: %s Setting Mutable: %s (Last Modified by: %s) in Slot %s%i \n", RenderPass->GetName(), Resource->GetName(), Resource->LastModifiedBy, TypeName, Offset);
		//Resource->LastModifiedBy = RenderPass->GetName();
	}

	void SetRenderPass(RenderPassBase* /*LocalRenderPass*/)
	{
		//if (LocalRenderPass != nullptr)
		//{
		//	check(RenderPass == nullptr);
		//	std::cout << ">>>Begin rendering Pass: " << LocalRenderPass->GetName() << std::endl;
		//} 
		//else
		//{
		//	check(RenderPass != nullptr);
		//	std::cout << "<<<<<End rendering Pass: " << RenderPass->GetName() << std::endl << std::endl;
		//}
		//RenderPass = LocalRenderPass;
	}
};

struct RenderContext final : private RenderContextBase
{
	using RenderContextBase::BindMutable;
	using RenderContextBase::BindStatic;
	using RenderContextBase::SetRenderPass;
};

struct ParallelRenderContext final : private RenderContextBase
{
	using RenderContextBase::BindMutable;
	using RenderContextBase::BindStatic;
	using RenderContextBase::SetRenderPass;
};

struct VulkanRenderContext final : private RenderContextBase
{
	using RenderContextBase::SetRenderPass;

	void BindStatic(const char* /*TypeName*/, U32 /*Offset*/, const RenderResourceBase* /*Resource*/)
	{
		//check(RenderPass != nullptr);
		//printf("VulkanContext: %s Setting Static: %s (Last Modified by: %s) in Slot %s%i \n", RenderPass->GetName(), Resource->GetName(), Resource->LastModifiedBy, TypeName, Offset);
	}

	void BindMutable(const char* /*TypeName*/, U32 /*Offset*/, const RenderResourceBase* /*Resource*/)
	{
		//check(RenderPass != nullptr);
		//printf("VulkanContext: %s Setting Mutable: %s (Last Modified by: %s) in Slot %s%i \n", RenderPass->GetName(), Resource->GetName(), Resource->LastModifiedBy, TypeName, Offset);
		//Resource->LastModifiedBy = RenderPass->GetName();
	}
};

