#pragma once
#include "LinearAlloc.h"
#include "Sequence.h"
#include "Plumber.h"
#include "RHI.h"
#include "Sequence.h"
#include <vector>

/* Base class of all actions which can contain dispatches or draws */
struct IRenderPassAction
{
	IRenderPassAction(const char* InName) : Name(InName) {}

	virtual ~IRenderPassAction() {}
	virtual const class IResourceTableInfo& GetRenderPassData() const = 0;
	virtual void Execute(struct ImmediateRenderContext&) const {};

	const char* GetName() const { return Name; };
	
	/* coloring is used to find independent paths though the graph */
	void SetColor(U32 InColor) const { Color = InColor; }
	U32 GetColor() const { return Color; }
private:
	const char* Name = nullptr;
	mutable U32 Color = UINT_MAX; //the node is culled to begin with
};

struct RenderPassBuilder
{
private:
	using ActionListType = std::vector<const IRenderPassAction*>;
	mutable ActionListType ActionList;

public:
	RenderPassBuilder(const RenderPassBuilder&) = delete;
	RenderPassBuilder(){}

	/* run an renderpass in another callable or function this can be useful to seperate the definition from the implementation in a cpp file */
	template<typename FunctionType, typename... ARGS>
	auto BuildRenderPass(const char* Name, const FunctionType& BuildFunction, const ARGS&... Args) const
	{
		/* sanity checking if the passed in variables make sense, otherwise fail early */
		typedef Traits::function_traits<FunctionType> Traits;
		static_assert(Traits::arity == (2 + sizeof...(Args)), "Build Functions have at least 2 parameters (the builder and the input table) plus the extra arguments");
		typedef std::decay_t<typename Traits::template arg<0>::type> BuilderType;
		static_assert(std::is_same_v<RenderPassBuilder, BuilderType>, "The 1st parameter must be a builder");
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		static_assert(std::is_base_of_v<IResourceTableBase, InputTableType>, "The 2nd parameter must be a resource table");
		typedef std::decay_t<typename Traits::return_type> NestedOutputTableType;
		static_assert(std::is_base_of_v<IResourceTableBase, NestedOutputTableType>, "The returntype must be a resource table");

		const RenderPassBuilder* Self = this;
		return Seq([&, Self, Name](const InputTableType& input)
		{
			CheckIsValidResourceTable(input);		
			//no heap allocation just run the build and merge the results (no linking as these are not real types!)
			return NestedOutputTableType(Name, BuildFunction(*Self, input, Args...));
		});
	}

	template<typename FunctionType>
	auto QueueRenderAction(const char* Name, const FunctionType& QueuedTask) const
	{
		/* sanity checking if the passed in variables make sense, otherwise fail early */
		typedef Traits::function_traits<FunctionType> Traits; 
		static_assert(Traits::arity == 2, "Queue Functions have 2 parameters: the rendercontext and the input table");
		typedef std::decay_t<typename Traits::template arg<0>::type> ContextType;
		static_assert(std::is_base_of_v<RenderContextBase, ContextType>, "The 1st parameter must be a rendercontext type");
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		static_assert(std::is_base_of_v<IResourceTableBase, InputTableType>, "The 2nd parameter must be a resource table");
		typedef std::decay_t<typename Traits::return_type> VoidReturnType;
		static_assert(std::is_same_v<void, VoidReturnType>, "The returntype must be void");
		
		ActionListType& LocalActionList = ActionList;
		return Seq([&LocalActionList, QueuedTask, Name](const InputTableType& input)
		{
			CheckIsValidResourceTable(input);

			typedef TRenderPassAction<ContextType, InputTableType, FunctionType> RenderActionType;

			/* create some space on the heap for the action as those are nodes of our graph */
			RenderActionType* NewRenderAction = new (LinearAlloc<RenderActionType>()) RenderActionType(Name, input, QueuedTask);
			LocalActionList.push_back(NewRenderAction);

			//extract the resources which can be written to (like UAVs and Rendertargets)
			using WritableSetType = decltype(Set::template Filter<IsMutableOp>(typename InputTableType::HandleTypes()));
			// merge and link (have the outputs point at this action from now on).
			return NewRenderAction->RenderPassData.Link(WritableSetType());
		});
	}

	template<typename Handle>
	auto DisableEntry() const
	{
		return Seq([](const auto& s)
		{
			CheckIsValidResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<Handle>(), "Source was not found in the resource table");

			SubResourceRevision SubResource = s.template GetSubResource<Handle>();
			SubResource.Revision.Parent = nullptr;

			//remove the old output and copy it into the new destination
			return ResourceTable<Handle>{ "DisableEntry", { SubResource } };
		});
	}

	/* this function moves a Handle between the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto AssignEntry(U32 DestinationSubResourceIndex = ALL_SUBRESOURCE_INDICIES) const
	{
		static_assert(!std::is_same_v<typename From::CompatibleType, typename To::CompatibleType>, "It is not very useful to rename the same resource to itself");
		static_assert(std::is_same_v<typename From::ResourceType, typename To::ResourceType>, "ResourceTypes must match");
		static_assert(std::is_same_v<typename From::DescriptorType, typename To::DescriptorType>, "DescriptorTypes must match");

		return Seq([DestinationSubResourceIndex](const auto& s)
		{
			CheckIsValidResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<From>(), "Source was not found in the resource table");

			//make a new destination 
			static_assert(To::template IsConvertible<From>(), "HandleTypes do not match");
			//remove the old output and copy it into the new destination
			SubResourceRevision SubResource = s.template GetSubResource<From>();
			U32 NumSubResources = SubResource.Revision.ImaginaryResource->GetNumSubResources();
			SubResource.SubResourceIndex = (DestinationSubResourceIndex == ALL_SUBRESOURCE_INDICIES) && (NumSubResources == 1) ? 0 : DestinationSubResourceIndex;
			return ResourceTable<To>{ "RenameAllEntries", { SubResource } };
		});
	}

	/* after the builing finished return all the actions recorded */
	const ActionListType& GetActionList() const
	{
		return ActionList;
	}

	void Reset()
	{
		ActionList.clear();
	}

	/* this function adds a new resource to the resourcetable all descriptors have to be provided */ 
	template<typename Handle>
	auto CreateResource(const typename Handle::DescriptorType& Descriptor) const
	{
		U32 NumSubResources = Handle::GetSubResourceCount(Descriptor);
		SubResourceRevision WrappedResource;	
		WrappedResource.Revision.ImaginaryResource = Handle::template OnCreate<Handle>(Descriptor);
		WrappedResource.Revision.Parent = nullptr;
		WrappedResource.SubResourceIndex = NumSubResources == 1 ? 0 : ALL_SUBRESOURCE_INDICIES;

		return Seq([WrappedResource](const auto& s)
		{
			CheckIsValidResourceTable(s);
			return ResourceTable<Handle>{ "CreateResource", { WrappedResource } };
		});
	}

private:
	struct IsMutableOp
	{
		template<typename T>
		static constexpr bool Filter()
		{
			return T::IsOutputResource;
		}
	};

	template<typename ContextType, typename RenderPassDataType, typename FunctionType>
	struct TRenderPassAction final : IRenderPassAction
	{
		friend struct RenderPassBuilder;

		TRenderPassAction(const char* Name, const RenderPassDataType& InRenderPassData, const FunctionType& InTask)
			: IRenderPassAction(Name)
			, RenderPassData(InRenderPassData, Name, this)
			, Task(InTask) {}

	private:
		const IResourceTableInfo& GetRenderPassData() const override
		{
			return RenderPassData;
		}

		void Execute(ImmediateRenderContext& RndCtx) const override
		{
			RenderPassData.OnExecute(RndCtx);
			Task(checked_cast<ContextType&>(RndCtx), RenderPassData);
		}

		IterableResourceTable<RenderPassDataType> RenderPassData;
		FunctionType Task;
	};
};
