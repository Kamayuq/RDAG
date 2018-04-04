#pragma once
#include "Set.h"
#include "Types.h"
#include "Assert.h"
#include "LinearAlloc.h"

#include <iterator>

/* Specialized Transient resource Implementation */
/* Handle is of ResourceHandle Type */
template<typename Handle>
class TransientResourceImpl;

/* Base of all ResourceHandles this class should never be used directly: use ResourceHandle instead */
struct ResourceHandleBase
{
	static constexpr const char* Name = nullptr;
	static constexpr const U32 ResourceCount = 0;

	/* Callback to specialize TransientResourceImpl creation from a Descriptor */
	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateInput(const typename Handle::DescriptorType& InDescriptor) 
	{
		return LinearNew<TransientResourceImpl<Handle>>(InDescriptor);
	}

	/* Callback to specialize TransientResourceImpl creation from a Descriptor */
	template<typename Handle>
	static TransientResourceImpl<Handle>* OnCreateOutput(const typename Handle::DescriptorType& InDescriptor)
	{
		return LinearNew<TransientResourceImpl<Handle>>(InDescriptor);
	}
};

/* A ResourceHande is used to implement and specialize your own Resources and callbacks */
template<typename CRTP>
struct ResourceHandle : ResourceHandleBase
{
	static constexpr const U32 ResourceCount = 1;
	/* Compatible types are used for automatic casting between each other */
	/* each set can only contain unique Compatible Types */
	using CompatibleType = CRTP;
};

/* Transient ResourceBase */
class TransientResource
{
protected:
	/* The type can be recovered by the TransientResourceImpl */
	mutable MaterializedResource* Resource = nullptr;

public:
	/* use the descriptor to create a resource and trigger OnMaterilize callback */
	virtual void Materialize() const = 0;

	bool IsMaterialized() const { return Resource != nullptr; }
	const MaterializedResource* GetResource() const { return Resource; }
};

/* Specialized Transient resource Implementation */
/* Handle is of ResourceHandle Type */
template<typename Handle>
class TransientResourceImpl : public TransientResource
{
	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;

	template<typename>
	friend struct Wrapped;

public:
	DescriptorType Descriptor;

	/* use the descriptor to create a resource and trigger OnMaterilize callback */
	void Materialize() const override
	{
		if (Resource == nullptr)
		{
			Resource = Handle::OnMaterialize(Descriptor);
		}
	}

public:
	TransientResourceImpl(const DescriptorType& InDescriptor) : Descriptor(InDescriptor) {}
};

class IResourceTableInfo;
/* the ResourceRevision stores the connectivity and History between transient resources in the graph */
struct ResourceRevision
{
	/* a link to it's underlying immaginary resource */
	const TransientResource* ImaginaryResource = nullptr;
	
	/* a link to the resourcetable set where this revision came from originally */
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

/* Wrapped resources share a common internal interface */ 
template<typename Handle>
struct Wrapped : private Handle
{
	/* Internalize properies of the Handle for convienence */
	using CompatibleType = typename Handle::CompatibleType;	
	using HandleType = Handle;
	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;
	static constexpr U32 ResourceCount = Handle::ResourceCount;
	
	/* each resource carries it's own revision */
	typedef ResourceRevision RevisionArray[ResourceCount];

	/* make friends with other classes as we are unrelated to each other */
	template<typename>
	friend struct Wrapped;

	template<typename, int, typename...>
	friend class ResourceTableIterator;

	friend struct RenderPassBuilder;

	template<typename, typename>
	friend class ResourceTable;

	Wrapped() = default;

	/* Wrapped resources need a handle and a resource array (which they could copy or compose from other handles) */
	Wrapped(const Handle& handle, const RevisionArray& InRevisions)
		: Handle(handle)
	{
		static_assert(std::is_base_of_v<ResourceHandleBase, Handle>, "Handles must derive from ResourceHandle to work"); 

		for (U32 i = 0; i < ResourceCount; i++)
		{
			Revisions[i] = InRevisions[i];
		}
	}

	/* When a pass action is queued we want to update the revision to point to this new action */
	template<typename SourceType>
	constexpr void Link(const Wrapped<SourceType>& Source, const IResourceTableInfo* Parent)
	{
		static_assert(std::is_same_v<typename SourceType::CompatibleType, CompatibleType>, "Incompatible Types during linking");
		check(AllocContains(Parent)); // make sure the parent is on the linear heap
		for (U32 i = 0; i < ResourceCount; i++)
		{
			//check that the set has not been tampered with between pass creation and linkage
			check(Revisions[i].ImaginaryResource == Source.Revisions[i].ImaginaryResource);
			
			//point to the new parent
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

	/* forward the OnExecute callback to the Handles implementation and all of it's Resources*/
	void OnExecute(struct ImmediateRenderContext& RndCtx) const
	{
		for (U32 i = 0; i < ResourceCount; i++)
		{
			if (IsValid(i) && Revisions[i].ImaginaryResource->IsMaterialized())
			{
				Handle::OnExecute(RndCtx, GetResource(i));
			}
		}
	}

	/* Handles should always be valid, invalid handles were a concept in the past 
	and should not be possible due to the strong type safty of the implementation */
	constexpr bool IsValid(U32 i = 0) const
	{
		check(i < ResourceCount);
		return Revisions[i].ImaginaryResource != nullptr;
	}

	/* Handles can get undefined when they never have been written to */
	constexpr bool IsUndefined(U32 i = 0) const
	{
		check(i < ResourceCount);
		return Revisions[i].ImaginaryResource == nullptr || Revisions[i].Parent == nullptr;
	}

	/* Convert Handles between each other */
	template<typename SourceType>
	static constexpr Wrapped<Handle> ConvertFrom(const Wrapped<SourceType>& Source)
	{
		// During casting of Handles try to call the conversion constructor otherwise fail 
		// if conversion is not allowed by the user
		return { Handle(Source.GetHandle()), Source.Revisions };
	}
	
private:
	const ResourceType& GetResource(U32 i = 0) const
	{
		check(IsValid(i) && Revisions[i].ImaginaryResource->IsMaterialized());
		return static_cast<const ResourceType&>(*Revisions[i].ImaginaryResource->GetResource());
	}

	template<bool B>
	static void Test()
	{
		static_assert(B, "ResourceHandleType could not be matched");
		if constexpr(!B)
		{
			bool fail = Handle(); (void)fail;
		}
	}

	RevisionArray Revisions;
};

/* A ResourceTableEntry is a temporary object for loop itteration, this allows generic access to some parts of the data */
struct ResourceTableEntry
{
	ResourceTableEntry(const ResourceTableEntry& Entry) = default;

	template<typename Handle> // only the static data of the Handle can be stored
	ResourceTableEntry(const ResourceRevision& InRevision, const IResourceTableInfo* InOwner, bool InIsInputTable, const Handle&)
		: Revision(InRevision), Owner(InOwner), Name(Handle::Name), IsInputTable(InIsInputTable)
	{}

	bool operator==(const ResourceTableEntry& Other) const
	{
		return (Owner == Other.Owner) && (Revision == Other.Revision);
	}

	bool operator!=(const ResourceTableEntry& Other) const
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
	
	bool IsUndefined() const
	{
		return Revision.ImaginaryResource == nullptr || (IsInputTable && Revision.Parent == nullptr);
	}

	bool IsMaterialized() const
	{
		return Revision.ImaginaryResource && Revision.ImaginaryResource->IsMaterialized();
	}

	/* Materialization from for-each loops starts here */
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
	//Graph connectivity information 
	ResourceRevision Revision;
	//the current owner
	const IResourceTableInfo* Owner = nullptr;
	//the name as given by the constexpr value of the Handle
	const char* Name = nullptr;

	bool IsInputTable = false;

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

	/* External resourcers are not managed by the graph and the user has to provide an implementation to retrieve the resource */
	bool IsExternal() const
	{
		return IsMaterialized() && Revision.ImaginaryResource->GetResource()->IsExternalResource();
	}

	/* A Resource is related when their immaginary resources match */
	bool IsRelatedTo(const ResourceTableEntry& Other) const
	{
		return Revision.ImaginaryResource == Other.Revision.ImaginaryResource;
	}
};

/* Itterator Base to itterate over ResourceTables with for-each*/
class IResourceTableIterator
{
protected:
	const void* TablePtr;
	const ResourceTableEntry Entry;
	IResourceTableIterator(const void* InTablePtr, const ResourceTableEntry& InEntry) : TablePtr(InTablePtr), Entry(InEntry)
	{}
	
public:
	/* move to the next element, can return the empty element */
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

	/* access the element the itterator currently point at */
	const ResourceTableEntry& Get() const
	{
		return Entry;
	}
};

/* Itterator Implementation to itterate over ResourceTables with for-each*/
template<typename TableType, int Index, typename... XS>
class ResourceTableIterator;

/* a non empty Itterator */
template<typename TableType, int Index, typename X, typename... XS>
class ResourceTableIterator<TableType, Index, X, XS...> : public IResourceTableIterator
{
	/* helper function to build a ResourceTableEntry for the base class */
	static ResourceTableEntry Get(const TableType* TablePtr, const IResourceTableInfo* Owner)
	{
		const Wrapped<X>& Wrap = TablePtr->template GetWrapped<X>();
		return ResourceTableEntry(Wrap.Revisions[Index], Owner, TableType::IsInputTable(), Wrap.GetHandle());
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo* Owner) : IResourceTableIterator(InTablePtr, Get(InTablePtr, Owner))
	{}

	IResourceTableIterator* Next() override
	{
		if constexpr (Index + 1 < X::ResourceCount)
		{
			//as long as there are still resources in this handle itterate over them
			//sanity check sizes before sliceing the type
			static_assert(sizeof(ResourceTableIterator<TableType, Index + 1, X, XS...>) == sizeof(ResourceTableIterator<TableType, Index, X, XS...>), "Size don't Match for inplace storage");
			//sliceing the type so that the next vtable call will point to the next element
			new (this) ResourceTableIterator<TableType, Index + 1, X, XS...>(reinterpret_cast<const TableType*>(TablePtr), Entry.GetOwner());
			return this;
		}
		else
		{
			//move to the next handle in the list
			//sanity check sizes before sliceing the type
			static_assert(sizeof(ResourceTableIterator<TableType, 0, XS...>) == sizeof(ResourceTableIterator<TableType, Index, X, XS...>), "Size don't Match for inplace storage");
			//sliceing the type so that the next vtable call will point to the next element
			new (this) ResourceTableIterator<TableType, 0, XS...>(reinterpret_cast<const TableType*>(TablePtr), Entry.GetOwner());
			return this;
		}
	}
};

/* an empty Itterator */
template<typename TableType>
class ResourceTableIterator<TableType, 0> : public IResourceTableIterator
{
	/* helper function to build an empty ResourceTableEntry for the base class */
	static ResourceTableEntry Get()
	{
		return ResourceTableEntry(ResourceRevision(nullptr), nullptr, false, ResourceHandleBase());
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo*) : IResourceTableIterator(InTablePtr, Get())
	{}

	IResourceTableIterator* Next() override
	{
		return this;
	}
};

struct IRenderPassAction;

/* the base class of resource tables provides access to itterators */
class IResourceTableInfo
{
	friend struct RenderPassBuilder;

public:
	/* mandatory C++ itterator implementation */
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

	/* common base for input and output itterators */
	class Iteratable
	{
	public:
		Iteratable() = default;

		Iteratable(const Iteratable& Other) 
		{ 
			check(Other.ResourceTable); //slicing is wanted, static_assert is in place
			memcpy(static_cast<void*>(this), static_cast<const void*>(&Other), sizeof(Iteratable));
		};

		Iteratable& operator= (const Iteratable& Other) 
		{ 
			check(Other.ResourceTable); //slicing is wanted, static_assert is in place
			memcpy(static_cast<void*>(this), static_cast<const void*>(&Other), sizeof(Iteratable));
			return *this;
		}

		virtual IResourceTableInfo::Iterator begin() const 
		{ 
			check(0);
			return {};
		};

		virtual IResourceTableInfo::Iterator end() const 
		{
			check(0);
			return {};
		};

	protected:
		Iteratable(const IResourceTableInfo* InResourceTable) : ResourceTable(InResourceTable) {}
		const IResourceTableInfo* ResourceTable = nullptr;
	};

	IResourceTableInfo(const IRenderPassAction* InAction) : Action(InAction) {}
	virtual ~IResourceTableInfo() {}
	
	/* request to itterate over inputs */
	virtual const Iteratable AsInputIterator() const = 0;
	
	/* request to itterate over outputs */
	virtual const Iteratable AsOutputIterator() const = 0;

	virtual const char* GetName() const = 0;

	/* Only ResourceTables that do draw/dispatch work have an action */
	const IRenderPassAction* GetAction() const
	{
		return Action;
	}

private:
	const IRenderPassAction* Action = nullptr;
};

template<typename... TS>
struct SetOperation
{
private:
	/* A helper struct thas is just a pair of types */
	template<typename CompatibleType, typename SetType>
	struct CompatiblePair {};

	/* A list of pairs with each having their origninal type first and their compatible type second */
	struct CompatiblePairList : CompatiblePair<typename TS::CompatibleType, TS>... {};

	/* Given a CompatibleType overload resolution will find us the Original type from a set of pairs */
	template<typename CompatibleType, typename SetType>
	static constexpr auto GetOriginalTypeInternal(const CompatiblePair<CompatibleType, SetType>&)->SetType;

public:
	/* Helper declaration which plumbs in the List into the extracting function returning the OriginalType given a CompatibleType */
	template<typename CompatibleType>
	static constexpr auto GetOriginalType() -> decltype(GetOriginalTypeInternal<CompatibleType>(CompatiblePairList()));
};

struct IBaseTable {};

namespace Internal
{
	//the Microsoft compiler is a bomb (it blows trying to deduce differnt return type in constexpr if)
	template<bool B>
	struct VisualStudioDeductionHelper
	{
		template<typename X, template<typename...> class RightType, typename... RS>
		static constexpr auto Select(const IBaseTable&, const RightType<RS...>& Rhs)
		{
			// if the right table contains our result than use it 
			// but first restore the original RealType to be able to extract it 
			// because we checked compatible types which might not be the same
			using RealType = decltype(SetOperation<RS...>::template GetOriginalType<typename X::CompatibleType>());
			return Rhs.template GetWrapped<RealType>();
		}

		template<typename X, template<typename...> class RightType, typename... RS>
		static constexpr auto CollectSelect(const RightType<RS...>& Rhs)
		{
			using RealType = decltype(SetOperation<RS...>::template GetOriginalType<typename X::CompatibleType>());
			return Wrapped<X>::ConvertFrom(Rhs.template GetWrapped<RealType>());
		}

		template<template<typename...> class Derived, typename... XS, typename RightType>
		static constexpr Derived<XS...> Collect(const RightType& Rhs)
		{
			(void)Rhs; //silly MSVC thinks it's unreferenced
			// if we found a Handle we searched for we use it 
			// but first restore the original ealType to be able to extract it 
			// because we checked compatible types which might not be the same
			return Derived<XS...>
			(
				CollectSelect<XS>(Rhs)...
			);
		}
	};

	template<>
	struct VisualStudioDeductionHelper<false>
	{
		template<template<typename...> class Derived>
		struct ErrorType
		{
			template<typename... XS>
			static void ThrowError(const Set::Type<XS...>&)
			{
				static int Error = Derived<XS...>();
				Error++;
				static_assert(false, "missing entry: cannot collect");
			}
		};

		template<typename X, template<typename...> class LeftType, typename... LS>
		static constexpr auto Select(const LeftType<LS...>& Lhs, const IBaseTable&)
		{
			// otherwise use the result from the left table 
			// but first restore the original ealType to be able to extract it 
			// because we checked compatible types which might not be the same
			using RealType = decltype(SetOperation<LS...>::template GetOriginalType<typename X::CompatibleType>());
			return Lhs.template GetWrapped<RealType>();
		}

		template<template<typename...> class Derived, typename... XS, typename RightType>
		static constexpr Derived<XS...> Collect(const RightType&)
		{
			//if the collection missed some handles we force an error and print the intersection of the missing values
			ErrorType<Derived>::ThrowError(Set::LeftDifference(Set::Type<XS...>(), RightType::GetCompatibleSetType())); //this will always fail with an error where the DiffTable type is visible
			return std::declval<Derived<XS...>>();
		}
	};
}

/* Common interface for Table operations */
template<template<typename...> class Derived, typename... TS>
class BaseTable : protected Wrapped<TS>..., public IBaseTable
{
	typedef BaseTable<Derived, TS...> ThisType;
public:
	/* create an itterator and point to the start of the table */
	IResourceTableInfo::Iterator begin(const IResourceTableInfo* Owner) const
	{
		return IResourceTableInfo::Iterator::MakeIterator<Derived<TS...>, TS...>(static_cast<const Derived<TS...>*>(this), Owner);
	};

	/* returning the empty itterator */
	IResourceTableInfo::Iterator end() const
	{
		return IResourceTableInfo::Iterator::MakeIterator<Derived<TS...>>(static_cast<const Derived<TS...>*>(this), nullptr);
	};

public:
	/* make friends with other tables */
	template<template<typename...> class, typename...>
	friend class BaseTable;
	
	BaseTable(const Wrapped<TS>&... xs) : Wrapped<TS>(xs)... {}

	/* return the set of unique compatible handle types */
	static constexpr auto GetCompatibleSetType() { return Set::Type<typename TS::CompatibleType...>(); }
	
	/* return the set of unique handle types */
	static constexpr auto GetSetType() { return Set::Type<TS...>(); }
	
	/* return the size of the set */
	static constexpr size_t GetSetSize() { return sizeof...(TS); }

	/* check if a resourcetable contains a compatible handle */
	template<typename C>
	static constexpr bool Contains() 
	{ 
		return GetCompatibleSetType().template Contains<typename C::CompatibleType>();
	}

	/* merge two Tables where the right table overwrites entries from the left if they both contain the same compatible type */
	template<template<typename...> class TableType, typename... YS>
	constexpr auto Union(const TableType<YS...>& Other) const
	{
		using OtherType = TableType<YS...>;
		return ThisType::MergeToLeft(Set::Union(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), static_cast<const Derived<TS...>&>(*this), Other);
	}

	/* intersect two Tables taking the handles from the right Table */
	template<template<typename...> class TableType, typename... YS>
	constexpr auto Intersect(const TableType<YS...>& Other) const
	{
		using OtherType = TableType<YS...>;
		return ThisType::MergeToLeft(Set::Intersect(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), static_cast<const Derived<TS...>&>(*this), Other);
	}

	/* returning the merged table without the intersection */
	template<template<typename...> class TableType, typename... YS>
	constexpr auto Difference(const TableType<YS...>& Other) const
	{
		using OtherType = TableType<YS...>;
		return ThisType::MergeToLeft(Set::Difference(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), static_cast<const Derived<TS...>&>(*this), Other);
	}

	/* given another table itterate though all its elements and fill a new table that only contains the current set of compatble handles */
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

	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(void)Ctx; //silly MSVC thinks it's unreferenced
		(GetWrapped<TS>().OnExecute(Ctx), ...);
	}

private:
	/* prefer select element X from right otherwise take the lefthand side */
	template<typename X, typename LeftType, typename RightType>
	static constexpr auto MergeSelect(const LeftType& Lhs, const RightType& Rhs)
	{
		return Internal::VisualStudioDeductionHelper<RightType::template Contains<X>()>::template Select<X>(Lhs, Rhs);
	}

	/* use the first argument to define the list of elements we are looking for, the second and third are the two tables to merge */
	template<typename... XS, typename LeftType, typename RightType>
	static constexpr auto MergeToLeft(const Set::Type<XS...>&, const LeftType& Lhs, const RightType& Rhs)
	{
		(void)Lhs; //silly MSVC thinks they are unreferenced
		(void)Rhs; //silly MSVC thinks they are unreferenced
		return Derived<typename decltype(ThisType::MergeSelect<XS>(Lhs, Rhs))::HandleType...>
		(
			ThisType::MergeSelect<XS>(Lhs, Rhs)...
		);
	}

	/* use the first argument to define the list of elements we are looking for, the second argument contains the table we collect from */
	template<typename... XS, typename RightType>
	static constexpr auto CollectInternal(const Set::Type<XS...>&, const RightType& Rhs) -> Derived<XS...>
	{
		constexpr bool ContainsAll = (RightType::template Contains<XS>() && ...);
		return Internal::VisualStudioDeductionHelper<ContainsAll>::template Collect<Derived, XS...>(Rhs);
	}
};

/* An interface for InputTables */
template<typename... XS>
class InputTable : protected BaseTable<InputTable, XS...>
{
public:
	typedef BaseTable<::InputTable, XS...> BaseType;
	template<template<typename...> class, typename...>
	friend class BaseTable;

	template<typename, typename>
	friend class ResourceTable;

	template<bool>
	friend struct Internal::VisualStudioDeductionHelper;

	friend struct RenderPassBuilder;

	using BaseType::begin;
	using BaseType::end;
	using BaseType::GetSetType;
	using BaseType::GetCompatibleSetType;
	using BaseType::GetSetSize;
	using BaseType::Contains;
	using BaseType::Union;
	using BaseType::Intersect;
	using BaseType::Difference;
	using BaseType::Collect;
	using BaseType::GetWrapped;
	using BaseType::OnExecute;

	static constexpr bool IsInputTable() { return true; }
	static constexpr bool IsOutputTable() { return false; }

private:
	InputTable(const Wrapped<XS>&... xs) : BaseType(xs...) {}
};

/* An interface for OutputTables */
template<typename... XS>
class OutputTable : protected BaseTable<OutputTable, XS...>
{
public:
	typedef BaseTable<::OutputTable, XS...> BaseType;
	template<template<typename...> class, typename...>
	friend class BaseTable;

	template<typename, typename>
	friend class ResourceTable;

	template<typename>
	friend class ItterableResourceTable;

	template<bool>
	friend struct Internal::VisualStudioDeductionHelper;

	friend struct RenderPassBuilder;

	using BaseType::begin;
	using BaseType::end;
	using BaseType::GetSetType;
	using BaseType::GetCompatibleSetType;
	using BaseType::GetSetSize;
	using BaseType::Contains;
	using BaseType::Union;
	using BaseType::Intersect;
	using BaseType::Difference;
	using BaseType::Collect;
	using BaseType::GetWrapped;
	using BaseType::OnExecute;

	static constexpr bool IsInputTable() { return false; }
	static constexpr bool IsOutputTable() { return true; }

protected:
	/* only OutputTables support Linking */
	template<typename... TS>
	constexpr void Link(const OutputTable<TS...>& Source, const IResourceTableInfo* Parent)
	{
		// we can only link elements that are coming from the source
		LinkInternal(Source, Parent);
	}

private:
	OutputTable(const Wrapped<XS>&... xs) : BaseType(xs...) {}

	/* incrementally link all the Handles from the intersecting set*/
	template<typename... YS>
	constexpr void LinkInternal(const OutputTable<YS...>& Source, const IResourceTableInfo* Parent)
	{
		(this->template GetWrapped<YS>().Link(Source.template GetWrapped<YS>(), Parent), ...);
	}
};

struct IResourceTableBase 
{
	IResourceTableBase(const char* InName) : Name(InName) {}

	/* Name of the Subpass or Action */
	const char* GetName() const
	{
		return Name;
	}

private:
	const char* Name = nullptr;
};

/* ResourceTables are the main payload of the graph implementation */
/* they are similar to compile time sets and they can be easily stored on the stack */
template<typename TInputTableType, typename TOutputTableType>
class ResourceTable : private TInputTableType, private TOutputTableType, public IResourceTableBase
{
public:
	template<typename, typename>
	friend class ResourceTable;

	template<typename>
	friend class ItterableResourceTable;

	friend struct RenderPassBuilder;

	typedef TInputTableType InputTableType;
	typedef TOutputTableType OutputTableType;
	typedef ResourceTable<InputTableType, OutputTableType> ThisType;

public:
	ResourceTable(const ThisType& RTT)
		: ResourceTable(RTT, RTT.GetName()) {};

	explicit ResourceTable(const ThisType& RTT, const char* Name)
		: ResourceTable(RTT.GetInputTable(), RTT.GetOutputTable(), Name) {};

	explicit ResourceTable(const InputTableType& IT, const OutputTableType& OT, const char* Name = "Unnamed")
		: InputTableType(IT)
		, OutputTableType(OT)
		, IResourceTableBase(Name)
	{ 
		CheckIntegrity(); 
	};

	/* assignment constructor from another resourcetable */
	template<typename ITT2, typename OTT2>
	ResourceTable(const ResourceTable<ITT2, OTT2>& RTT) : ResourceTable(RTT.template Populate<ThisType>()) {}

	static constexpr auto GetInputSetType() { return InputTableType::GetSetType(); };
	static constexpr auto GetOutputSetType() { return OutputTableType::GetSetType(); };
	static constexpr size_t GetInputSetSize() { return InputTableType::GetSetSize(); };
	static constexpr size_t GetOutputSetSize() { return OutputTableType::GetSetSize(); };

	template<typename C>
	static constexpr bool ContainsInput() { return InputTableType::template Contains<C>(); }

	template<typename C>
	static constexpr bool ContainsOutput() { return OutputTableType::template Contains<C>(); }

	/* returns the union of two Resourcetables where the right side is taken if they contain similar handles */
	template<typename ITT, typename OTT>
	constexpr auto Union(const ResourceTable<ITT, OTT>& Other) const
	{
		auto in = GetInputTable().Union(Other.GetInputTable());
		auto out = GetOutputTable().Union(Other.GetOutputTable());
		return ResourceTable<decltype(in), decltype(out)>(in, out);
	}

	/* returns the intersection of two Resourcetables */
	template<typename ITT, typename OTT>
	constexpr auto Intersect(const ResourceTable<ITT, OTT>& Other) const
	{
		auto in = GetInputTable().Intersect(Other.GetInputTable());
		auto out = GetOutputTable().Intersect(Other.GetOutputTable());
		return ResourceTable<decltype(in), decltype(out)>(in, out);
	}

	/* returns the union of two Resourcetables without their intersection */
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
	constexpr bool IsUndefinedInput(U32 i = 0) const
	{
		return GetWrappedInput<Handle>().IsUndefined(i);
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
	/* Merging returns the Union of the outputs only, inputs are never merged */ 
	template<typename ITT, typename OTT>
	constexpr auto Merge(const ResourceTable<ITT, OTT>& Parent) const
	{
		auto MergedOutput = Parent.GetOutputTable().Union(GetOutputTable());
		return ResourceTable<ITT, decltype(MergedOutput)>(Parent.GetInputTable(), MergedOutput);
	}

	/* Populate will generate a new Resourcetable from a set of HandleTypes and another Table that must contain all those Handles */ 
	template<typename RTT, typename ITT = typename RTT::InputTableType, typename OTT = typename RTT::OutputTableType>
	constexpr ResourceTable<ITT, OTT> Populate() const
	{
		RTT::CheckIntegrity();
		//collect the results otherwise fail
		return ResourceTable<ITT, OTT>(ITT::Collect(GetInputTable().Union(GetOutputTable())), OTT::Collect(GetOutputTable()));
	}

	template<typename Handle>
	const auto& GetWrappedInput() const
	{
		constexpr bool contains = InputTableType::template Contains<Handle>();
		Wrapped<Handle>::template Test<contains>();
		return GetInputTable().template GetWrapped<Handle>();
	}

	template<typename Handle>
	const auto& GetWrappedOutput() const
	{
		constexpr bool contains = OutputTableType::template Contains<Handle>();
		Wrapped<Handle>::template Test<contains>();
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
};

template<typename ResourceTableType>
class ItterableResourceTable final : public ResourceTableType, public IResourceTableInfo
{
	using ThisType = ItterableResourceTable<ResourceTableType>;
	using InputTableType = typename ResourceTableType::InputTableType;
	using OutputTableType = typename ResourceTableType::OutputTableType;

	friend struct RenderPassBuilder;

private:
	/* an itterator implementation T where is either Input- or OutputTable and RT is this Resourcetable*/
	template<typename T>
	class IteratableImpl : public IResourceTableInfo::Iteratable
	{
		template<typename, typename>
		friend class ResourceTable;

	public:
		IteratableImpl(const ThisType* InResourceTable) : IResourceTableInfo::Iteratable(InResourceTable)
		{
			static_assert(sizeof(IteratableImpl) == sizeof(IResourceTableInfo::Iteratable), "Sizes must match for safe slicing");
		}

		IteratableImpl& operator= (const IteratableImpl& Other)
		{
			//MSVC Compiler bug: the default implementation is mising this->
			this->ResourceTable = Other.ResourceTable;
			return *this;
		}

		IResourceTableInfo::Iterator begin() const override
		{
			return static_cast<const T&>(static_cast<const ThisType&>(*ResourceTable)).begin(ResourceTable);
		}

		IResourceTableInfo::Iterator end() const override
		{
			return static_cast<const T&>(static_cast<const ThisType&>(*ResourceTable)).end();
		}
	};

public:
	explicit ItterableResourceTable(const ResourceTableType& RTT, const char* Name, const IRenderPassAction* InAction)
		: ResourceTableType(RTT, Name)
		, IResourceTableInfo(InAction) {};

	// Get the input itterator
	const IResourceTableInfo::Iteratable AsInputIterator() const override
	{
		IResourceTableInfo::Iteratable Ret; //slice the type
		new (&Ret) IteratableImpl<InputTableType>(this);
		return Ret;
	}

	// Get the output itterator
	const IResourceTableInfo::Iteratable AsOutputIterator() const override
	{
		IResourceTableInfo::Iteratable Ret; //slice the type
		new (&Ret) IteratableImpl<OutputTableType>(this);
		return Ret;
	}

	const char* GetName() const override
	{
		return IResourceTableBase::GetName();
	}

private:
	/* First the tables are merged and than the results are linked to track the history */
	template<typename ITT, typename OTT>
	constexpr auto MergeAndLink(const ResourceTable<ITT, OTT>& Parent) const
	{
		auto MergedOutput = Parent.GetOutputTable().Union(this->GetOutputTable());
		MergedOutput.Link(this->GetOutputTable(), this);
		return ResourceTable<ITT, decltype(MergedOutput)>(Parent.GetInputTable(), MergedOutput);
	}

	/* Entry point for OnExecute callbacks */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		//first transition and execute the inputs
		this->GetInputTable().OnExecute(Ctx);
		//than transition and execute the outputs 
		this->GetOutputTable().OnExecute(Ctx);
		//this way if a compatible type was input bound as Texture
		//and the output type was UAV and previous state was UAV
		//and UAV to texture to UAV barrier will be issued
		//otherwise if UAV was input and output no barrier will be issued
	}
};

template<typename ResourceTableType>
struct ResourceTableOperation
{
private:
	using InputTableType = typename ResourceTableType::InputTableType;
	using OutputTableType = typename ResourceTableType::OutputTableType;

	template<typename... XS>
	static constexpr auto GetSetOperationType(const InputTable<XS...>&)->SetOperation<XS...>;
	using InputOperationType = decltype(GetSetOperationType(std::declval<InputTableType>()));

	template<typename... XS>
	static constexpr auto GetSetOperationType(const OutputTable<XS...>&)->SetOperation<XS...>;
	using OutputOperationType = decltype(GetSetOperationType(std::declval<OutputTableType>())); 

	/* Given two Sets extract their handles and give us a ResourceTable with all those Handles */
	template<typename InSetOp, typename OutSetOp, typename... XS, typename... YS>
	static constexpr auto TableTypeFromSets(const Set::Type<XS...>&, const Set::Type<YS...>&)->ResourceTable
	<
		InputTable<decltype(InSetOp::template GetOriginalType<XS>())...>,
		OutputTable<decltype(OutSetOp::template GetOriginalType<YS>())...>
	>;

	/* Compute the input Intersection e.g keep the Handles in the OutputSet that are also Inputs */
	template<typename InSetOp, typename OutSetOp, typename InputSet, typename OutputSet, typename InputOutputSet = decltype(Set::Intersect(InputSet(), OutputSet()))>
	static constexpr auto ExtractInputTableType(const InputSet&, const OutputSet&) -> decltype(TableTypeFromSets<InSetOp, OutSetOp>(InputSet(), InputOutputSet()));

	/* Remove the Inputs that are also in the OutputSet */
	template<typename InSetOp, typename OutSetOp, typename InputSet, typename OutputSet, typename InputOutputSet = decltype(Set::LeftDifference(InputSet(), OutputSet()))>
	static constexpr auto ExtractOutputTableType(const InputSet&, const OutputSet&) -> decltype(TableTypeFromSets<InSetOp, OutSetOp>(InputOutputSet(), OutputSet()));

public:
	using PassOutputType = decltype(ExtractOutputTableType<InputOperationType, OutputOperationType>(InputTableType::GetCompatibleSetType(), OutputTableType::GetCompatibleSetType()));
	using PassInputType = decltype(ExtractInputTableType<InputOperationType, OutputOperationType>(InputTableType::GetCompatibleSetType(), OutputTableType::GetCompatibleSetType()));
};


#define RESOURCE_TABLE(...)														\
	using ResourceTableType	= ResourceTable< __VA_ARGS__ >;						\
	using PassInputType		= typename ResourceTableOperation<ResourceTableType>::PassInputType;		\
	using PassOutputType	= ResourceTableType;		
