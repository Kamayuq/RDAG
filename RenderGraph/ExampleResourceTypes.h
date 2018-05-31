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

template<typename CompatibleType>
struct Texture2dResourceHandle : ResourceHandle<CompatibleType>
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
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource);
	}
};

template<typename CompatibleType>
struct RendertargetResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource);
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


struct ExternalTexture2dDescriptor : Texture2d::Descriptor
{
	I32 Index = -1;
public:
	ExternalTexture2dDescriptor() = default;
	ExternalTexture2dDescriptor(const Texture2d::Descriptor& Other) : Texture2d::Descriptor(Other) {};
};

template<typename CompatibleType>
struct ExternalTexture2dResourceHandle : Texture2dResourceHandle<CompatibleType>
{
	typedef Texture2d ResourceType;
	typedef ExternalTexture2dDescriptor DescriptorType;
	static constexpr bool IsReadOnlyResource = true;

	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreate(const DescriptorType& InDescriptor)
	{
		TransientResourceImpl<Handle>* Ret = Texture2dResourceHandle<CompatibleType>::template OnCreate<Handle>(InDescriptor);
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

template<typename CompatibleType>
struct ExternalUav2dResourceHandle : ExternalTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource);
	}
};

template<typename CompatibleType>
struct ExternalRendertargetResourceHandle : ExternalTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename ExternalTexture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::Target);
		Ctx.BindTexture(Resource);
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
	static constexpr bool IsReadOnlyResource = true;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTexture);
		Ctx.BindTexture(Resource);
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
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::UAV);
		Ctx.BindTexture(Resource);
	}
};

template<typename CompatibleType>
struct DepthRendertargetResourceHandle : DepthTexture2dResourceHandle<CompatibleType>
{
	static constexpr bool IsReadOnlyResource = false;

	static void OnExecute(ImmediateRenderContext& Ctx, const typename Texture2dResourceHandle<CompatibleType>::ResourceType& Resource)
	{
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTarget);
		Ctx.BindTexture(Resource);
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