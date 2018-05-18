#pragma once
#include "Set.h"
#include "Types.h"
#include "Assert.h"
#include "LinearAlloc.h"

#include <iterator>

template<typename F, typename T, typename = std::void_t<>>
struct IsCompatible : std::false_type {};

template<typename F, typename T>
struct IsCompatible<F, T, std::void_t<decltype(F(std::declval<T>()))>> : std::true_type {};


/* Specialized Transient resource Implementation */
/* Handle is of ResourceHandle Type */
template<typename Handle>
class TransientResourceImpl;

/* Base of all ResourceHandles this class should never be used directly: use ResourceHandle instead */
struct ResourceHandleBase {};

/* A ResourceHande is used to implement and specialize your own Resources and callbacks */
template<typename CRTP>
struct ResourceHandle : ResourceHandleBase
{
	/* Compatible types are used for automatic casting between each other */
	/* each set can only contain unique Compatible Types */
	using CompatibleType = CRTP;
};

/* the base class of all materialized resources */
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

	/* make friends with other classes as we are unrelated to each other */
	template<typename>
	friend struct Wrapped;

	template<typename, typename...>
	friend class ResourceTableIterator;

	friend struct RenderPassBuilder;

	template<typename...>
	friend class ResourceTable;

	Wrapped(const Handle& handle, U32 InResourceCount)
		: Handle(handle)
		, Revisions(LinearAlloc<ResourceRevision>(InResourceCount))
		, ResourceCount(InResourceCount)
	{
		static_assert(std::is_base_of_v<ResourceHandleBase, Handle>, "Handles must derive from ResourceHandle to work");
		for (U32 i = 0; i < ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = nullptr;
			Revisions[i].Parent = nullptr;
		}
	}

	/* Wrapped resources need a handle and a resource array (which they could copy or compose from other handles) */
	Wrapped(const Handle& handle, const DescriptorType* InDescriptors, U32 InResourceCount)
		: Handle(handle)
		, Revisions(LinearAlloc<ResourceRevision>(InResourceCount))
		, ResourceCount(InResourceCount)
	{
		static_assert(std::is_base_of_v<ResourceHandleBase, Handle>, "Handles must derive from ResourceHandle to work"); 
		for (U32 i = 0; i < ResourceCount; i++)
		{
			Revisions[i].ImaginaryResource = Handle::template OnCreate<Handle>(InDescriptors[i]);
			Revisions[i].Parent = nullptr;
			check(Revisions[i].ImaginaryResource);
		}
	}

	Wrapped(const Handle& handle, const ResourceRevision* InRevisions, U32 InResourceCount)
		: Handle(handle)
		, Revisions(LinearAlloc<ResourceRevision>(InResourceCount))
		, ResourceCount(InResourceCount)
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
		check(ResourceCount == Source.ResourceCount);

		ResourceRevision* NewRevisions = LinearAlloc<ResourceRevision>(ResourceCount);
		for (U32 i = 0; i < ResourceCount; i++)
		{
			//check that the set has not been tampered with between pass creation and linkage
			check(Revisions[i].ImaginaryResource == Source.Revisions[i].ImaginaryResource);
			NewRevisions[i].ImaginaryResource = Source.Revisions[i].ImaginaryResource;
			//point to the new parent
			NewRevisions[i].Parent = Parent;
		}
		Revisions = NewRevisions;
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
			if (!IsUndefined(i) && Revisions[i].ImaginaryResource->IsMaterialized())
			{
				Handle::OnExecute(RndCtx, GetResource(i));
			}
		}
	}

	const U32 GetResourceCount() const
	{
		return ResourceCount;
	}

	/* Convert Handles between each other */
	template<typename SourceType>
	static constexpr Wrapped<Handle> ConvertFrom(const Wrapped<SourceType>& Source)
	{
		// During casting of Handles try to call the conversion constructor otherwise fail 
		// if conversion is not allowed by the user
		return { Handle(Source.GetHandle()), Source.Revisions, Source.ResourceCount };
	}
	
protected:
	/* Handles should always be valid, invalid handles were a concept in the past
	and should not be possible due to the strong type safty of the implementation */
	constexpr void CheckAllValid() const
	{
		for (U32 i = 0; i < ResourceCount; i++)
		{
			check(Revisions[i].ImaginaryResource != nullptr);
		}
	}

private:
	/* Handles can get undefined when they never have been written to */
	constexpr bool IsUndefined(U32 i = 0) const
	{
		check(i < ResourceCount);
		check(Revisions[i].ImaginaryResource != nullptr);
		return Revisions[i].Parent == nullptr;
	}

	const ResourceType& GetResource(U32 i = 0) const
	{
		check(Revisions[i].ImaginaryResource != nullptr && Revisions[i].ImaginaryResource->IsMaterialized());
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

	ResourceRevision* Revisions = nullptr;
	U32 ResourceCount = 0;
};

/* A ResourceTableEntry is a temporary object for loop itteration, this allows generic access to some parts of the data */
struct ResourceTableEntry
{
	ResourceTableEntry(const ResourceTableEntry& Entry) = default;

	template<typename Handle> // only the static data of the Handle can be stored
	ResourceTableEntry(const ResourceRevision& InRevision, const IResourceTableInfo* InOwner, const Handle&)
		: Revision(InRevision), Owner(InOwner), Name(Handle::Name)
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
	
	bool IsUndefined() const
	{
		return Revision.ImaginaryResource == nullptr || Revision.Parent == nullptr;
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

public:
	UintPtr ParentHash() const
	{
		return (((reinterpret_cast<UintPtr>(Revision.ImaginaryResource) >> 3) * 805306457)
			  + ((reinterpret_cast<UintPtr>(Revision.Parent) >> 3) * 1610612741));
	}

	UintPtr Hash() const
	{
		return (((reinterpret_cast<UintPtr>(Revision.ImaginaryResource) >> 3) * 805306457)
			  + ((reinterpret_cast<UintPtr>(Owner) >> 3) * 1610612741));
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
	const void* TablePtr = nullptr;
	ResourceTableEntry Entry;
	U32 ResourceIndex = 0;
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
template<typename TableType, typename... XS>
class ResourceTableIterator;

/* a non empty Itterator */
template<typename TableType, typename X, typename... XS>
class ResourceTableIterator<TableType, X, XS...> : public IResourceTableIterator
{
	/* helper function to build a ResourceTableEntry for the base class */
	static ResourceTableEntry Get(const TableType* TablePtr, const IResourceTableInfo* Owner, U32 InResourceIndex)
	{
		const Wrapped<X>& Wrap = TablePtr->template GetWrapped<X>();
		if (Wrap.ResourceCount > 0)
		{
			return ResourceTableEntry(Wrap.Revisions[InResourceIndex], Owner, Wrap.GetHandle());
		}
		else
		{
			return ResourceTableEntry(ResourceRevision(nullptr), Owner, Wrap.GetHandle());
		}
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo* Owner) 
		: IResourceTableIterator(InTablePtr, Get(InTablePtr, Owner, 0))
	{}

	IResourceTableIterator* Next() override
	{
		const TableType* RealTablePtr = static_cast<const TableType*>(TablePtr);
		U32 ResourceCount = RealTablePtr->template GetWrapped<X>().ResourceCount;
		if (ResourceIndex + 1 < ResourceCount)
		{
			//as long as there are still resources in this handle itterate over them
			ResourceIndex++;
			Entry = Get(RealTablePtr, Entry.GetOwner(), ResourceIndex);
			return this;
		}
		else
		{
			//move to the next handle in the list
			//sanity check sizes before sliceing the type
			static_assert(sizeof(ResourceTableIterator<TableType, XS...>) == sizeof(ResourceTableIterator<TableType, X, XS...>), "Size don't Match for inplace storage");
			//sliceing the type so that the next vtable call will point to the next element
			new (this) ResourceTableIterator<TableType, XS...>(reinterpret_cast<const TableType*>(TablePtr), Entry.GetOwner());
			return this;
		}
	}
};

/* an empty Itterator */
template<typename TableType>
class ResourceTableIterator<TableType> : public IResourceTableIterator
{
	struct DummyResourceHandle
	{
		static constexpr const char* Name = nullptr;
	};

	/* helper function to build an empty ResourceTableEntry for the base class */
	static ResourceTableEntry Get()
	{
		return ResourceTableEntry(ResourceRevision(nullptr), nullptr, DummyResourceHandle());
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo*)
		: IResourceTableIterator(InTablePtr, Get())
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

		char ItteratorStorage[sizeof(ResourceTableIterator<void>)];

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
			static_assert(sizeof(ResourceTableIterator<TableType, TS...>) == sizeof(ResourceTableIterator<void>), "Size don't Match for inplace storage");
			new (RetVal.ItteratorStorage) ResourceTableIterator<TableType, TS...>(InTableType, Owner);
			return RetVal;
		}
	};

	IResourceTableInfo(const IRenderPassAction* InAction) : Action(InAction) {}
	virtual ~IResourceTableInfo() {}
	
	/* iterator implementation */
	virtual IResourceTableInfo::Iterator begin() const = 0;
	virtual IResourceTableInfo::Iterator end() const = 0;
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

	/* this function has no implementation as it is only used within a decltype */
	/* Given a CompatibleType overload resolution will find us the Original type from a set of pairs */
	template<typename CompatibleType, typename SetType>
	static constexpr auto GetOriginalTypeInternal(const CompatiblePair<CompatibleType, SetType>&)->SetType;

public:
	/* this function has no implementation as it is only used within a decltype */
	/* Helper declaration which plumbs in the List into the extracting function returning the OriginalType given a CompatibleType */
	template<typename CompatibleType>
	static constexpr auto GetOriginalType() -> decltype(GetOriginalTypeInternal<CompatibleType>(CompatiblePairList()));
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

template<typename, typename>
class DebugResourceTable;

/* Common interface for Table operations */
/* ResourceTables are the main payload of the graph implementation */
/* they are similar to compile time sets and they can be easily stored on the stack */
template<typename... TS>
class ResourceTable : public IResourceTableBase, protected Wrapped<TS>...
{
	using ThisType = ResourceTable<TS...>;

public:
	/*                   MakeFriends                    */
	/* make friends with other tables */
	template<typename...>
	friend class ResourceTable;

	template<typename>
	friend class IterableResourceTable;
	/*                   MakeFriends                    */

	/*                   Constructors                    */
	ResourceTable(const ThisType& RTT)
		: ResourceTable(RTT.GetName(), RTT)
	{}

	explicit ResourceTable(const char* Name, const ThisType& RTT)
		: ResourceTable(Name, RTT.GetWrapped<TS>()...) 
	{}

	explicit ResourceTable(const char* Name, const Wrapped<TS>&... xs)
		: IResourceTableBase(Name)
		, Wrapped<TS>(xs)...
	{}

	/* assignment constructor from another resourcetable with SINFAE*/
	template<typename... YS, typename = std::enable_if_t<std::is_same_v<ThisType, decltype(CollectFrom(std::declval<ResourceTable<YS...>>()))>>>
	ResourceTable(const ResourceTable<YS...>& RTT) 
		: ResourceTable(CollectFrom(RTT)) 
	{}

	/* assignment constructor from another resourcetable for debuging purposes without SINFAE*/
	template<typename DebugType, typename FunctionType>
	ResourceTable(const DebugResourceTable<DebugType, FunctionType>&)
		: ResourceTable(DebugResourceTable<DebugType, FunctionType>::CompileTimeError(*this))
	{}

	/*                   Constructors                    */

	/*                   StaticStuff                     */
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
	/*                   StaticStuff                     */

	/*                  SetOperations                    */
	/* merge two Tables where the right table overwrites entries from the left if they both contain the same compatible type */
	template<typename... YS>
	constexpr auto Union(const ResourceTable<YS...>& Other) const
	{
		using OtherType = ResourceTable<YS...>;
		return ThisType::MergeToLeft(Set::Union(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), *this, Other);
	}

	/* intersect two Tables taking the handles from the right Table */
	template<typename... YS>
	constexpr auto Intersect(const ResourceTable<YS...>& Other) const
	{
		using OtherType = ResourceTable<YS...>;
		return ThisType::MergeToLeft(Set::Intersect(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), *this, Other);
	}

	/* returning the merged table without the intersection */
	template<typename... YS>
	constexpr auto Difference(const ResourceTable<YS...>& Other) const
	{
		using OtherType = ResourceTable<YS...>;
		return ThisType::MergeToLeft(Set::Difference(GetCompatibleSetType(), OtherType::GetCompatibleSetType()), *this, Other);
	}
	/*                  SetOperations                    */

	/*                ElementOperations                  */
	template<typename Handle>
	const auto& GetDescriptor(U32 i = 0) const
	{
		return GetWrapped<Handle>().GetDescriptor(i);
	}

	template<typename Handle>
	const U32 GetResourceCount() const
	{
		return GetWrapped<Handle>().GetResourceCount();
	}

	template<typename Handle>
	const auto& GetHandle() const
	{
		return GetWrapped<Handle>().GetHandle();
	}

	void CheckAllValid() const
	{
		(GetWrapped<TS>().CheckAllValid(), ...);
	}

	template<typename Handle>
	const auto& GetWrapped() const
	{ 
		constexpr bool contains = this->template Contains<Handle>();
		Wrapped<Handle>::template Test<contains>();
		using RealType = decltype(SetOperation<TS...>::template GetOriginalType<typename Handle::CompatibleType>());
		static_assert(IsCompatible<Handle, RealType>::value, "no valid conversion constructor available");
		return *static_cast<const Wrapped<RealType>*>(this);
	}

protected:
	template<typename Handle>
	auto& GetWrapped()
	{
		using RealType = decltype(SetOperation<TS...>::template GetOriginalType<typename Handle::CompatibleType>());
		static_assert(IsCompatible<Handle, RealType>::value, "no valid conversion constructor available");
		return *static_cast<Wrapped<RealType>*>(this);
	}
	/*                ElementOperations                  */

private:
	template<typename X, typename... RS>
	static constexpr auto SelectInternal(const ResourceTable<RS...>& Table)
	{
		// restore the original RealType to be able to extract it 
		// because we checked compatible types which might not be the same
		using RealType = decltype(SetOperation<RS...>::template GetOriginalType<typename X::CompatibleType>());
		static_assert(IsCompatible<X, RealType>::value, "no valid conversion constructor available");
		return Table.template GetWrapped<RealType>();
	}

	/* prefer select element X from right otherwise take the lefthand side */
	template<typename X, typename LeftType, typename RightType>
	static constexpr auto MergeSelect(const LeftType& Lhs, const RightType& Rhs)
	{
		if constexpr (RightType::template Contains<X>())
		{
			(void)Lhs; //silly MSVC thinks they are unreferenced
			// if the right table contains our result than use it 
			// but first restore the original RealType to be able to extract it 
			// because we checked compatible types which might not be the same
			return SelectInternal<X>(Rhs);
		}
		else
		{
			(void)Rhs; //silly MSVC thinks they are unreferenced
			// otherwise use the result from the left table 
			// but first restore the original ealType to be able to extract it 
			// because we checked compatible types which might not be the same
			return SelectInternal<X>(Lhs);
		}
	}

	/* use the first argument to define the list of elements we are looking for, the second and third are the two tables to merge */
	template<typename... XS, typename LeftType, typename RightType>
	static constexpr auto MergeToLeft(const Set::Type<XS...>&, const LeftType& Lhs, const RightType& Rhs)
	{
		using ReturnType = ResourceTable<typename decltype(ThisType::MergeSelect<XS>(Lhs, Rhs))::HandleType...>;
		return ReturnType
		{
			"MergeToLeft", ThisType::MergeSelect<XS>(Lhs, Rhs)...
		};
	}

	/* CollectFrom will generate a new Resourcetable from a set of HandleTypes and another Table that must contain all those Handles */
	/* given another table itterate though all its elements and fill a new table that only contains the current set of compatible handles */
	/* the argument contains the table we collect from */
	template<typename SourceTableType>
	static constexpr auto CollectFrom(const SourceTableType& Rhs)
	{
		(void)Rhs; //silly MSVC thinks it's unreferenced
		constexpr bool ContainsAll = (SourceTableType::template Contains<TS>() && ...);
		if constexpr (ContainsAll)
		{
			//for all XSs try to collect their values
			//we know the definite type here therefore there is no need to deduce from any compatible type
			return ThisType
			(
				//Rhs might contain a compatible type so we look for a compatible type and get its RealType (in Rhs) first 
				//before we cast it to the type that we want to fill the new table with
				"CollectFrom", Wrapped<TS>::ConvertFrom(SelectInternal<TS>(Rhs))...
			);
		}
	}

protected:
	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(GetWrapped<TS>().OnExecute(Ctx), ...);
	}

	IResourceTableInfo::Iterator begin(const IResourceTableInfo* Owner) const
	{
		return IResourceTableInfo::Iterator::MakeIterator<ThisType, TS...>(this, Owner);
	};

	/* returning the empty itterator */
	IResourceTableInfo::Iterator end() const
	{
		return IResourceTableInfo::Iterator::MakeIterator<ThisType>(this, nullptr);
	};
};

template<typename SourceResourceTableType, typename FunctionType>
class DebugResourceTable final : public SourceResourceTableType
{
	template<typename FunctionType2>
	struct ErrorType
	{
		template<typename... XS>
		static constexpr auto ThrowError(const Set::Type<XS...>&)
		{
			//this will error and therefore print the values that are missing from the table
			static_assert(sizeof(Set::Type<XS...>) == 0, "A table entry is missing and the following error will print the types after: TheTypesMissingWere");
			using NotAvailable = decltype(Set::Type<XS...>::TheTypesMissingWere);
			return NotAvailable();
		}
	};

public:
	DebugResourceTable(const SourceResourceTableType& RTT, const FunctionType&) : SourceResourceTableType(RTT)
	{}

	template<typename DestinationResourceTableType>
	static constexpr DestinationResourceTableType CompileTimeError(const DestinationResourceTableType&)
	{
		ErrorType<FunctionType>::ThrowError(Set::LeftDifference(DestinationResourceTableType::GetCompatibleSetType(), SourceResourceTableType::GetCompatibleSetType()));
		return std::declval<DestinationResourceTableType>();
	}
};

template<typename SourceResourceTableType, typename FunctionType>
DebugResourceTable(const SourceResourceTableType&, const FunctionType&) -> DebugResourceTable<SourceResourceTableType, FunctionType>;


template<typename ResourceTableType>
class IterableResourceTable final : public ResourceTableType, public IResourceTableInfo
{
	using ThisType = IterableResourceTable<ResourceTableType>;

	friend struct RenderPassBuilder;

public:
	explicit IterableResourceTable(const ResourceTableType& RTT, const char* Name, const IRenderPassAction* InAction)
		: ResourceTableType(Name, RTT)
		, IResourceTableInfo(InAction) {};

	/* IResourceTableInfo implementation */
	const char* GetName() const override
	{
		return IResourceTableBase::GetName();
	}

	IResourceTableInfo::Iterator begin() const override
	{
		return ResourceTableType::begin(this);
	}

	IResourceTableInfo::Iterator end() const override
	{
		return ResourceTableType::end();
	}

private:
	/* First the tables are merged and than the results are linked to track the history */
	/* Linking is used to update the Resourcetable-entries to point to the previous action */
	template<typename... XS>
	constexpr auto Link(const Set::Type<XS...>&) const
	{
		auto MergedOutput = *this;
		/* incrementally link all the Handles from the intersecting set*/
		(LinkInternal<XS>(MergedOutput), ...);
		return MergedOutput;
	}

	template<typename X, typename... ZS>
	constexpr void LinkInternal(ResourceTable<ZS...>& MergedOutput) const
	{
		using RealType = decltype(SetOperation<ZS...>::template GetOriginalType<typename X::CompatibleType>());
		static_assert(IsCompatible<X, RealType>::value, "no valid conversion constructor available");
		MergedOutput.template GetWrapped<RealType>().Link(this->template GetWrapped<RealType>(), this);
	}

	/* Entry point for OnExecute callbacks */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		//first transition and execute
		ResourceTableType::OnExecute(Ctx);
		//this way if a compatible type was input bound as Texture
		//and the output type was UAV and previous state was UAV
		//and UAV to texture to UAV barrier will be issued
		//otherwise if UAV was input and output no barrier will be issued
	}
};	
