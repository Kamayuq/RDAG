#pragma once
#include "Types.h"
#include "Plumber.h"
#include "LinearAlloc.h"
#include <unordered_map>
/* example Texture2d implementation */
struct Texture2d : MaterializedResource
{
	struct Descriptor
	{
		const char* Name = "Noname";
		U32 Width = 0;
		U32 Height = 0;
		U32 MipLevel = 1;
		U32 TexSlices = 1;

		void ComputeFullMipChain()
		{
			U32 MaxMipLevel = 1;
			U32 LocalWidth = Width;
			U32 LocalHeight = Height;
			while ((LocalWidth >>= 1) || (LocalHeight >>= 1))
			{
				MaxMipLevel++;
			}
			MipLevel = MaxMipLevel;
		}

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

template<typename CompatibleType>
struct Texture2dResourceHandle : ResourceHandle<CompatibleType>
{
	typedef Texture2d ResourceType;
	typedef Texture2d::Descriptor DescriptorType;
	static constexpr bool IsOutputResource = false;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreate(const typename Handle::DescriptorType& InDescriptor)
	{
		return LinearNew<TransientResourceImpl<Handle>>(InDescriptor);
	}

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		return LinearNew<ResourceType>(Descriptor, EResourceFlags::Managed);
	}

	static void OnExecute(struct ImmediateRenderContext& Ctx, const ResourceType& Resource, U32 SubResourceIndex)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Texture);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}

	static DescriptorType GetSubResourceDescriptor(const DescriptorType& ResourceDescriptor, U32 SubResourceIndex)
	{
		if (SubResourceIndex == ALL_SUBRESOURCE_INDICIES)
		{
			return ResourceDescriptor;
		}
		else
		{
			check(SubResourceIndex < GetSubResourceCount(ResourceDescriptor));
			DescriptorType ReturnValue = ResourceDescriptor;
			ReturnValue.Width >>= (SubResourceIndex % ResourceDescriptor.MipLevel);
			ReturnValue.Height >>= (SubResourceIndex % ResourceDescriptor.MipLevel);
			ReturnValue.MipLevel = 1;
			ReturnValue.TexSlices = 1;
			return ReturnValue;
		}
	}

	static U32 GetSubResourceCount(const DescriptorType& ResourceDescriptor)
	{
		return ResourceDescriptor.MipLevel * ResourceDescriptor.TexSlices;
	}

	template<typename OTHER>
	static constexpr bool IsConvertible()
	{
		static_assert(std::is_base_of_v<Texture2dResourceHandle<typename CompatibleType::CompatibleType>, CompatibleType>, "The compatible Type needs to be same base class");
		return std::is_base_of_v<Texture2dResourceHandle<typename OTHER::CompatibleType>, OTHER>;
	}
};

template<typename CompatibleType>
struct Uav2dResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

template<typename CompatibleType>
struct RendertargetResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		check(SubResourceIndex != ALL_SUBRESOURCE_INDICIES);
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

#define SIMPLE_TEX_HANDLE2(HandleName, Compatible)					\
struct HandleName : Texture2dResourceHandle<Compatible>				\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define SIMPLE_UAV_HANDLE(HandleName, Compatible)					\
struct HandleName : Uav2dResourceHandle<Compatible>					\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define SIMPLE_RT_HANDLE(HandleName, Compatible)					\
struct HandleName : RendertargetResourceHandle<Compatible>			\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define SIMPLE_TEX_HANDLE(HandleName) SIMPLE_TEX_HANDLE2(HandleName, HandleName)

template<typename CompatibleType>
struct ExternalTexture2dResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	typedef Texture2d ResourceType;
	typedef Texture2d::Descriptor DescriptorType;
	static constexpr bool IsOutputResource = false;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreate(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = Texture2dResourceHandle<CompatibleType>::template OnCreate<Handle>(InDescriptor);
		Ret->Materialize(ALL_SUBRESOURCE_INDICIES);
		return Ret;
	}

	static ResourceType* OnMaterialize(const DescriptorType& Descriptor)
	{
		//TODO static string key is not really safe
		static std::unordered_map<const char*, ResourceType*> ExternalResourceMap;
		auto it = ExternalResourceMap.find(Descriptor.Name);
		if (it == ExternalResourceMap.end())
		{
			ResourceType* ExternalResource = new ResourceType(Descriptor, EResourceFlags::External);
			ExternalResourceMap[Descriptor.Name] = ExternalResource;
			return ExternalResource;
		}
		return it->second;
	}
};

template<typename CompatibleType>
struct ExternalUav2dResourceHandle : ExternalTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

template<typename CompatibleType>
struct ExternalRendertargetResourceHandle : ExternalTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		check(SubResourceIndex != ALL_SUBRESOURCE_INDICIES);
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

#define EXTERNAL_TEX_HANDLE2(HandleName, Compatible)				\
struct HandleName : ExternalTexture2dResourceHandle<Compatible>		\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define EXTERNAL_UAV_HANDLE(HandleName, Compatible)					\
struct HandleName : ExternalUav2dResourceHandle<Compatible>			\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define EXTERNAL_RT_HANDLE(HandleName, Compatible)					\
struct HandleName : ExternalRendertargetResourceHandle<Compatible>	\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define EXTERNAL_TEX_HANDLE(HandleName) EXTERNAL_TEX_HANDLE2(HandleName, HandleName)

template<typename CompatibleType>
struct DepthTexture2dResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTexture);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}

	template<typename OTHER>
	static constexpr bool IsConvertible()
	{
		return std::is_base_of_v<DepthTexture2dResourceHandle<typename OTHER::CompatibleType>, OTHER>;
	}
};

template<typename CompatibleType>
struct DepthUav2dResourceHandle : DepthTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

template<typename CompatibleType>
struct DepthRendertargetResourceHandle : DepthTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsOutputResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource, U32 SubResourceIndex)
	{
		check(SubResourceIndex != ALL_SUBRESOURCE_INDICIES);
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTarget);
		Ctx.BindTexture(Resource, SubResourceIndex);
	}
};

#define DEPTH_TEX_HANDLE2(HandleName, Compatible)				\
struct HandleName : DepthTexture2dResourceHandle<Compatible>	\
{																\
	static constexpr const char* Name = #HandleName;			\
};

#define DEPTH_UAV_HANDLE(HandleName, Compatible)					\
struct HandleName : DepthUav2dResourceHandle<Compatible>			\
{																	\
	static constexpr const char* Name = #HandleName;				\
};

#define DEPTH_RT_HANDLE(HandleName, Compatible)					\
struct HandleName : DepthRendertargetResourceHandle<Compatible>	\
{																\
	static constexpr const char* Name = #HandleName;			\
};

#define DEPTH_TEX_HANDLE(HandleName) DEPTH_TEX_HANDLE2(HandleName, HandleName)