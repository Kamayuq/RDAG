#pragma once
#include <mutex>
#include "LinearAlloc.h"
#include "Continuation.h"
#include "Sequence.h"
#include "Plumber.h"
#include "RHI.h"

extern std::mutex ActionListMutex;

struct IRenderPassAction
{
	IRenderPassAction(const char* InName) : Name(InName) {}

	virtual ~IRenderPassAction() {}
	virtual const class IResourceTableInfo* GetRenderPassData() const = 0;
	virtual void Execute(struct RenderContext&) const {};
	virtual void Execute(struct ParallelRenderContext&) const {};
	virtual void Execute(struct VulkanRenderContext&) const {};

	const char* GetName() const { return Name; };
	void SetColor(U32 InColor) const { Color = InColor; }
	U32 GetColor() const { return Color; }
private:
	const char* Name = nullptr;
	mutable U32 Color = UINT_MAX;
};

template<typename CRTP, typename ASYNC>
struct RenderPassBuilderBase
{
	template<typename, typename>
	friend struct RenderPassBuilderBase;

	RenderPassBuilderBase(const RenderPassBuilderBase&) = delete;
	RenderPassBuilderBase(std::vector<const IRenderPassAction*>& InActionList)
		: ActionList(InActionList)
	{}

	template<typename FunctionType, typename ReturnType = typename std::decay_t<typename Traits::function_traits<FunctionType>::return_type>::ReturnType>
	auto BuildAsyncRenderPass(const char* Name, const FunctionType& BuildFunction, Promise<ReturnType>& Promise) const
	{
		typedef Traits::function_traits<FunctionType> Traits;
		typedef std::decay_t<typename Traits::template arg<0>::type> BuilderType;
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		typedef std::decay_t<typename Traits::return_type> PromiseType;
		static_assert(Traits::arity == 2, "Build Functions have 2 parameters: the builder and the input table");
		static_assert(std::is_base_of_v<PromiseBase, PromiseType>, "The returntype must be a Promise");
		static_assert(std::is_base_of_v<IResourceTableInfo, InputTableType>, "The 2nd parameter must be a resource table");
		typedef PromiseType(*BuildFunctionType)(const BuilderType&, const InputTableType&);
		static_assert(std::is_assignable_v<BuildFunctionType&, FunctionType>, "Only global and static functions as well as lambdas without capture are allowed");
		typedef ReturnType NestedOutputTableType;

		auto& LocalActionList = ActionList;
		return [&LocalActionList, &Promise, BuildFunction, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			//typedef typename std::decay<decltype(s)>::type StateType;

			auto input = s.template PopulateInput<InputTableType>();
			NestedOutputTableType* NestedRenderPassData = LinearAlloc<NestedOutputTableType>();
			ASYNC Builder(LocalActionList, NestedRenderPassData);

			Promise = BuildFunction(Builder, input);
			Promise.Run(NestedRenderPassData, Name);

			return s;
		};
	}

	template<typename ReturnType>
	auto SynchronizeAsyncRenderPass(Promise<ReturnType>& Promise) const
	{
		return [&Promise](const auto& s)
		{
			CheckIsResourceTable(s);
			return Promise.Get().Merge(s);
		};
	}

	template<typename FunctionType>
	auto BuildRenderPass(const char* Name, const FunctionType& BuildFunction) const
	{
		typedef Traits::function_traits<FunctionType> Traits;
		typedef std::decay_t<typename Traits::template arg<0>::type> BuilderType;
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		typedef std::decay_t<typename Traits::return_type> NestedOutputTableType;
		static_assert(Traits::arity == 2, "Build Functions have 2 parameters: the builder and the input table");
		static_assert(std::is_base_of_v<IResourceTableInfo, NestedOutputTableType>, "The returntype must be a resource table");
		static_assert(std::is_base_of_v<IResourceTableInfo, InputTableType>, "The 2nd parameter must be a resource table");
		typedef NestedOutputTableType(*BuildFunctionType)(const BuilderType&, const InputTableType&);
		static_assert(std::is_assignable_v<BuildFunctionType&, FunctionType>, "Only global and static functions as well as lambdas without capture are allowed");

		auto& LocalActionList = ActionList;
		return [&LocalActionList, BuildFunction, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			//typedef typename std::decay<decltype(s)>::type StateType;

			auto input = s.template PopulateInput<InputTableType>();
			NestedOutputTableType* NestedRenderPassData = LinearAlloc<NestedOutputTableType>();

			CRTP Builder(LocalActionList);
			new (NestedRenderPassData) NestedOutputTableType(BuildFunction(Builder, input), Name, nullptr);
			return NestedRenderPassData->Merge(s);
		};
	}

	template<typename FunctionType>
	auto QueueRenderAction(const char* Name, const FunctionType& QueuedTask) const
	{
		typedef Traits::function_traits<FunctionType> Traits;
		typedef std::decay_t<typename Traits::template arg<0>::type> ContextType;
		typedef std::decay_t<typename Traits::template arg<1>::type> InputTableType;
		typedef std::decay_t<typename Traits::return_type> VoidReturnType;
		static_assert(Traits::arity == 2, "Queue Functions have 2 parameters: the rendercontext and the input table");
		static_assert(std::is_same_v<VoidReturnType, void>, "The returntype must be void");
		static_assert(std::is_base_of_v<RenderContextBase, ContextType>, "The 1st parameter must be a rendercontext type");
		static_assert(std::is_base_of_v<IResourceTableInfo, InputTableType>, "The 2nd parameter must be a resource table");
		typedef void(*QueueFunctionType)(ContextType&, const InputTableType&);
		static_assert(std::is_assignable_v<QueueFunctionType&, FunctionType>, "Only global and static functions as well as lambdas without capture are allowed");

		auto& LocalActionList = ActionList;
		return [&LocalActionList, QueuedTask, Name](const auto& s)
		{
			CheckIsResourceTable(s);
			//typedef typename std::decay<decltype(s)>::type StateType;

			typedef TRenderPassAction<ContextType, InputTableType, QueueFunctionType> RenderActionType;
			InputTableType input = s.template PopulateAll<InputTableType>();
			RenderActionType* NewRenderAction = LinearAlloc<RenderActionType>();

			new (NewRenderAction) RenderActionType(Name, input, QueuedTask);
			{
				std::lock_guard<std::mutex> lock(ActionListMutex);
				LocalActionList.push_back(NewRenderAction);
			}
			return NewRenderAction->RenderPassData.MergeAndLink(s);
		};
	}

	template<typename From, typename To>
	auto MoveOutputTableEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::OutputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromOutput = s.template GetWrappedOutput<From>();
			OutputTableType<From> SourceTable{ InputTable<>(), OutputTable<From>(FromOutput) };

			Wrapped<To> ToOutput(To(FromOutput.GetHandle()), {});
			if constexpr (s.template ContainsOutput<To>())
			{
				ToOutput = s.template GetWrappedOutput<To>();
			}

			ToOutput.Revisions[ToIndex] = FromOutput.Revisions[FromIndex];
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	template<typename From, typename To>
	auto MoveInputTableEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			Wrapped<To> ToInput(To(FromInput.GetHandle()), {});
			if constexpr (s.template ContainsInput<To>())
			{
				ToInput = s.template GetWrappedInput<To>();
			}

			ToInput.Revisions[ToIndex] = FromInput.Revisions[FromIndex];
			InputTableType<To> DestTable{ InputTable<To>(ToInput), OutputTable<>() };
			return s.Union(DestTable);
		};
	}

	template<typename From, typename To>
	auto MoveInputToOutputTableEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");
	
			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			Wrapped<To> ToOutput(To(FromInput.GetHandle()), {});
			if constexpr (s.template ContainsOutput<To>())
			{
				ToOutput = s.template GetWrappedOutput<To>();
			}
			
			ToOutput.Revisions[ToIndex] = FromInput.Revisions[FromIndex];
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	template<typename From, typename To>
	auto MoveOutputToInputTableEntry(U32 FromIndex = 0, U32 ToIndex = 0) const
	{
		return [FromIndex, ToIndex](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::OutputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromOutput = s.template GetWrappedOutput<From>();
			OutputTableType<From> SourceTable{ InputTable<>(), OutputTable<From>(FromOutput) };

			Wrapped<To> ToInput(To(FromOutput.GetHandle()), {});
			if constexpr (s.template ContainsInput<To>())
			{
				ToInput = s.template GetWrappedInput<To>();
			}

			ToInput.Revisions[ToIndex] = FromOutput.Revisions[FromIndex];
			InputTableType<To> DestTable{ InputTable<To>(ToInput), OutputTable<>() };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	template<typename From, typename To>
	auto MoveAllInputToOutputTableEntries() const
	{
		static_assert(From::ResourceCount == To::ResourceCount, "ResourceCounts must match");
		return [](const auto& s)
		{
			CheckIsResourceTable(s);
			typedef typename std::decay<decltype(s)>::type StateType;
			static_assert(std::is_base_of_v<From, typename StateType::InputTableType>, "Source was not found in the resource table");

			Wrapped<From> FromInput = s.template GetWrappedInput<From>();
			InputTableType<From> SourceTable{ InputTable<From>(FromInput), OutputTable<>() };

			Wrapped<To> ToOutput(To(FromInput.GetHandle()), {});
			if constexpr (s.template ContainsOutput<To>())
			{
				ToOutput = s.template GetWrappedOutput<To>();
			}

			for(U32 i = 0; i < To::ResourceCount; i++)
			{
				ToOutput.Revisions[i] = FromInput.Revisions[i];
			}
			OutputTableType<To> DestTable{ InputTable<>(), OutputTable<To>(ToOutput) };

			//remove the old output and copy it into the new destination
			return s.Union(DestTable);
		};
	}

	template<typename ResourceTableType>
	auto ExtractResourceTableEntries() const
	{
		return [](const auto& s)
		{
			CheckIsResourceTable(s);
			return s.template PopulateAll<ResourceTableType>();
		};
	}

	template<typename TInputTableType, typename TOutputTableType>
	auto ReplaceResourceTableEntries(const ResourceTable<TInputTableType, TOutputTableType>& table) const
	{
		return [table](const auto& s)
		{
			CheckIsResourceTable(s);
			return table;
		};
	}

	template<typename Handle, typename... ARGS>
	auto CreateInputResource(const typename Handle::DescriptorType(&InDescriptors)[Handle::ResourceCount], const ARGS&... Args) const
	{
		ResourceRevision Revisions[Handle::ResourceCount];
		for (U32 i = 0; i < Handle::ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template CreateInput<Handle>(InDescriptors[i]);
		}
		auto WrappedResource = Wrapped<Handle>(Handle(Args...), Revisions);

		return [WrappedResource](const auto& s)
		{	
			CheckIsResourceTable(s);
			auto NewResourceTable = InputTableType<Handle>(InputTable<Handle>(WrappedResource), OutputTable<>());
			return s.Union(NewResourceTable);
		};
	}

	template<typename Handle, typename... ARGS>
	auto CreateOutputResource(const typename Handle::DescriptorType(&InDescriptors)[Handle::ResourceCount], const ARGS&... Args) const
	{
		ResourceRevision Revisions[Handle::ResourceCount];
		for (U32 i = 0; i < Handle::ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template CreateOutput<Handle>(InDescriptors[i]);
		}
		auto WrappedResource = Wrapped<Handle>(Handle(Args...), Revisions);

		return [WrappedResource](const auto& s)
		{	
			CheckIsResourceTable(s);
			auto NewResourceTable = OutputTableType<Handle>(InputTable<>(), OutputTable<Handle>(WrappedResource));
			return s.Union(NewResourceTable);
		};
	}

	static inline auto GetEmptyResourceTable()
	{
		return ResourceTable<InputTable<>, OutputTable<>>(InputTable<>(), OutputTable<>(), "EmptyResourceTable");
	}

private:
	template<typename Handle>
	using OutputTableType = ResourceTable<InputTable<>, OutputTable<Handle>>;

	template<typename Handle>
	using InputTableType = ResourceTable<InputTable<Handle>, OutputTable<>>;

	static void CheckIsResourceTable(const IResourceTableInfo&)
	{
	}

	template<typename ContextType, typename RenderPassDataType, typename FunctionType>
	struct TRenderPassAction final : IRenderPassAction
	{
		template<typename, typename>
		friend struct RenderPassBuilderBase;

		TRenderPassAction(const char* Name, const RenderPassDataType& InRenderPassData, const FunctionType& InTask)
			: IRenderPassAction(Name)
			, RenderPassData(InRenderPassData, Name, this)
			, Task(InTask) {}

	private:
		const IResourceTableInfo* GetRenderPassData() const override
		{
			return &RenderPassData;
		}

		void Execute(ContextType& RndCtx) const override final
		{
			Task(RndCtx, RenderPassData);
		}

		RenderPassDataType RenderPassData;
		FunctionType Task;
	};

	std::vector<const IRenderPassAction*>& ActionList;
};

struct AsyncRenderPassBuilder : private RenderPassBuilderBase<AsyncRenderPassBuilder, AsyncRenderPassBuilder>
{
	typedef RenderPassBuilderBase<AsyncRenderPassBuilder, AsyncRenderPassBuilder> Base;
	using Base::RenderPassBuilderBase;
	using Base::BuildRenderPass;
	using Base::MoveAllInputToOutputTableEntries;
	using Base::MoveInputToOutputTableEntry;
	using Base::MoveInputTableEntry;
	using Base::MoveOutputToInputTableEntry;
	using Base::MoveOutputTableEntry;
	using Base::CreateInputResource;
	using Base::CreateOutputResource;
	using Base::ExtractResourceTableEntries;
	using Base::GetEmptyResourceTable;
	using Base::QueueRenderAction;
	using Base::ReplaceResourceTableEntries;
};

struct RenderPassBuilder : private RenderPassBuilderBase<RenderPassBuilder, AsyncRenderPassBuilder>
{
	typedef RenderPassBuilderBase<RenderPassBuilder, AsyncRenderPassBuilder> Base;
	using Base::RenderPassBuilderBase;
	using Base::BuildAsyncRenderPass;
	using Base::SynchronizeAsyncRenderPass;
	using Base::BuildRenderPass;
	using Base::MoveAllInputToOutputTableEntries;
	using Base::MoveInputToOutputTableEntry;
	using Base::MoveInputTableEntry;
	using Base::MoveOutputToInputTableEntry;
	using Base::MoveOutputTableEntry;
	using Base::CreateInputResource;
	using Base::CreateOutputResource;
	using Base::ExtractResourceTableEntries;
	using Base::GetEmptyResourceTable;
	using Base::QueueRenderAction;
	using Base::ReplaceResourceTableEntries;
};