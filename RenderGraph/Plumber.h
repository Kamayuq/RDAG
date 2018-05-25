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
struct ResourceHandleBase {};

/* A ResourceHande is used to implement and specialize your own Resources and callbacks */
template<typename Compatible>
struct ResourceHandle : ResourceHandleBase
{
	/* Compatible types are used for automatic casting between each other */
	/* each set can only contain unique Compatible Types */
	using CompatibleType = Compatible;

	ResourceHandle()
	{
		static_assert(sizeof(SizeTest) == 1, "Handles are not allowed to have any values");
	}

private:
	struct SizeTest : Compatible
	{
		char dummy;
	};
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

struct RevisionSet
{
	RevisionSet(const RevisionSet&) = default;

	RevisionSet(ResourceRevision* const InRevisions, U32 InRevisionCount)
		: Revisions(InRevisions)
		, RevisionCount(InRevisionCount)
	{
	}

	ResourceRevision* const Revisions = nullptr;
	U32 RevisionCount = 0;
};

template<typename Handle>
struct InternalRevisionSet
{
	friend struct RenderPassBuilder;

	using HandleType = Handle;
	using ResourceType = typename Handle::ResourceType;
	using DescriptorType = typename Handle::DescriptorType;

	InternalRevisionSet(const RevisionSet& InRevisionSet) : Revisions(InRevisionSet)
	{}

	const DescriptorType& GetDescriptor(U32 i = 0) const
	{
		check(i < Revisions.RevisionCount && Revisions.Revisions[i].ImaginaryResource != nullptr);
		return static_cast<const TransientResourceImpl<Handle>*>(Revisions.Revisions[i].ImaginaryResource)->Descriptor;
	}

	const U32 GetResourceCount() const
	{
		return Revisions.RevisionCount;
	}

	/* forward the OnExecute callback to the Handles implementation and all of it's Resources*/
	void OnExecute(struct ImmediateRenderContext& RndCtx) const
	{
		for (U32 i = 0; i < Revisions.RevisionCount; i++)
		{
			if (!IsUndefined(i) && Revisions.Revisions[i].ImaginaryResource->IsMaterialized())
			{
				Handle::OnExecute(RndCtx, GetResource(i));
			}
		}
	}

	constexpr void CheckAllValid() const
	{
		for (U32 i = 0; i < Revisions.RevisionCount; i++)
		{
			check(Revisions.Revisions[i].ImaginaryResource != nullptr);
		}
	}

private:
	const ResourceRevision& GetRevision(U32 i = 0) const
	{
		check(i < Revisions.RevisionCount);
		return Revisions.Revisions[i];
	}

	/* Handles can get undefined when they never have been written to */
	constexpr bool IsUndefined(U32 i = 0) const
	{
		check(i < Revisions.RevisionCount);
		check(Revisions.Revisions[i].ImaginaryResource != nullptr);
		return Revisions.Revisions[i].Parent == nullptr;
	}

	const ResourceType& GetResource(U32 i = 0) const
	{
		check(i < Revisions.RevisionCount);
		check(Revisions.Revisions[i].ImaginaryResource != nullptr && Revisions.Revisions[i].ImaginaryResource->IsMaterialized());
		return static_cast<const ResourceType&>(*Revisions.Revisions[i].ImaginaryResource->GetResource());
	}

	RevisionSet Revisions;
};

/* A ResourceTableEntry is a temporary object for loop itteration, this allows generic access to some parts of the data */
struct ResourceTableEntry
{
	ResourceTableEntry() = default;
	ResourceTableEntry(const ResourceTableEntry& Entry) = default;

	ResourceTableEntry(const ResourceRevision& InRevision, const IResourceTableInfo* InOwner, const char* HandleName)
		: Revision(InRevision), Owner(InOwner), Name(HandleName)
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

	IResourceTableIterator() = default;
	IResourceTableIterator(const void* InTablePtr, const ResourceTableEntry& InEntry) : TablePtr(InTablePtr), Entry(InEntry)
	{}
	
public:
	/* move to the next element, can return the empty element */
	virtual IResourceTableIterator* Next() = 0;

public:
	bool Equals(const IResourceTableIterator& Other) const
	{
		if (Entry != Other.Entry)
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
class ResourceTableIterator<TableType, X, XS...> final : public IResourceTableIterator
{
	/* helper function to build a ResourceTableEntry for the base class */
	static ResourceTableEntry GetInternal(const TableType* TablePtr, const IResourceTableInfo* Owner, U32 InResourceIndex)
	{
		RevisionSet Wrap = TablePtr->template GetRevisionSet<X>();
		if (Wrap.RevisionCount > 0)
		{
			return ResourceTableEntry(Wrap.Revisions[InResourceIndex], Owner, X::Name);
		}
		else
		{
			return ResourceTableEntry();
		}
	}

public:
	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo* Owner) 
		: IResourceTableIterator(InTablePtr, GetInternal(InTablePtr, Owner, 0))
	{}

	IResourceTableIterator* Next() override
	{
		const TableType* RealTablePtr = static_cast<const TableType*>(TablePtr);
		U32 ResourceCount = RealTablePtr->template GetRevisionSet<X>().RevisionCount;
		if (ResourceIndex + 1 < ResourceCount)
		{
			//as long as there are still resources in this handle itterate over them
			ResourceIndex++;
			Entry = GetInternal(RealTablePtr, Entry.GetOwner(), ResourceIndex);
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
class ResourceTableIterator<TableType> final : public IResourceTableIterator
{
public:
	ResourceTableIterator() = default;

	ResourceTableIterator(const TableType* InTablePtr, const IResourceTableInfo*)
		: IResourceTableIterator(InTablePtr, ResourceTableEntry())
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

		ResourceTableIterator<void> DummyItterator;

	public:
		Iterator& operator++() 
		{ 
			//cast necessary for slicing to take effect
			reinterpret_cast<IResourceTableIterator&>(DummyItterator).Next();
			return *this;
		}

		bool operator==(const Iterator& Other) const 
		{ 
			return DummyItterator.Equals(Other.DummyItterator);
		}

		bool operator!=(const Iterator& Other) const
		{ 
			return !DummyItterator.Equals(Other.DummyItterator);
		}

		reference operator*() const
		{ 
			return DummyItterator.Get();
		}

		template<typename TableType, typename... TS>
		static Iterator MakeIterator(const TableType* InTableType, const IResourceTableInfo* Owner)
		{
			Iterator RetVal;
			static_assert(sizeof(ResourceTableIterator<TableType, TS...>) == sizeof(ResourceTableIterator<void>), "Size don't Match for inplace storage");
			new (&RetVal.DummyItterator) ResourceTableIterator<TableType, TS...>(InTableType, Owner);
			return RetVal;
		}
	};

	IResourceTableInfo(const IRenderPassAction* InAction) : Action(InAction) {}
	virtual ~IResourceTableInfo() {}
	
	/* iterator implementation */
	virtual IResourceTableInfo::Iterator begin() const = 0;

	IResourceTableInfo::Iterator end() const
	{
		return Iterator::MakeIterator<void>(nullptr, nullptr);
	}

	virtual const char* GetName() const = 0;

	/* Only ResourceTables that do draw/dispatch work have an action */
	const IRenderPassAction* GetAction() const
	{
		return Action;
	}

private:
	const IRenderPassAction* Action = nullptr;
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
class ResourceTable : public IResourceTableBase
{
	using ThisType = ResourceTable<TS...>;
	using SetType = Set::Type<TS...>;
	using CompatibleType = Set::Type<typename TS::CompatibleType...>;

	static constexpr size_t StorageSize = sizeof...(TS) > 0 ? sizeof...(TS) : 1;
	//const char* HandleNames[StorageSize];
	ResourceRevision* HandleRevisions[StorageSize];
	U32 RevisionCounts[StorageSize];

public:
	/*                   MakeFriends                    */
	/* make friends with other tables */
	template<typename...>
	friend class ResourceTable;

	template<typename>
	friend class IterableResourceTable;

	template<typename, typename...>
	friend class ResourceTableIterator;

	friend struct RenderPassBuilder;
	/*                   MakeFriends                    */

	/*                   Constructors                    */
	ResourceTable(const ThisType& RTT)
		: ResourceTable(RTT.GetName(), RTT)
	{}

	explicit ResourceTable(const char* Name, const ThisType& RTT)
		: ResourceTable(Name, { RevisionSet(RTT.HandleRevisions[SetType::template GetIndex<TS>()], RTT.RevisionCounts[SetType::template GetIndex<TS>()])... })
	{
		(void)RTT;
	}

	explicit ResourceTable(const char* Name)
		: IResourceTableBase(Name)
		//, HandleNames{ "EmptyTable" }
		, HandleRevisions{ nullptr }
		, RevisionCounts{ 0 }
	{}

	explicit ResourceTable(const char* Name, const RevisionSet (&InRevisions)[sizeof...(TS)])
		: IResourceTableBase(Name)
		//, HandleNames{ TS::Name... }
		, HandleRevisions{ InRevisions[SetType::template GetIndex<TS>()].Revisions... }
		, RevisionCounts{ InRevisions[SetType::template GetIndex<TS>()].RevisionCount... }
	{
		(void)InRevisions;
	}

	/* assignment constructor from another resourcetable with SINFAE*/
	template
	<
		typename OtherResourceTable, 
		bool ContainsAll = (OtherResourceTable::template Contains<TS>() && ...),
		typename = std::enable_if_t<ContainsAll>
	>
	ResourceTable(const OtherResourceTable& Other)
		: ResourceTable("Assignment", { Other.template GetRevisionSet<TS>()... })
	{
		(void)Other;
	}

	/* assignment constructor from another resourcetable for debuging purposes without SINFAE*/
	template<typename DebugType, typename FunctionType>
	explicit ResourceTable(const DebugResourceTable<DebugType, FunctionType>&)
		: ResourceTable(DebugResourceTable<DebugType, FunctionType>::CompileTimeError(*this))
	{}
	/*                   Constructors                    */

	/*                   StaticStuff                     */
	/* check if a resourcetable contains a compatible handle */
	template<typename Handle>
	static constexpr bool Contains() 
	{ 
		return CompatibleType::template Contains<typename Handle::CompatibleType>();
	}
	/*                   StaticStuff                     */

	/*                  SetOperations                    */
	/* merge two Tables where the right table overwrites entries from the left if they both contain the same compatible type */
	template<typename OtherResourceTable>
	constexpr auto Union(const OtherResourceTable& Other) const
	{
		using ThisTypeMinusOtherType = decltype(Set::Filter<UnionFilterOp<OtherResourceTable>, ::ResourceTable>(typename ThisType::CompatibleType()));
		return Meld(ThisTypeMinusOtherType(*this), Other);
	}
	/*                  SetOperations                    */

	/*                ElementOperations                  */
	template<typename Handle>
	const auto& GetDescriptor(U32 i = 0) const
	{
		return InternalRevisionSet<Handle>(GetRevisionSet<Handle>()).GetDescriptor(i);
	}

	template<typename Handle>
	const U32 GetResourceCount() const
	{
		return InternalRevisionSet<Handle>(GetRevisionSet<Handle>()).GetResourceCount();
	}

	void CheckAllValid() const
	{
		(InternalRevisionSet<TS>(GetRevisionSet<TS>()).CheckAllValid(), ...);
	}

protected:
	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(InternalRevisionSet<TS>(GetRevisionSet<TS>()).OnExecute(Ctx), ...);
	}

	IResourceTableInfo::Iterator begin(const IResourceTableInfo* Owner) const
	{
		return IResourceTableInfo::Iterator::MakeIterator<ThisType, TS...>(this, Owner);
	};

private:
	template<typename Handle>
	RevisionSet GetRevisionSet() const
	{
		static_assert(CompatibleType::template Contains<typename Handle::CompatibleType>(), "the Handle is not available");
		static_assert(Handle::CompatibleType::template IsConvertible<Handle>(), "the Handle is not convertible");
		constexpr int RevisionIndex = CompatibleType::template GetIndex<typename Handle::CompatibleType>();
		return { HandleRevisions[RevisionIndex], RevisionCounts[RevisionIndex] };
	}

	template<typename OtherResourceTable>
	struct UnionFilterOp
	{
		template<typename Handle>
		static constexpr bool Filter()
		{
			return !OtherResourceTable::CompatibleType::template Contains<typename Handle::CompatibleType>();
		}
	};

	template<typename... XS, typename... YS>
	static constexpr auto Meld(const ResourceTable<XS...>& A, const ResourceTable<YS...>& B)
	{
		(void)A; (void)B;
		return ResourceTable<XS..., YS...>{ "MeldedTable", { A.template GetRevisionSet<XS>()..., B.template GetRevisionSet<YS>()... }};
	}
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

	template<typename Handle, typename... ZS>
	constexpr void LinkInternal(ResourceTable<ZS...>& MergedOutput) const
	{
		using MergedTable = ResourceTable<ZS...>;
		static_assert(MergedTable::CompatibleType::template Contains<typename Handle::CompatibleType>(), "the Handle is not available");
		static_assert(Handle::CompatibleType::template IsConvertible<Handle>(), "the Handle is not convertible");
		constexpr int DestIndex = MergedTable::CompatibleType::template GetIndex<typename Handle::CompatibleType>();
		
		ResourceRevision* NewRevisions = LinearAlloc<ResourceRevision>(MergedOutput.RevisionCounts[DestIndex]);
		for (U32 i = 0; i < MergedOutput.RevisionCounts[DestIndex]; i++)
		{
			//check that the set has not been tampered with between pass creation and linkage
			check(MergedOutput.HandleRevisions[DestIndex][i].ImaginaryResource == this->template GetRevisionSet<Handle>().Revisions[i].ImaginaryResource);
			NewRevisions[i].ImaginaryResource = MergedOutput.HandleRevisions[DestIndex][i].ImaginaryResource;
			//point to the new parent
			NewRevisions[i].Parent = this;
		}
		MergedOutput.HandleRevisions[DestIndex] = NewRevisions;
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
