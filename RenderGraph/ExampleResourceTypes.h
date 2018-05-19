#pragma once
#include "Types.h"
#include "Plumber.h"
#include "LinearAlloc.h"


/* example Texture2d implementation */
struct Texture2d : MaterializedResource
{
	struct Descriptor
	{
		const char* Name = "Noname";
		U32 Width = 0;
		U32 Height = 0;
		ERenderResourceFormat::Type Format = ERenderResourceFormat::Invalid;

		bool operator==(const Descriptor& Other) const
		{
			if (Format != Other.Format)
				return false;

			if (Width != Other.Width)
				return false;

			if (Height != Other.Height)
				return false;

			if (strcmp(Name, Other.Name) != 0)
				return false;

			return true;
		}
	};

	explicit Texture2d(const Descriptor& InDesc, EResourceFlags::Type InResourceFlags)
		: MaterializedResource(InResourceFlags), Desc(InDesc)
	{

	}

	const char* GetName() const
	{
		return Desc.Name;
	}

	bool RequiresTransition(EResourceTransition::Type& OldState, EResourceTransition::Type NewState) const
	{
		if (NewState != CurrentState)
		{
			OldState = CurrentState;
			CurrentState = NewState;
			return true;
		}
		return false;
	}

private:
	Descriptor Desc;
	mutable EResourceTransition::Type CurrentState = EResourceTransition::Undefined;
};

template<typename CRTP>
struct Texture2dResourceHandle : ResourceHandle<CRTP>
{
	typedef Texture2d ResourceType;
	typedef Texture2d::Descriptor DescriptorType;
	static constexpr bool IsReadOnlyResource = true;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreate(const typename Handle::DescriptorType& InDescriptor)
	{
		return LinearNew<TransientResourceImpl<Handle>>(InDescriptor);
	}

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		return LinearNew<ResourceType>(Descriptor, EResourceFlags::Managed);
	}

	static void OnExecute(struct ImmediateRenderContext& Ctx, const ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Texture);
		Ctx.BindTexture(Resource);
	}
};

template<typename CRTP>
struct Uav2dResourceHandle : Texture2dResourceHandle<CRTP>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CRTP>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource);
	}
};

template<typename CRTP>
struct RendertargetResourceHandle : Texture2dResourceHandle<CRTP>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CRTP>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource);
	}
};

struct ExternalTexture2dDescriptor : Texture2d::Descriptor
{
	I32 Index = -1;
public:
	ExternalTexture2dDescriptor() = default;
	ExternalTexture2dDescriptor(const Texture2d::Descriptor& Other) : Texture2d::Descriptor(Other) {};
};

template<typename CRTP>
struct ExternalTexture2dResourceHandle : Texture2dResourceHandle<CRTP>
{
	typedef Texture2d ResourceType;
	typedef ExternalTexture2dDescriptor DescriptorType;
	static constexpr bool IsReadOnlyResource = true;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreate(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = Texture2dResourceHandle<CRTP>::template OnCreate<Handle>(InDescriptor);
		if (InDescriptor.Index >= 0)
		{
			Ret->Materialize();
		}
		return Ret;
	}

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		static std::vector<ResourceType*> ExternalResourceMap;
		if (Descriptor.Index < 0)
		{
			return nullptr;
		}

		if (ExternalResourceMap.size() <= (size_t)Descriptor.Index)
		{
			ExternalResourceMap.resize(Descriptor.Index + 1);
		}

		if (ExternalResourceMap[Descriptor.Index] == nullptr)
		{
			ExternalResourceMap[Descriptor.Index] = new ResourceType(Descriptor, EResourceFlags::External);
		}

		return ExternalResourceMap[Descriptor.Index];
	}
};

template<typename CRTP>
struct ExternalUav2dResourceHandle : ExternalTexture2dResourceHandle<CRTP>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CRTP>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource);
	}
};

template<typename CRTP>
struct ExternalRendertargetResourceHandle : ExternalTexture2dResourceHandle<CRTP>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CRTP>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource);
	}
};