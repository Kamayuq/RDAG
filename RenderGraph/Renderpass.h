#pragma once
#include "LinearAlloc.h"
#include "Sequence.h"
#include "Plumber.h"
#include "RHI.h"

#include <vector>

/* Base class of all actions which can contain dispatches or draws */
struct IRenderPassAction
{
	IRenderPassAction(const char* InName) : Name(InName) {}

	virtual ~IRenderPassAction() {}
	virtual const class IResourceTableInfo* GetRenderPassData() const = 0;
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
		static_assert(Traits::arity >= 2, "Build Functions have at least 2 parameters: the builder and the input table");
		typedef std::decay_t<typename Traits::template arg<0>::type> BuilderType;
		static_assert(std::is_same_v<RenderPassBuilder, BuilderType>, "The 1st parameter must be a builder");
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		static_assert(std::is_base_of_v<IResourceTableBase, InputTableType>, "The 2nd parameter must be a resource table");
		typedef std::decay_t<typename Traits::return_type> NestedOutputTableType;
		static_assert(std::is_base_of_v<IResourceTableBase, NestedOutputTableType>, "The returntype must be a resource table");

		const RenderPassBuilder* Self = this;
		return [&, Self, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			InputTableType input = s;
			
			//no heap allocation just run the build and merge the results (no linking as these are not real types!)
			NestedOutputTableType NestedRenderPassData(Name, BuildFunction(*Self, input, Args...));
			return s.Union(NestedRenderPassData);
		};
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
		return [&LocalActionList, QueuedTask, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			//typedef typename std::decay<decltype(s)>::type StateType;

			typedef TRenderPassAction<ContextType, InputTableType, FunctionType> RenderActionType;
			InputTableType input = s;

			/* create some space on the heap for the action as those are nodes of our graph */
			RenderActionType* NewRenderAction = new (LinearAlloc<RenderActionType>()) RenderActionType(Name, input, QueuedTask);
			LocalActionList.push_back(NewRenderAction);

			auto WritableSet = Set::template Filter<IsMutableOp>(InputTableType::GetSetType());
			// merge and link (have the outputs point at this action from now on).
			return NewRenderAction->RenderPassData.Link(WritableSet, s);
		};
	}

	/* this function moves a Handle between the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		static_assert(!std::is_same_v<typename From::CompatibleType, typename To::CompatibleType>, "It is not very useful to remane the same resource to itself");
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<From>(), "Source was not found in the resource table");

			auto FromEntry = s.template GetWrapped<From>();
			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToEntry(To(FromEntry.GetHandle()), {});

			if constexpr (s.template Contains<To>())
			{
				//copy over the old revisions to not loose entries
				const auto& Destination = s.template GetWrapped<To>();
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					ToEntry.Revisions[i] = Destination.Revisions[i];
				}
			}
			else
			{
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToEntry.Revisions[i].ImaginaryResource = To::template OnCreate<To>(FromEntry.GetDescriptor(FromIndex));
						check(ToEntry.Revisions[i].ImaginaryResource);
					}
				}
			}

			ToEntry.Revisions[ToIndex] = FromEntry.Revisions[FromIndex];
			ResourceTable<To> DestTable { "RenameEntry", ToEntry };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	/* this function moves all Handles from the InputList into the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameAllEntries() const
	{
		static_assert(From::ResourceCount == To::ResourceCount, "ResourceCounts must match");
		static_assert(!std::is_same_v<typename From::CompatibleType, typename To::CompatibleType>, "It is not very useful to remane the same resource to itself");
		return [](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(StateType::template Contains<From>(), "Source was not found in the resource table");

			Wrapped<From> FromEntry = s.template GetWrapped<From>();

			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToEntry(To(FromEntry.GetHandle()), {});

			for (U32 i = 0; i < To::ResourceCount; i++)
			{
				ToEntry.Revisions[i] = FromEntry.Revisions[i];
			}
			ResourceTable<To> DestTable{ "RenameAllEntries", ToEntry };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}
	/* this function adds a new resource to the resourcetable all descriptors have to be provided */ 
	template<typename Handle, typename... ARGS>
	auto CreateResource(const typename Handle::DescriptorType(&InDescriptors)[Handle::ResourceCount], const ARGS&... Args) const
	{
		ResourceRevision Revisions[Handle::ResourceCount];
		for (U32 i = 0; i < Handle::ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template OnCreate<Handle>(InDescriptors[i]);
			check(Revisions[i].ImaginaryResource);
		}
		auto WrappedResource = Wrapped<Handle>(Handle(Args...), Revisions);

		return [WrappedResource](const auto& s)
		{	
			CheckIsResourceTable(s);
			auto NewResourceTable = ResourceTable<Handle>("CreateResource", WrappedResource);
			return s.Union(NewResourceTable);
		};
	}

	/* returns the empty resourcetable */
	static inline auto GetEmptyResourceTable()
	{
		return ResourceTable<>("EmptyResourceTable");
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
	struct IsMutableOp
	{
		template<typename T>
		static constexpr bool Test()
		{
			return !T::IsReadOnlyResource;
		}
	};

	template<typename ResourceTableType>
	static void CheckIsResourceTable(const ResourceTableType& Table)
	{
		static_assert(std::is_base_of<IResourceTableBase, ResourceTableType>(), "Table is not a ResorceTable");
		Table.CheckAllValid();
	}

	template<typename ContextType, typename RenderPassDataType, typename FunctionType>
	struct TRenderPassAction final : IRenderPassAction
	{
		friend struct RenderPassBuilder;

		TRenderPassAction(const char* Name, const RenderPassDataType& InRenderPassData, const FunctionType& InTask)
			: IRenderPassAction(Name)
			, RenderPassData(InRenderPassData, Name, this)
			, Task(InTask) {}

	private:
		const IResourceTableInfo* GetRenderPassData() const override
		{
			return &RenderPassData;
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
