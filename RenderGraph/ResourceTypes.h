#pragma once
#include "Types.h"
#include "Plumber.h"
#include "LinearAlloc.h"

namespace ERenderResourceFormat
{
	enum Enum
	{
		ARGB8U,
		ARGB16F,
		ARGB16U,
		RG16F,
		L8,
		D16F,
		D32F,
		Structured,
		Invalid,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Invalid) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};

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

	explicit Texture2d(const Descriptor&, EResourceFlags::Type InResourceFlags) : MaterializedResource(InResourceFlags)
	{

	}
};

struct Texture2dResourceHandle : ResourceHandle
{
	typedef Texture2d ResourceType;
	typedef Texture2d::Descriptor DescriptorType;

	static ResourceType* Materialize(const DescriptorType& Descriptor)
	{
		return new (LinearAlloc<ResourceType>()) ResourceType(Descriptor, EResourceFlags::Managed);
	}
};

struct ExternalTexture2dResourceHandle : Texture2dResourceHandle
{
	struct Descriptor : Texture2d::Descriptor
	{
		I32 Index = -1;
	public:
		Descriptor& operator= (const Texture2d::Descriptor& Other)
		{ 
			(*static_cast<Texture2d::Descriptor*>(this)) = Other;
			return *this; 
		};
	};

	typedef Texture2d ResourceType;
	typedef Descriptor DescriptorType;

	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateInput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = nullptr;
		if (InDescriptor.Index != -1)
		{
			Ret = Texture2dResourceHandle::CreateInput<Handle>(InDescriptor);
			Ret->Materialize();
		}
		return Ret;
	}

	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateOutput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = nullptr;
		if (InDescriptor.Index != -1)
		{
			Ret = Texture2dResourceHandle::CreateOutput<Handle>(InDescriptor);
			Ret->Materialize();
		}
		return Ret;
	}

	static ResourceType* Materialize(const DescriptorType& Descriptor)
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

struct CpuOnlyResourceHandle : ResourceHandle
{
	typedef CpuOnlyResource ResourceType;
	typedef CpuOnlyResource::Descriptor DescriptorType;

	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateInput(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = ResourceHandle::CreateInput<Handle>(InDescriptor);
		Ret->Materialize();
		return Ret;
	}

	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateOutput(const DescriptorType& InDescriptor)
	{
		check(false && "Not valid Use Case");
		return nullptr;
	}

	static ResourceType* Materialize(const DescriptorType&)
	{
		return nullptr;
	}
};