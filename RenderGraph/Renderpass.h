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
	template<typename... LinkedTypes, typename FunctionType, typename... ARGS>
	auto BuildRenderPass(const char* Name, const FunctionType& BuildFunction, const ARGS&... Args) const
	{
		/* sanity checking if the passed in variables make sense, otherwise fail early */
		typedef Traits::function_traits<FunctionType> Traits;
		//typedef std::decay_t<typename Traits::template arg<0>::type> BuilderType;
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		typedef std::decay_t<typename Traits::return_type> NestedOutputTableType;
		static_assert(Traits::arity >= 2, "Build Functions have at least 2 parameters: the builder and the input table");
		static_assert(std::is_base_of_v<IResourceTableBase, NestedOutputTableType>, "The returntype must be a resource table");
		static_assert(std::is_base_of_v<IResourceTableBase, InputTableType>, "The 2nd parameter must be a resource table");

		const RenderPassBuilder* Self = this;
		return [&, Self, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			InputTableType input = s;
			
			//no heap allocation just run the build and merge the results (no linking as these are not real types!)
			NestedOutputTableType NestedRenderPassData(BuildFunction(*Self, input, Args...), Name);
			return NestedRenderPassData.Merge(s);
		};
	}

	template<typename... LinkedTypes, typename FunctionType>
	auto QueueRenderAction(const char* Name, const FunctionType& QueuedTask) const
	{
		/* sanity checking if the passed in variables make sense, otherwise fail early */
		typedef Traits::function_traits<FunctionType> Traits;
		typedef std::decay_t<typename Traits::template arg<0>::type> ContextType;
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		typedef std::decay_t<typename Traits::return_type> VoidReturnType;
		static_assert(Traits::arity == 2, "Queue Functions have 2 parameters: the rendercontext and the input table");
		static_assert(std::is_same_v<VoidReturnType, void>, "The returntype must be void");
		static_assert(std::is_base_of_v<RenderContextBase, ContextType>, "The 1st parameter must be a rendercontext type");
		static_assert(std::is_base_of_v<IResourceTableBase, InputTableType>, "The 2nd parameter must be a resource table");

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

			// merge and link (have the outputs point at this action from now on).
			return NewRenderAction->RenderPassData.MergeAndLink(s);
		};
	}

	/* this function moves a Handle between the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameOutputToOutput(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::OutputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromOutput = s.template GetWrappedOutput<From>();
			OutputTableType<From> SourceTable{ InputTable<>(), OutputTable<From>(FromOutput) };
			
			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToOutput(To(FromOutput.GetHandle()), {});
			if constexpr (s.template ContainsOutput<To>())
			{
				//copy over the old revisions to not loose entries
				const auto& Destination = s.template GetWrappedOutput<To>();
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					ToOutput.Revisions[i] = Destination.Revisions[i];
				}
			}
			else
			{
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToOutput.Revisions[i].ImaginaryResource = To::template OnCreateInput<To>(FromOutput.GetDescriptor(FromIndex));
					}
				}
			}

			ToOutput.Revisions[ToIndex] = FromOutput.Revisions[FromIndex];
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	/* this function moves a Handle between the InputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameInputToInput(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToInput(To(FromInput.GetHandle()), {});
			if constexpr (s.template ContainsInput<To>())
			{
				//copy over the old revisions to not loose entries
				const auto& Destination = s.template GetWrappedInput<To>();
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					ToInput.Revisions[i] = Destination.Revisions[i];
				}
			}
			else
			{
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToInput.Revisions[i].ImaginaryResource = To::template OnCreateInput<To>(FromInput.GetDescriptor(FromIndex));
					}
				}
			}

			ToInput.Revisions[ToIndex] = FromInput.Revisions[FromIndex];
			InputTableType<To> DestTable{ InputTable<To>(ToInput), OutputTable<>() };
			return s.Union(DestTable);
		};
	}

	// TODO usage of this function might be unsafe, use with care 
	/* this function moves a Handle from the InputList into the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameInputToOutput(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");
	
			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToOutput(To(FromInput.GetHandle()), {});
			if constexpr (s.template ContainsOutput<To>())
			{
				//copy over the old revisions to not loose entries
				const auto& Destination = s.template GetWrappedOutput<To>();
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					ToOutput.Revisions[i] = Destination.Revisions[i];
				}
			}
			else
			{
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToOutput.Revisions[i].ImaginaryResource = To::template OnCreateInput<To>(FromInput.GetDescriptor(FromIndex));
					}
				}
			}

			ToOutput.Revisions[ToIndex] = FromInput.Revisions[FromIndex];
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	/* this function moves a Handle from the OutputList into the InputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameOutputToInput(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::OutputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromOutput = s.template GetWrappedOutput<From>();
			OutputTableType<From> SourceTable{ InputTable<>(), OutputTable<From>(FromOutput) };

			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToInput(To(FromOutput.GetHandle()), {});
			if constexpr (s.template ContainsInput<To>())
			{
				//copy over the old revisions to not loose entries
				const auto& Destination = s.template GetWrappedInput<To>();
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					ToInput.Revisions[i] = Destination.Revisions[i];
				}
			}
			else
			{
				for (U32 i = 0; i < To::ResourceCount; i++)
				{
					if (i != ToIndex)
					{
						//create undefined dummy resources with the same descriptor
						ToInput.Revisions[i].ImaginaryResource = To::template OnCreateInput<To>(FromOutput.GetDescriptor(FromIndex));
					}
				}
			}

			ToInput.Revisions[ToIndex] = FromOutput.Revisions[FromIndex];
			InputTableType<To> DestTable{ InputTable<To>(ToInput), OutputTable<>() };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	/* this function moves all Handles from the InputList into the OutputList the destination is overwitten and the Source stays */
	template<typename From, typename To>
	auto RenameEntireInputsToOutputs() const
	{
		static_assert(From::ResourceCount == To::ResourceCount, "ResourceCounts must match");
		return [](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			//make a new destination and use the conversion constructor to check if the conversion is valid
			Wrapped<To> ToOutput(To(FromInput.GetHandle()), {});

			for(U32 i = 0; i < To::ResourceCount; i++)
			{
				ToOutput.Revisions[i] = FromInput.Revisions[i];
			}
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	/* this function adds a new input to the resourcetable all descriptors have to be provided */ 
	template<typename Handle, typename... ARGS>
	auto CreateInputResource(const typename Handle::DescriptorType(&InDescriptors)[Handle::ResourceCount], const ARGS&... Args) const
	{
		ResourceRevision Revisions[Handle::ResourceCount];
		for (U32 i = 0; i < Handle::ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template OnCreateInput<Handle>(InDescriptors[i]);
		}
		auto WrappedResource = Wrapped<Handle>(Handle(Args...), Revisions);

		return [WrappedResource](const auto& s)
		{	
			CheckIsResourceTable(s);
			auto NewResourceTable = InputTableType<Handle>(InputTable<Handle>(WrappedResource), OutputTable<>());
			return s.Union(NewResourceTable);
		};
	}

	/* this function adds a new output to the resourcetable all descriptors have to be provided */ 
	template<typename Handle, typename... ARGS>
	auto CreateOutputResource(const typename Handle::DescriptorType(&InDescriptors)[Handle::ResourceCount], const ARGS&... Args) const
	{
		ResourceRevision Revisions[Handle::ResourceCount];
		for (U32 i = 0; i < Handle::ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template OnCreateOutput<Handle>(InDescriptors[i]);
		}
		auto WrappedResource = Wrapped<Handle>(Handle(Args...), Revisions);

		return [WrappedResource](const auto& s)
		{	
			CheckIsResourceTable(s);
			auto NewResourceTable = OutputTableType<Handle>(InputTable<>(), OutputTable<Handle>(WrappedResource));
			return s.Union(NewResourceTable);
		};
	}

	/* returns the empty resourcetable */
	static inline auto GetEmptyResourceTable()
	{
		return ResourceTable<InputTable<>, OutputTable<>>(InputTable<>(), OutputTable<>());
	}

	/* after the builing finished return all the actions recorded */
	const std::vector<const IRenderPassAction*>& GetActionList() //this should never be const
	{
		return ActionList;
	}

	void Reset()
	{
		ActionList.clear();
	}

private:
	template<typename Handle>
	using OutputTableType = ResourceTable<InputTable<>, OutputTable<Handle>>;

	template<typename Handle>
	using InputTableType = ResourceTable<InputTable<Handle>, OutputTable<>>;

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

		ItterableResourceTable<RenderPassDataType> RenderPassData;
		FunctionType Task;
	};

	mutable std::vector<const IRenderPassAction*> ActionList;
};
