#pragma once
#include "Set.h"

#include <memory>
#include <vector>
#include <utility>
#include "Concepts.h"
#include "Types.h"
#include "Assert.h"
#include "LinearAlloc.h"

template<typename Handle>
class TransientResourceImpl;

struct ResourceHandle
{
	static constexpr const U32 ResourceCount = 1;
	
	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateInput(const typename Handle::DescriptorType& InDescriptor) 
	{
		return Create<Handle>(InDescriptor);
	}

	template<typename Handle>
	static TransientResourceImpl<Handle>* CreateOutput(const typename Handle::DescriptorType& InDescriptor)
	{
		return Create<Handle>(InDescriptor);
	}

private:
	template<typename Handle>
	static TransientResourceImpl<Handle>* Create(const typename Handle::DescriptorType& InDescriptor);
};

namespace EResourceFlags
{
	enum Enum
	{
		Discard  = 0,
		Managed  = 1 << 0,
		External = 1 << 1,
		Default = Managed,
	};

	struct Type : SafeEnum<Enum, Type>
	{
		Type() : SafeEnum(Default) {}
		Type(const Enum& e) : SafeEnum(e) {}
	};
};

class MaterializedResource 
{
	EResourceFlags::Type ResourceFlags = EResourceFlags::Default;
	
protected:
	MaterializedResource(EResourceFlags::Type InResourceFlags) : ResourceFlags(InResourceFlags) {}

public:
	bool IsDiscardedResource() const { return ResourceFlags == EResourceFlags::Discard; };
	bool IsManagedResource() const { return All(EResourceFlags::Managed, ResourceFlags); };
	bool IsExternalResource() const { return All(EResourceFlags::External, ResourceFlags); };
};

class TransientResource
{
protected:
	mutable MaterializedResource* Resource = nullptr;

public:
	virtual void Materialize() const = 0;

	bool IsMaterialized() const { return Resource != nullptr; }
	const MaterializedResource* GetResource() const { return Resource; }
};

template<typename Handle>
class TransientResourceImpl : public TransientResource
{
	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;

	template<typename>
	friend struct Wrapped;

public:
	DescriptorType Descriptor;

	virtual void Materialize() const
	{
		if (Resource == nullptr)
		{
			Resource = Handle::Materialize(Descriptor);
		}
	}

public:
	TransientResourceImpl(const DescriptorType& InDescriptor) : Descriptor(InDescriptor) {}
};

template <typename Handle>
inline TransientResourceImpl<Handle>* ResourceHandle::Create(const typename Handle::DescriptorType& InDescriptor)
{
	return LinearNew<TransientResourceImpl<Handle>>(InDescriptor);
}

class IResourceTableInfo;
struct ResourceRevision
{
	const TransientResource* ImaginaryResource = nullptr;
	const IResourceTableInfo* Parent = nullptr;

	ResourceRevision(const TransientResource* InImaginaryResource = nullptr) : ImaginaryResource(InImaginaryResource) {}

	bool operator==(ResourceRevision Other) const
	{
		if (ImaginaryResource != Other.ImaginaryResource)
		{
			return false;
		}
		return Parent == Other.Parent;
	}

	bool operator!=(ResourceRevision Other) const
	{
		return!(*this == Other);
	}
};

template<typename Handle>
struct Wrapped : private Handle
{
	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;
	static constexpr U32 ResourceCount = Handle::ResourceCount;
	typedef ResourceRevision RevisionArray[ResourceCount];

	template<typename, int, typename...>
	friend class ResourceTableIterator;

	friend struct RenderPassBuilder;

	Wrapped() = default;

	Wrapped(const Handle& handle, const RevisionArray& InRevisions)
		: Handle(handle)
	{ 
		static_assert(std::is_base_of_v<ResourceHandle, Handle>, "Handles must derive from ResourceHandle to work"); 

		for (U32 i = 0; i < ResourceCount; i++)
		{
			Revisions[i] = InRevisions[i];
		}
	}

	constexpr void Link(const Wrapped<Handle>& Source, const IResourceTableInfo* Parent)
	{
		check(AllocContains(Parent));
		for (U32 i = 0; i < ResourceCount; i++)
		{
			check(Revisions[i].ImaginaryResource == Source.Revisions[i].ImaginaryResource);
			Revisions[i].Parent = Parent;		
		}
	}

	const Handle& GetHandle() const
	{
		return *this;
	}

	const DescriptorType& GetDescriptor(U32 i = 0) const
	{
		check(i < ResourceCount && Revisions[i].ImaginaryResource != nullptr);
		return static_cast<const TransientResourceImpl<Handle>*>(Revisions[i].ImaginaryResource)->Descriptor;
	}

	constexpr bool IsValid(U32 i = 0) const
	{
		check(i < ResourceCount);
		return Revisions[i].ImaginaryResource != nullptr;
	}

private:
	RevisionArray Revisions;
};

struct ResourceTableEntry
{
	ResourceTableEntry(const ResourceTableEntry& Entry)
		: Revision(Entry.Revision), Owner(Entry.Owner), Name(Entry.Name)
	{}

	ResourceTableEntry(const ResourceRevision& InRevision, const IResourceTableInfo* InOwner, const char* InName)
		: Revision(InRevision), Owner(InOwner), Name(InName)
	{}

	bool operator==(ResourceTableEntry Other) const
	{
		return (Owner == Other.Owner) && (Revision == Other.Revision);
	}

	bool operator!=(ResourceTableEntry Other) const
	{
		return!(*this == Other);
	}

	const TransientResource* GetImaginaryResource() const
	{
		return Revision.ImaginaryResource;
	}

	const IResourceTableInfo* GetParent() const
	{
		return Revision.Parent;
	}

	const IResourceTableInfo* GetOwner() const
	{
		return Owner;
	}

	bool IsValid() const
	{
		return Revision.ImaginaryResource != nullptr;
	}
	
	bool IsMaterialized() const
	{
		return Revision.ImaginaryResource && Revision.ImaginaryResource->IsMaterialized();
	}

	void Materialize() const
	{
		check(Revision.ImaginaryResource != nullptr);
		Revision.ImaginaryResource->Materialize();
	}

	const char* GetName() const
	{
		return Name;
	}

private:
	ResourceRevision Revision;
	const IResourceTableInfo* Owner = nullptr;
	const char* Name = nullptr;

public:
	UintPtr ParentHash() const
	{
		return ((reinterpret_cast<UintPtr>(Revision.ImaginaryResource) * 19) 
			  + (reinterpret_cast<UintPtr>(Revision.Parent) * 7));
	}

	UintPtr Hash() const
	{
		return ((reinterpret_cast<UintPtr>(Revision.ImaginaryResource) * 19)
			  + (reinterpret_cast<UintPtr>(Owner) * 7));
	}

	bool IsExternal() const
	{
		return IsMaterialized() && Revision.ImaginaryResource->GetResource()->IsExternalResource();
	}

	bool IsRelatedTo(const ResourceTableEntry& Other) const
	{
		return Revision.ImaginaryResource == Other.Revision.ImaginaryResource;
	}
};

class IResourceTableIterator
{
protected:
	const void* TablePtr;
	const ResourceTableEntry Entry;
	IResourceTableIterator(const void* InTablePtr, const ResourceTableEntry& InEntry) : TablePtr(InTablePtr), Entry(InEntry)
	{}
	
public:
	virtual IResourceTableIterator* Next() = 0;

public:
	bool Equals(const IResourceTableIterator* Other) const
	{
		if (TablePtr != Other->TablePtr)
		{
			return false;
		}

		if (Entry != Other->Entry)
		{
			return false;
		}

		return true;
	}

	const ResourceTableEntry& Get() const
	{
		return Entry;
	}
};

template<typename TableType, int Index, typename... XS>
class ResourceTableIterator;

template<typename TableType, int Index, typename X, typename... XS>
class ResourceTableIterator<TableType, Index, X, XS...> : public IResourceTableIterator
{
	static ResourceTableEntry Get(const TableType* TablePtr, const IResourceTableInfo* Owner)
	{
		const Wrapped<X>& Wrap = TablePtr->template GetWrapped<X>();
		return ResourceTableEntry(Wrap.Revisions[Index], Owner, X::Name);
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo* Owner) : IResourceTableIterator(InTablePtr, Get(InTablePtr, Owner))
	{}

	IResourceTableIterator* Next() override
	{
		if constexpr (Index + 1 < X::ResourceCount)
		{
			static_assert(sizeof(ResourceTableIterator<TableType, Index + 1, X, XS...>) == sizeof(ResourceTableIterator<TableType, Index, X, XS...>), "Size don't Match for inplace storage");
			new (this) ResourceTableIterator<TableType, Index + 1, X, XS...>(reinterpret_cast<const TableType*>(TablePtr), Entry.GetOwner());
			return this;
		}
		else
		{
			static_assert(sizeof(ResourceTableIterator<TableType, 0, XS...>) == sizeof(ResourceTableIterator<TableType, Index, X, XS...>), "Size don't Match for inplace storage");
			new (this) ResourceTableIterator<TableType, 0, XS...>(reinterpret_cast<const TableType*>(TablePtr), Entry.GetOwner());
			return this;
		}
	}
};

template<typename TableType>
class ResourceTableIterator<TableType, 0> : public IResourceTableIterator
{
	static ResourceTableEntry Get()
	{
		return ResourceTableEntry(ResourceRevision(nullptr), nullptr, nullptr);
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo*) : IResourceTableIterator(InTablePtr, Get())
	{}

	IResourceTableIterator* Next() override
	{
		new (this) ResourceTableIterator<TableType, 0>(reinterpret_cast<const TableType*>(TablePtr), nullptr);
		return this;
	}
};

struct IRenderPassAction;
class IResourceTableInfo
{
	friend struct RenderPassBuilder;

public:
	class Iterator
	{
		typedef size_t difference_type;
		typedef ResourceTableEntry value_type;
		typedef const ResourceTableEntry* pointer;
		typedef const ResourceTableEntry& reference;
		typedef std::input_iterator_tag iterator_category;

		char* ItteratorStorage[sizeof(ResourceTableIterator<void, 0>)];

	public:
		Iterator& operator++() 
		{ 
			reinterpret_cast<IResourceTableIterator*>(ItteratorStorage)->Next();
			return *this;
		}

		bool operator==(const Iterator& Other) const 
		{ 
			return reinterpret_cast<const IResourceTableIterator*>(ItteratorStorage)->Equals(reinterpret_cast<const IResourceTableIterator*>(Other.ItteratorStorage));
		}

		bool operator!=(const Iterator& Other) const
		{ 
			return !reinterpret_cast<const IResourceTableIterator*>(ItteratorStorage)->Equals(reinterpret_cast<const IResourceTableIterator*>(Other.ItteratorStorage));
		}

		reference operator*() const
		{ 
			return reinterpret_cast<const IResourceTableIterator*>(ItteratorStorage)->Get();
		}

		template<typename TableType, typename... TS>
		static Iterator MakeIterator(const TableType* InTableType, const IResourceTableInfo* Owner)
		{
			Iterator RetVal;
			static_assert(sizeof(ResourceTableIterator<TableType, 0, TS...>) == sizeof(ResourceTableIterator<void, 0>), "Size don't Match for inplace storage");
			new (RetVal.ItteratorStorage) ResourceTableIterator<TableType, 0, TS...>(InTableType, Owner);
			return RetVal;
		}
	};

	class Iteratable
	{
	public:
		virtual IResourceTableInfo::Iterator begin() const = 0;
		virtual IResourceTableInfo::Iterator end() const = 0;
	};

	IResourceTableInfo(const char* InName, const IRenderPassAction* InAction) : Name(InName), Action(InAction) {}
	virtual ~IResourceTableInfo() {}
	virtual const Iteratable& AsInputIterator() const = 0;
	virtual const Iteratable& AsOutputIterator() const = 0;

	const char* GetName() const
	{
		return Name;
	}

	const IRenderPassAction* GetAction() const
	{
		return Action;
	}

private:
	const char* Name = nullptr;
	const IRenderPassAction* Action = nullptr;
};

template<template<typename...> class Derived, typename... TS>
class BaseTable : Wrapped<TS>...
{
	typedef BaseTable<Derived, TS...> ThisType;
public:
	IResourceTableInfo::Iterator begin(const IResourceTableInfo* Owner) const
	{
		return IResourceTableInfo::Iterator::MakeIterator<ThisType, TS...>(this, Owner);
	};

	IResourceTableInfo::Iterator end() const
	{
		return IResourceTableInfo::Iterator::MakeIterator<ThisType>(this, nullptr);
	};

public:
	template<template<typename...> class, typename...>
	friend class BaseTable;

	BaseTable(const Wrapped<TS>&... xs) : Wrapped<TS>(xs)... {}

	static constexpr auto GetSetType() { return Set::Type<TS...>(); };
	static constexpr size_t GetSetSize() { return sizeof...(TS); };

	template<typename C>
	static constexpr bool Contains() 
	{ 
		return std::is_base_of_v<Wrapped<C>, ThisType>;
	}

	template<template<typename...> class TableType, typename... YS>
	constexpr auto Union(const TableType<YS...>& Other) const
	{
		return MergeToLeft(Set::Union(GetSetType(), Set::Type<YS...>()), *this, Other);
	}

	template<template<typename...> class TableType, typename... YS>
	constexpr auto Intersect(const TableType<YS...>& Other) const
	{
		return MergeToLeft(Set::Intersect(GetSetType(), Set::Type<YS...>()), *this, Other);
	}

	template<template<typename...> class TableType, typename... YS>
	constexpr auto Difference(const TableType<YS...>& Other) const
	{
		return MergeToLeft(Set::Difference(GetSetType(), Set::Type<YS...>()), *this, Other);
	}

	template<template<typename...> class TableType, typename... YS>
	static constexpr auto Collect(const TableType<YS...>& Other)
	{
		return CollectInternal(GetSetType(), Other);
	}

	template<typename Handle>
	const Wrapped<Handle>& GetWrapped() const { return static_cast<const Wrapped<Handle>&>(*this); }

protected:
	template<typename Handle>
	Wrapped<Handle>& GetWrapped() { return static_cast<Wrapped<Handle>&>(*this); }

private:
	template<typename X, typename... XS, template<typename...> class TypeY, typename... YS, template<typename...> class TypeZ, typename... ZS, typename... ARGS>
	static constexpr auto MergeToLeft(const Set::Type<X, XS...>&, const BaseTable<TypeY, YS...>& Lhs, const BaseTable<TypeZ, ZS...>& Rhs, const Wrapped<ARGS>&... Args)
	{
		if constexpr (BaseTable<TypeZ, ZS...>::template Contains<X>())
			return MergeToLeft(Set::Type<XS...>(), Lhs, Rhs, Args..., Rhs.template GetWrapped<X>());
		else
			return MergeToLeft(Set::Type<XS...>(), Lhs, Rhs, Args..., Lhs.template GetWrapped<X>());
	}

	template<template<typename...> class TypeY, typename... YS, template<typename...> class TypeZ, typename... ZS, typename... ARGS>
	static constexpr auto MergeToLeft(const Set::Type<>&, const BaseTable<TypeY, YS...>&, const BaseTable<TypeZ, ZS...>&, const Wrapped<ARGS>&... Args)
	{
		return TypeY<ARGS...>(Args...);
	}

	template<typename X, typename... XS, template<typename...> class TypeZ, typename... ZS, typename... ARGS>
	static constexpr auto CollectInternal(const Set::Type<X, XS...>&, const BaseTable<TypeZ, ZS...>& Rhs, const Wrapped<ARGS>&... Args)
	{
		if constexpr (BaseTable<TypeZ, ZS...>::template Contains<X>())
			return CollectInternal(Set::Type<XS...>(), Rhs, Args..., Rhs.template GetWrapped<X>());
		else
			return CollectInternal(Set::Type<XS...>(), Rhs, Args...);
	}

	template<template<typename...> class TypeZ, typename... ZS, typename... ARGS>
	static constexpr auto CollectInternal(const Set::Type<>&, const BaseTable<TypeZ, ZS...>&, const Wrapped<ARGS>&... Args)
	{
		//static_assert(std::is_same_v<Derived<TS...>, Derived<ARGS...>>, "could not collect all parameters");
		return Derived<ARGS...>(Args...);
	}
};

template<typename... XS>
class InputTable : private BaseTable<InputTable, XS...>
{
public:
	typedef BaseTable<::InputTable, XS...> BaseType;
	template<template<typename...> class, typename...>
	friend class BaseTable;

	friend struct RenderPassBuilder;

	using BaseType::begin;
	using BaseType::end;
	using BaseType::GetSetType;
	using BaseType::GetSetSize;
	using BaseType::Contains;
	using BaseType::Union;
	using BaseType::Intersect;
	using BaseType::Difference;
	using BaseType::Collect;
	using BaseType::GetWrapped;

	static constexpr bool IsInputTable() { return true; }
	static constexpr bool IsOutputTable() { return false; }

private:
	InputTable(const Wrapped<XS>&... xs) : BaseType(xs...) {}
};

template<typename... XS>
class OutputTable : private BaseTable<OutputTable, XS...>
{
public:
	typedef BaseTable<::OutputTable, XS...> BaseType;
	template<template<typename...> class, typename...>
	friend class BaseTable;

	friend struct RenderPassBuilder;

	using BaseType::begin;
	using BaseType::end;
	using BaseType::GetSetType;
	using BaseType::GetSetSize;
	using BaseType::Contains;
	using BaseType::Union;
	using BaseType::Intersect;
	using BaseType::Difference;
	using BaseType::Collect;
	using BaseType::GetWrapped;

	static constexpr bool IsInputTable() { return false; }
	static constexpr bool IsOutputTable() { return true; }

	template<typename... TS>
	constexpr void Link(const OutputTable<TS...>& Source, const IResourceTableInfo* Parent)
	{
		typedef decltype(Set::Intersect(Set::Type<XS...>(), Set::Type<TS...>())) IntersectionSet;
		LinkInternal(IntersectionSet(), Source, Parent);
	}

private:
	OutputTable(const Wrapped<XS>&... xs) : BaseType(xs...) {}

	template<typename T, typename... TS, typename... YS>
	constexpr void LinkInternal(const Set::Type<T, TS...>&, const OutputTable<YS...>& Source, const IResourceTableInfo* Parent)
	{
		this->template GetWrapped<T>().Link(Source.template GetWrapped<T>(), Parent);
		LinkInternal(Set::Type<TS...>(), Source, Parent);
	}

	template<typename... ZS>
	constexpr void LinkInternal(const Set::Type<>&, const OutputTable<ZS...>&, const IResourceTableInfo*) {}
};

template<typename TInputTableType, typename TOutputTableType>
class ResourceTable final : private TInputTableType, private TOutputTableType, public IResourceTableInfo
{
public:
	template<typename, typename>
	friend class ResourceTable;

	friend struct RenderPassBuilder;

	typedef TInputTableType InputTableType;
	typedef TOutputTableType OutputTableType;
	typedef ResourceTable<InputTableType, OutputTableType> ThisType;

private:
	template<typename T>
	class IteratableImpl : public IResourceTableInfo::Iteratable
	{
		template<typename, typename>
		friend class ResourceTable;

		ThisType* ResourceTable;
	public:
		IteratableImpl(ThisType* InResourceTable) : ResourceTable(InResourceTable) {}

		IteratableImpl& operator= (const IteratableImpl& Other)
		{
			//MSVC Compiler bug: the default implementation is mising this->
			this->ResourceTable = Other.ResourceTable;
			return *this;
		}

		virtual IResourceTableInfo::Iterator begin() const
		{
			return static_cast<const T&>(*ResourceTable).begin(ResourceTable);
		}

		virtual IResourceTableInfo::Iterator end() const
		{
			return static_cast<const T&>(*ResourceTable).end();
		}
	};

	IteratableImpl<InputTableType> InputIterator;
	IteratableImpl<OutputTableType> OutputIterator;

public:
	const IResourceTableInfo::Iteratable& AsInputIterator() const override
	{
		check(InputIterator.ResourceTable == this);
		return InputIterator;
	}

	const IResourceTableInfo::Iteratable& AsOutputIterator() const override
	{
		check(OutputIterator.ResourceTable == this);
		return OutputIterator;
	}

public:
	ResourceTable(const ThisType& RTT)
		: ResourceTable(RTT, RTT.GetName(), RTT.GetAction()) {};

	explicit ResourceTable(const ThisType& RTT, const char* Name, const IRenderPassAction* InAction)
		: ResourceTable(RTT.GetInputTable(), RTT.GetOutputTable(), Name, InAction) {};

	explicit ResourceTable(const InputTableType& IT, const OutputTableType& OT, const char* Name = "Unnamed", const IRenderPassAction* InAction = nullptr)
		: InputTableType(IT)
		, OutputTableType(OT) 
		, IResourceTableInfo(Name, InAction)
		, InputIterator(this)
		, OutputIterator(this)
	{ 
		CheckIntegrity(); 
	};

	template<typename ITT2, typename OTT2>
	ResourceTable(const ResourceTable<ITT2, OTT2>& RTT) : ResourceTable(RTT.template Populate<ThisType>())
	{
	}

	static constexpr auto GetInputSetType() { return InputTableType::GetSetType(); };
	static constexpr auto GetOutputSetType() { return OutputTableType::GetSetType(); };
	static constexpr size_t GetInputSetSize() { return InputTableType::GetSetSize(); };
	static constexpr size_t GetOutputSetSize() { return OutputTableType::GetSetSize(); };

	template<typename C>
	static constexpr bool ContainsInput() { return InputTableType::template Contains<C>(); }

	template<typename C>
	static constexpr bool ContainsOutput() { return OutputTableType::template Contains<C>(); }

	template<typename ITT, typename OTT>
	constexpr auto Union(const ResourceTable<ITT, OTT>& Other) const
	{
		auto in = GetInputTable().Union(Other.GetInputTable());
		auto out = GetOutputTable().Union(Other.GetOutputTable());
		return ResourceTable<decltype(in), decltype(out)>(in, out);
	}

	template<typename ITT, typename OTT>
	constexpr auto Intersect(const ResourceTable<ITT, OTT>& Other) const
	{
		auto in = GetInputTable().Intersect(Other.GetInputTable());
		auto out = GetOutputTable().Intersect(Other.GetOutputTable());
		return ResourceTable<decltype(in), decltype(out)>(in, out);
	}

	template<typename ITT, typename OTT>
	constexpr auto Difference(const ResourceTable<ITT, OTT>& Other) const
	{
		auto in = GetInputTable().Difference(Other.GetInputTable());
		auto out = GetOutputTable().Difference(Other.GetOutputTable());
		return ResourceTable<decltype(in), decltype(out)>(in, out);
	}

	template<typename Handle>
	constexpr bool IsValidInput(U32 i = 0) const
	{
		return GetWrappedInput<Handle>().IsValid(i);
	}

	template<typename Handle>
	constexpr bool IsValidOutput(U32 i = 0) const
	{
		return GetWrappedOutput<Handle>().IsValid(i);
	}

	template<typename Handle>
	const auto& GetInputHandle() const
	{
		return GetWrappedInput<Handle>().GetHandle();
	}

	template<typename Handle>
	const auto& GetInputDescriptor(U32 i = 0) const
	{
		return GetWrappedInput<Handle>().GetDescriptor(i);
	}

	template<typename Handle>
	const auto& GetOutputDescriptor(U32 i = 0) const
	{
		return GetWrappedOutput<Handle>().GetDescriptor(i);
	}

private:
	template<typename ITT, typename OTT>
	constexpr auto Merge(const ResourceTable<ITT, OTT>& Parent) const
	{
		auto mergedOutput = Parent.GetOutputTable().Union(GetOutputTable());
		return ResourceTable<ITT, decltype(mergedOutput)>(Parent.GetInputTable(), mergedOutput);
	}

	template<typename ITT, typename OTT>
	constexpr auto MergeAndLink(const ResourceTable<ITT, OTT>& Parent) const
	{
		auto mergedOutput = Parent.GetOutputTable().Union(GetOutputTable());
		mergedOutput.Link(GetOutputTable(), this);
		return ResourceTable<ITT, decltype(mergedOutput)>(Parent.GetInputTable(), mergedOutput);
	}

	template<typename RTT, typename ITT = typename RTT::InputTableType, typename OTT = typename RTT::OutputTableType>
	constexpr ResourceTable<ITT, OTT> Populate() const
	{
		RTT::CheckIntegrity();
		auto in = ITT::Collect(GetInputTable().Union(GetOutputTable()));
		auto out = OTT::Collect(GetOutputTable());

		constexpr bool assignable = std::is_same_v<ResourceTable<ITT, OTT>, ResourceTable<decltype(in), decltype(out)>>;
		if constexpr(!assignable)
		{
			using DiffTable = ResourceTable<decltype(in.Difference(std::declval<ITT>())), decltype(out.Difference(std::declval<OTT>()))>;
			struct ErrorType {} Error;
			DiffTable Table(Error);
			static_assert(assignable, "missing resourcetable entry: cannot assign");
			return Table;
		}

		return ResourceTable<ITT, OTT>(in, out);
	}

	template<typename Handle>
	const auto& GetWrappedInput() const
	{
		return GetInputTable().template GetWrapped<Handle>();
	}

	template<typename Handle>
	const auto& GetWrappedOutput() const
	{
		return GetOutputTable().template GetWrapped<Handle>();
	}

	const InputTableType& GetInputTable() const { return static_cast<const InputTableType&> (*this); }
	const OutputTableType& GetOutputTable() const { return static_cast<const OutputTableType&> (*this); }
	OutputTableType& GetOutputTable() { return static_cast<OutputTableType&> (*this); }

	static constexpr void CheckIntegrity()
	{
		static_assert(InputTableType::IsInputTable(), "First Parameter must be the InputTable");
		static_assert(OutputTableType::IsOutputTable(), "Second Parameter must be the OutputTable");
	}

	template<typename... XS, typename... YS>
	static constexpr auto TableTypeFromSets(const Set::Type<XS...>&, const Set::Type<YS...>&) -> ResourceTable<InputTable<XS...>, OutputTable<YS...>>;

	template<typename InputSet, typename OutputSet, typename InputOutputSet = decltype(Set::Intersect(InputSet(), OutputSet()))>
	static constexpr auto ExtractInputOutputTableType(const InputSet&, const OutputSet&) -> decltype(TableTypeFromSets(InputSet(), InputOutputSet()));
 
public:
	using PassOutputType = ThisType;
	using PassInputType = decltype(ExtractInputOutputTableType(InputTableType::GetSetType(), OutputTableType::GetSetType()));
};

#define RESOURCE_TABLE(...)														\
	using ResourceTableType	= ResourceTable< __VA_ARGS__ >;						\
	using PassInputType		= typename ResourceTableType::PassInputType;		\
	using PassOutputType	= typename ResourceTableType::PassOutputType;		
