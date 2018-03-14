#pragma once
#include "Types.h"
#include "Plumber.h"
#include "LinearAlloc.h"

template<typename CRTP>
struct Texture2dResourceHandle : ResourceHandle<CRTP>
{
	typedef Texture2d ResourceType;
	typedef Texture2d::Descriptor DescriptorType;

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		return new (LinearAlloc<ResourceType>()) ResourceType(Descriptor, EResourceFlags::Managed);
	}

	void OnExecute(struct ImmediateRenderContext& Ctx, const ResourceType& Resource) const
	{
		Ctx.BindTexture(Resource);
	}
};

struct ExternalTexture2dDescriptor : Texture2d::Descriptor
{
	I32 Index = -1;
public:
	Descriptor & operator= (const Texture2d::Descriptor& Other)
	{
		(*static_cast<Texture2d::Descriptor*>(this)) = Other;
		return *this;
	};
};

template<typename CRTP>
struct ExternalTexture2dResourceHandle : Texture2dResourceHandle<CRTP>
{
	typedef Texture2d ResourceType;
	typedef ExternalTexture2dDescriptor DescriptorType;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateInput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = nullptr;
		if (InDescriptor.Index != -1)
		{
			Ret = Texture2dResourceHandle<CRTP>::template OnCreateInput<Handle>(InDescriptor);
			Ret->Materialize();
		}
		return Ret;
	}

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateOutput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = nullptr;
		if (InDescriptor.Index != -1)
		{
			Ret = Texture2dResourceHandle<CRTP>::template OnCreateOutput<Handle>(InDescriptor);
			Ret->Materialize();
		}
		return Ret;
	}

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		static std::vector<ResourceType*> ExternalResourceMap;
		check(Descriptor.Index >= 0);

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

struct CpuOnlyResource : MaterializedResource
{
	struct Descriptor
	{
	};

	explicit CpuOnlyResource(const Descriptor&) : MaterializedResource(EResourceFlags::Discard)
	{
	}
};

template<typename CRTP>
struct CpuOnlyResourceHandle : ResourceHandle<CRTP>
{
	typedef CpuOnlyResource ResourceType;
	typedef CpuOnlyResource::Descriptor DescriptorType;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateInput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = ResourceHandleBase::OnCreateInput<Handle>(InDescriptor);
		Ret->Materialize();
		return Ret;
	}

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateOutput(const DescriptorType& InDescriptor)
	{
		check(false && "Not valid Use Case");
		return nullptr;
	}

	static ResourceType* OnMaterialize(const DescriptorType&)
	{
		return nullptr;
	}

	void OnExecute(struct ImmediateRenderContext&, const ResourceType&) const
	{
	}
};