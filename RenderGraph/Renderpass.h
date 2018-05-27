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
public:
	RenderPassBuilder(const RenderPassBuilder&) = delete;
	RenderPassBuilder()
	{}

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
		
		auto& LocalActionList = ActionList;
		return Seq([&LocalActionList, QueuedTask, Name](const InputTableType& input)
		{
			CheckIsValidResourceTable(input);
			//typedef typename std::decay<decltype(s)>::type StateType;

			typedef TRenderPassAction<ContextType, InputTableType, FunctionType> RenderActionType;

			/* create some space on the heap for the action as those are nodes of our graph */
			RenderActionType* NewRenderAction = new (LinearAlloc<RenderActionType>()) RenderActionType(Name, input, QueuedTask);
			LocalActionList.push_back(NewRenderAction);

			//extract the resources which can be written to (like UAVs and Rendertargets)
			using WritableSetType = decltype(Set::template Filter<IsMutableOp>(typename InputTableType::SetType()));
			// merge and link (have the outputs point at this action from now on).
			return NewRenderAction->RenderPassData.Link(WritableSetType());
		});
	}

	/* this function moves a Handle between the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		static_assert(!std::is_same_v<typename From::CompatibleType, typename To::CompatibleType>, "It is not very useful to remane the same resource to itself");
		return Seq([FromIndex, ToIndex](const auto& s)
		{
			CheckIsValidResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<From>(), "Source was not found in the resource table");

			RevisionSet FromEntry = s.template GetRevisionSet<From>();
			U32 ResourceCount = (ToIndex + 1);

			if constexpr (s.template Contains<To>())
			{
				RevisionSet Destination = s.template GetRevisionSet<To>();
				ResourceCount = ResourceCount > Destination.RevisionCount ? ResourceCount : Destination.RevisionCount;
			}

			//make a new destination
			static_assert(To::template IsConvertible<From>(), "HandleTypes do not match");
			RevisionSet ToEntry(LinearAlloc<ResourceRevision>(ResourceCount), ResourceCount);

			if constexpr (s.template Contains<To>()) // destination already in the table
			{
				RevisionSet Destination = s.template GetRevisionSet<To>();

				//copy over the previous entries
				for (U32 i = 0; i < ResourceCount; i++)
				{
					if (i < Destination.RevisionCount)
					{
						ToEntry.Revisions[i] = Destination.Revisions[i];
					}
					else if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToEntry.Revisions[i].ImaginaryResource = To::template OnCreate<To>(RevisionSetInterface<From>::GetDescriptor(FromEntry, FromIndex));
						ToEntry.Revisions[i].Parent = nullptr;
						check(ToEntry.Revisions[i].ImaginaryResource);
					}
				}
			}
			else // destination not in the table and need to be created
			{
				for (U32 i = 0; i < ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToEntry.Revisions[i].ImaginaryResource = To::template OnCreate<To>(RevisionSetInterface<From>::GetDescriptor(FromEntry, FromIndex));
						ToEntry.Revisions[i].Parent = nullptr;
						check(ToEntry.Revisions[i].ImaginaryResource);
					}
				}
			}

			check(FromIndex < FromEntry.RevisionCount);
			check(ToIndex < ResourceCount);
			ToEntry.Revisions[ToIndex] = FromEntry.Revisions[FromIndex];

			//remove the old output and copy it into the new destination
			return ResourceTable<To>{ "RenameEntry", { ToEntry } };
		});
	}

	/* this function moves all Handles from the InputList into the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameAllEntries() const
	{
		static_assert(!std::is_same_v<typename From::CompatibleType, typename To::CompatibleType>, "It is not very useful to remane the same resource to itself");
		return Seq([](const auto& s) 
		{
			CheckIsValidResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<From>(), "Source was not found in the resource table");

			//make a new destination 
			static_assert(To::template IsConvertible<From>(), "HandleTypes do not match");			
			//remove the old output and copy it into the new destination
			return ResourceTable<To>{ "RenameAllEntries", { s.template GetRevisionSet<From>() } };
		});
	}

	template<typename Handle>
	auto CreateResource() const
	{
		return CreateResourceInternal<Handle>(nullptr, 0);
	}

	/* this function adds a new resource to the resourcetable all descriptors have to be provided */ 
	template<typename Handle, unsigned ResourceCount>
	auto CreateResource(const typename Handle::DescriptorType(&InDescriptors)[ResourceCount]) const
	{
		return CreateResourceInternal<Handle>(&InDescriptors[0], ResourceCount);
	}

	template
	<
		typename Handle, typename VectorType,
		typename = std::void_t //use SFINAE to check for supported interface of VectorType
		<
			decltype(std::declval<VectorType>().data()), //requires member function data()
			decltype(std::declval<VectorType>().size())  //requires member function size()
		>
	>
	auto CreateResource(const VectorType& InDescriptors) const
	{
		return CreateResourceInternal<Handle>(InDescriptors.data(), (U32)InDescriptors.size());
	}

	/* returns the empty resourcetable */
	static inline auto GetEmptyResourceTable()
	{
		return ResourceTable<>();
	}

	/* after the builing finished return all the actions recorded */
	const std::vector<const IRenderPassAction*>& GetActionList() const
	{
		return ActionList;
	}

	void Reset()
	{
		ActionList.clear();
	}

private:
	/* this function adds a new resource to the resourcetable all descriptors have to be provided */
	template<typename Handle>
	auto CreateResourceInternal(const typename Handle::DescriptorType* InDescriptors, U32 ResourceCount) const
	{
		RevisionSet WrappedResource(LinearAlloc<ResourceRevision>(ResourceCount), ResourceCount);
		for (U32 i = 0; i < ResourceCount; i++)
		{
			WrappedResource.Revisions[i].ImaginaryResource = Handle::template OnCreate<Handle>(InDescriptors[i]);
			WrappedResource.Revisions[i].Parent = nullptr;
			check(WrappedResource.Revisions[i].ImaginaryResource);
		}

		return Seq([WrappedResource](const auto& s)
		{
			CheckIsValidResourceTable(s);
			return ResourceTable<Handle>{ "CreateResource", { WrappedResource } };
		});
	}

	struct IsMutableOp
	{
		template<typename T>
		static constexpr bool Filter()
		{
			return !T::IsReadOnlyResource;
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

		void Execute(ImmediateRenderContext& RndCtx) const override final
		{
			RenderPassData.OnExecute(RndCtx);
			Task(checked_cast<ContextType&>(RndCtx), RenderPassData);
		}

		IterableResourceTable<RenderPassDataType> RenderPassData;
		FunctionType Task;
	};

	mutable std::vector<const IRenderPassAction*> ActionList;
};
