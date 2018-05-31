#pragma once
#include "Set.h"
#include "Types.h"
#include "Assert.h"
#include "LinearAlloc.h"

/* A ResourceHande is used to implement and specialize your own Resources and callbacks */
template<typename Compatible>
struct ResourceHandle
{
	/* Compatible types are used for automatic casting between each other */
	/* each set can only contain unique Compatible Types */
	using CompatibleType = Compatible;

	ResourceHandle()
	{
		struct SizeTest : Compatible { char dummy; };
		static_assert(sizeof(SizeTest) == 1, "Handles are not allowed to have any values");
	}
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
public:
	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;

	DescriptorType Descriptor;

	TransientResourceImpl(const DescriptorType& InDescriptor) : Descriptor(InDescriptor) {}

	/* use the descriptor to create a resource and trigger OnMaterilize callback */
	void Materialize() const override
	{
		if (Resource == nullptr)
		{
			Resource = Handle::OnMaterialize(Descriptor);
		}
	}
};

/* the ResourceRevision stores the connectivity and History between transient resources in the graph */
struct ResourceRevision
{
	/* a link to it's underlying immaginary resource */
	const TransientResource* ImaginaryResource = nullptr;
	/* a link to the resourcetable set where this revision came from originally */
	const class IResourceTableInfo* Parent = nullptr;

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
	ResourceRevision* const Revisions = nullptr;
	U32 RevisionCount = 0;

	RevisionSet(const RevisionSet&) = default;
	RevisionSet(ResourceRevision* const InRevisions, U32 InRevisionCount)
		: Revisions(InRevisions)
		, RevisionCount(InRevisionCount)
	{}

	/* Handles can get undefined when they never have been written to */
	bool IsUndefined(U32 i = 0) const
	{
		check(i < RevisionCount);
		check(Revisions[i].ImaginaryResource != nullptr);
		return Revisions[i].Parent == nullptr;
	}

	void CheckAllValid() const
	{
		for (U32 i = 0; i < RevisionCount; i++)
		{
			check(Revisions[i].ImaginaryResource != nullptr);
		}
	}
};

template<typename Handle>
struct RevisionSetInterface
{
	static const typename Handle::DescriptorType& GetDescriptor(const RevisionSet& Revisions, U32 i = 0)
	{
		check(i < Revisions.RevisionCount && Revisions.Revisions[i].ImaginaryResource != nullptr);
		return static_cast<const TransientResourceImpl<Handle>*>(Revisions.Revisions[i].ImaginaryResource)->Descriptor;
	}

	/* forward the OnExecute callback to the Handles implementation and all of it's Resources*/
	static void OnExecute(const RevisionSet& Revisions, struct ImmediateRenderContext& RndCtx)
	{
		for (U32 i = 0; i < Revisions.RevisionCount; i++)
		{
			if (!Revisions.IsUndefined(i) && Revisions.Revisions[i].ImaginaryResource->IsMaterialized())
			{
				Handle::OnExecute(RndCtx, GetResource(Revisions, i));
			}
		}
	}

private:
	static const typename Handle::ResourceType& GetResource(const RevisionSet& Revisions, U32 i = 0)
	{
		check(i < Revisions.RevisionCount);
		check(Revisions.Revisions[i].ImaginaryResource != nullptr && Revisions.Revisions[i].ImaginaryResource->IsMaterialized());
		return static_cast<const typename Handle::ResourceType&>(*Revisions.Revisions[i].ImaginaryResource->GetResource());
	}
};

template<typename, typename>
class DebugResourceTable;

/* Common interface for Table operations */
/* ResourceTables are the main payload of the graph implementation */
/* they are similar to compile time sets and they can be easily stored on the stack */
struct IResourceTableBase {};

template<typename... TS>
class ResourceTable : public IResourceTableBase
{
	using ThisType = ResourceTable<TS...>;
	using SetType = Set::Type<TS...>;
	using CompatibleType = Set::Type<typename TS::CompatibleType...>;
	static constexpr size_t StorageSize = sizeof...(TS) > 0 ? sizeof...(TS) : 1;

	const char* Name = nullptr;
	const char* HandleNames[StorageSize];
	ResourceRevision* HandleRevisions[StorageSize];
	U32 RevisionCounts[StorageSize];

public:
	/*                   MakeFriends                    */
	/* make friends with other tables */
	template<typename...>
	friend class ResourceTable;

	template<typename>
	friend class IterableResourceTable;

	friend struct RenderPassBuilder;
	/*                   MakeFriends                    */

	/*                   Constructors                    */
	ResourceTable(const ThisType& RTT)
		: ResourceTable(RTT.GetName(), RTT)
	{}

	explicit ResourceTable(const char* Name, const ThisType& RTT)
		: ResourceTable(Name, { RevisionSet(RTT.HandleRevisions[SetType::template GetIndex<TS>()], RTT.RevisionCounts[SetType::template GetIndex<TS>()])... })
	{ (void)RTT; }

	template
	<
		bool IsTheEmptyTable = sizeof...(TS) == 0, 
		typename = std::enable_if_t<IsTheEmptyTable>
	>
	explicit ResourceTable()
		: Name("EmptyTable")
		, HandleNames{ "DummyHandle" }
		, HandleRevisions{ nullptr }
		, RevisionCounts{ 0 }
	{}

	explicit ResourceTable(const char* Name, const RevisionSet (&InRevisions)[sizeof...(TS)])
		: Name(Name)
		, HandleNames{ TS::Name... }
		, HandleRevisions{ InRevisions[SetType::template GetIndex<TS>()].Revisions... }
		, RevisionCounts{ InRevisions[SetType::template GetIndex<TS>()].RevisionCount... }
	{ (void)InRevisions; }

	/* assignment constructor from another resourcetable with SINFAE*/
	template
	<
		typename... Handles, 
		bool ContainsAll = (ResourceTable<Handles...>::template Contains<TS>() && ...),
		typename = std::enable_if_t<ContainsAll>
	>
	ResourceTable(const ResourceTable<Handles...>& Other)
		: ResourceTable(Other.GetName(), { Other.template GetRevisionSet<TS>()... })
	{ (void)Other; }

	/* assignment constructor from another resourcetable for debuging purposes without SINFAE*/
	template<typename DebugType, typename FunctionType>
	ResourceTable(const DebugResourceTable<DebugType, FunctionType>&)
		: ResourceTable(DebugResourceTable<DebugType, FunctionType>::CompileTimeError(*this))
	{}
	/*                   Constructors                    */

	/*                   StaticStuff                     */
	static constexpr size_t Size()
	{
		return sizeof...(TS);
	}

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
		return Meld(Other.GetName(), ThisTypeMinusOtherType(*this), Other);
	}
	/*                  SetOperations                    */

	/*                ElementOperations                  */
	template<typename Handle>
	const typename Handle::DescriptorType& GetDescriptor(U32 i = 0) const
	{
		return RevisionSetInterface<Handle>::GetDescriptor(GetRevisionSet<Handle>(), i);
	}

	template<typename Handle>
	const U32 GetResourceCount() const
	{
		return GetRevisionSet<Handle>().RevisionCount;
	}

	void CheckAllValid() const
	{
		(GetRevisionSet<TS>().CheckAllValid(), ...);
	}

	const char* GetName() const
	{
		return Name;
	}

private:
	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(RevisionSetInterface<TS>::OnExecute(GetRevisionSet<TS>(), Ctx), ...);
	}

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
	static constexpr ResourceTable<XS..., YS...> Meld(const char* Name, const ResourceTable<XS...>& A, const ResourceTable<YS...>& B)
	{	(void)A; (void)B;
		return ResourceTable<XS..., YS...>{ Name, { A.template GetRevisionSet<XS>()..., B.template GetRevisionSet<YS>()... }};
	}
};

/* A ResourceTableEntry is a temporary object for loop itteration, this allows generic access to some parts of the data */
class ResourceTableEntry
{
	//Graph connectivity information 
	ResourceRevision Revision;
	//the current owner
	const class IResourceTableInfo* Owner = nullptr;
	//the name as given by the constexpr value of the Handle
	const char* Name = nullptr;

public:
	ResourceTableEntry() = default;
	ResourceTableEntry(const ResourceTableEntry& Entry) = default;
	ResourceTableEntry(const ResourceRevision& InRevision, const class IResourceTableInfo* InOwner, const char* HandleName)
		: Revision(InRevision), Owner(InOwner), Name(HandleName)
	{}

	const TransientResource* GetImaginaryResource() const
	{
		return Revision.ImaginaryResource;
	}

	const class IResourceTableInfo* GetParent() const
	{
		return Revision.Parent;
	}

	const class IResourceTableInfo* GetOwner() const
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
};

/* the base class of resource tables provides access to itterators */
class IResourceTableInfo
{
private:
	const struct IRenderPassAction* Action = nullptr;

public:
	IResourceTableInfo(const struct IRenderPassAction* InAction) : Action(InAction) {}
	virtual ~IResourceTableInfo() {}

	/* mandatory C++ itterator implementation */
	class Iterator
	{
		const IResourceTableInfo* ResourceTable = nullptr;
		const char* const* HandleNames = nullptr;
		ResourceRevision* const* HandleRevisions = nullptr;
		const U32* RevisionCounts = nullptr;
		size_t NumHandles = 0;
		U32 CurrentHandleIndex = 0;
		U32 CurrentRevisionIndex = 0;

		bool Equals(const Iterator& Other) const
		{
			return ResourceTable == Other.ResourceTable
				&& CurrentHandleIndex == Other.CurrentHandleIndex
				&& CurrentRevisionIndex == Other.CurrentRevisionIndex;
		}

	public:
		Iterator(const IResourceTableInfo* InResourceTable, const char* const* InHandleNames, ResourceRevision* const* InHandleRevisions, const U32* InRevisionCounts, size_t InNumHandles, bool SetToEnd)
			: ResourceTable(InResourceTable), HandleNames(InHandleNames), HandleRevisions(InHandleRevisions), RevisionCounts(InRevisionCounts), NumHandles(InNumHandles)
		{
			if (SetToEnd)
			{
				CurrentHandleIndex = U32(NumHandles);
			}
		}

		Iterator& operator++()
		{
			CurrentRevisionIndex++;
			if (CurrentRevisionIndex >= RevisionCounts[CurrentHandleIndex])
			{
				CurrentRevisionIndex = 0;
				CurrentHandleIndex++;
			}
			return *this;
		}

		bool operator==(const Iterator& Other) const { return Equals(Other); }
		bool operator!=(const Iterator& Other) const { return !Equals(Other); }

		ResourceTableEntry operator*() const
		{
			return ResourceTableEntry(HandleRevisions[CurrentHandleIndex][CurrentRevisionIndex], ResourceTable, HandleNames[CurrentHandleIndex]);
		}
	};

	/* iterator implementation */
	virtual Iterator begin() const = 0;
	virtual Iterator end() const = 0;
	virtual const char* GetName() const = 0;

	/* Only ResourceTables that do draw/dispatch work have an action */
	const struct IRenderPassAction* GetAction() const
	{
		return Action;
	}
};

template<typename ResourceTableType>
class IterableResourceTable final : public ResourceTableType, public IResourceTableInfo
{
	friend struct RenderPassBuilder;

public:
	explicit IterableResourceTable(const ResourceTableType& RTT, const char* Name, const struct IRenderPassAction* InAction)
		: ResourceTableType(Name, RTT)
		, IResourceTableInfo(InAction) 
	{};

private:
	/* IResourceTableInfo implementation */
	const char* GetName() const override
	{
		return ResourceTableType::GetName();
	}

	Iterator begin() const override
	{
		return { this, &this->HandleNames[0], &this->HandleRevisions[0], &this->RevisionCounts[0], this->Size(), false };
	}

	Iterator end() const override
	{
		return { this, &this->HandleNames[0], &this->HandleRevisions[0], &this->RevisionCounts[0], this->Size(), true };
	}

	/* First the tables are merged and than the results are linked to track the history */
	/* Linking is used to update the Resourcetable-entries to point to the previous action */
	template<typename... XS>
	constexpr ResourceTableType Link(const Set::Type<XS...>&) const
	{
		ResourceTableType MergedOutput = *this;
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
};	
