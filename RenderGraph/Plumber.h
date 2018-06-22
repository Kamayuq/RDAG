#pragma once
#include "Set.h"
#include "Types.h"
#include "Assert.h"
#include "LinearAlloc.h"

static const U32 ALL_SUBRESOURCE_INDICIES = ~0u;

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
private:
	/* The type can be recovered by the TransientResourceImpl */
	mutable MaterializedResource* Resource = nullptr;
	mutable U64* MaterializedSubResources = nullptr;
	U32 SubResourceCount = 0;
	U32 BitFieldIntegers = 0;
	static const U64 BitsPerInt = sizeof(U64) * 8;

protected:
	TransientResource(U32 InSubResourceCount) : SubResourceCount(InSubResourceCount)
	{
		BitFieldIntegers = (SubResourceCount + BitsPerInt - 1) / BitsPerInt;
		MaterializedSubResources = LinearAlloc<U64>(BitFieldIntegers);

		for (U32 i = 0; i < BitFieldIntegers; i++)
		{
			MaterializedSubResources[i] = 0ull;
		}

		U64 Remainder = (BitFieldIntegers * BitsPerInt) - SubResourceCount;
		if (Remainder != 0)
		{
			U64 FillValue = ~0ull;
			FillValue <<= BitsPerInt - Remainder;
			MaterializedSubResources[BitFieldIntegers - 1] = FillValue;
		}
	}

public:
	/* use the descriptor to create a resource and trigger OnMaterilize callback */
	void Materialize(U32 SubResourceIndex) const
	{
		if (Resource == nullptr)
		{
			Resource = MaterializeInternal();
		}

		if (SubResourceIndex == ALL_SUBRESOURCE_INDICIES)
		{
			for (U32 i = 0; i < BitFieldIntegers; i++)
			{
				MaterializedSubResources[i] = ~0ull;
			}
		}
		else
		{
			check(SubResourceIndex < SubResourceCount);
			MaterializedSubResources[SubResourceIndex / BitsPerInt] |= 1ull << (SubResourceIndex % BitsPerInt);
		}
	}

	bool IsMaterialized(U32 SubResourceIndex) const 
	{ 
		if (Resource != nullptr)
		{
			if (SubResourceIndex == ALL_SUBRESOURCE_INDICIES)
			{
				for (U32 i = 0; i < BitFieldIntegers; i++)
				{
					if (MaterializedSubResources[i] != ~0ull)
					{
						return false;
					}
				}
				return true;
			}
			else
			{
				check(SubResourceIndex < SubResourceCount);
				if ((MaterializedSubResources[SubResourceIndex / BitsPerInt] >> (SubResourceIndex % BitsPerInt)) & 1ull)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool IsExternalResource() const 
	{ 
		return Resource && Resource->IsExternalResource(); 
	}

	template<typename Handle>
	const typename Handle::DescriptorType GetDescriptor(U32 SubResourceIndex) const
	{
		return Handle::GetSubResourceDescriptor(static_cast<const TransientResourceImpl<Handle>*>(this)->Descriptor, SubResourceIndex);
	}

	template<typename Handle>
	const typename Handle::ResourceType& GetResource() const
	{
		return *static_cast<const typename Handle::ResourceType*>(static_cast<const TransientResourceImpl<Handle>*>(this)->Resource);
	}

	virtual const char* GetResourceName() const = 0;
	virtual U32 GetResourceWidth(U32 SubResourceIndex) const = 0;
	virtual U32 GetResourceHeight(U32 SubResourceIndex) const = 0;
	virtual U32 GetNumSubResources() const = 0;

private:
	virtual MaterializedResource* MaterializeInternal() const = 0;
};

/* Specialized Transient resource Implementation */
/* Handle is of ResourceHandle Type */
template<typename Handle>
class TransientResourceImpl final : public TransientResource
{
	friend class TransientResource;

	typedef typename Handle::ResourceType ResourceType;
	typedef typename Handle::DescriptorType DescriptorType;
	DescriptorType Descriptor;

public:
	TransientResourceImpl(const DescriptorType& InDescriptor) 
		: TransientResource(Handle::GetSubResourceCount(InDescriptor))
		, Descriptor(InDescriptor) 
	{
		static_assert(std::is_same_v<ResourceType, typename Handle::CompatibleType::ResourceType>, "ResourceTypes must match");
		static_assert(std::is_same_v<DescriptorType, typename Handle::CompatibleType::DescriptorType>, "DescriptorTypes must match");
	}

	const char* GetResourceName() const override
	{
		return Descriptor.Name;
	}

	U32 GetResourceWidth(U32 SubResourceIndex) const override
	{
		return Handle::GetSubResourceDescriptor(Descriptor, SubResourceIndex).Width;
	}

	U32 GetResourceHeight(U32 SubResourceIndex) const override
	{
		return Handle::GetSubResourceDescriptor(Descriptor, SubResourceIndex).Height;
	}

	U32 GetNumSubResources() const override
	{
		return Handle::GetSubResourceCount(Descriptor);
	}

private:
	/* use the descriptor to create a resource and trigger OnMaterilize callback */
	MaterializedResource* MaterializeInternal() const override
	{
		return Handle::OnMaterialize(Descriptor);
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

	bool IsUndefined() const
	{
		return Parent != nullptr;
	}

	bool IsValid() const
	{
		return ImaginaryResource != nullptr;
	}

	bool operator!=(ResourceRevision Other) const
	{
		return!(*this == Other);
	}
};

struct SubResourceRevision
{
	ResourceRevision Revision;
	U32 SubResourceIndex;
};

template<typename Handle>
struct ResourceRevisionInterface
{
	static const typename Handle::DescriptorType GetDescriptor(const SubResourceRevision& SubResource)
	{
		check(SubResource.Revision.IsValid());
		return SubResource.Revision.ImaginaryResource->GetDescriptor<Handle>(SubResource.SubResourceIndex);
	}

	/* forward the OnExecute callback to the Handles implementation and all of it's Resources*/
	static void OnExecute(const SubResourceRevision& SubResource, struct ImmediateRenderContext& RndCtx)
	{
		if (SubResource.Revision.IsUndefined() && SubResource.Revision.ImaginaryResource->IsMaterialized(SubResource.SubResourceIndex))
		{
			Handle::OnExecute(RndCtx, GetResource(SubResource), SubResource.SubResourceIndex);
		}
	}

private:
	static const typename Handle::ResourceType& GetResource(const SubResourceRevision& SubResource)
	{
		check(SubResource.Revision.IsValid() && SubResource.Revision.ImaginaryResource->IsMaterialized(SubResource.SubResourceIndex));
		return SubResource.Revision.ImaginaryResource->GetResource<Handle>();
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
	using HandleTypes = Set::Type<TS...>;
	using CompatibleTypes = Set::Type<typename TS::CompatibleType...>;
	static constexpr size_t StorageSize = sizeof...(TS) > 0 ? sizeof...(TS) : 1;

	const char* Name = nullptr;
	const char* HandleNames[StorageSize];
	ResourceRevision HandleRevisions[StorageSize];
	U32 SubResourceIndicies[StorageSize];
	bool AreOutputResources[StorageSize];

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
	ResourceTable(const ThisType& RTT) = default;

	explicit ResourceTable(const char* InName, const ThisType& RTT)
		: ResourceTable(RTT)
	{  
		Name = InName;
	}

	explicit ResourceTable(const char* Name, const SubResourceRevision (&InSubResources)[sizeof...(TS)])
		: Name(Name)
		, HandleNames{ TS::Name... }
		, HandleRevisions{ InSubResources[HandleTypes::template GetIndex<TS>()].Revision... }
		, SubResourceIndicies{ InSubResources[HandleTypes::template GetIndex<TS>()].SubResourceIndex... }
		, AreOutputResources{ TS::IsOutputResource... }
	{
		(void)InSubResources;
	}

	template
	<
		bool IsTheEmptyTable = sizeof...(TS) == 0, 
		typename = std::enable_if_t<IsTheEmptyTable>
	>
	explicit ResourceTable()
		: Name("EmptyTable")
		, HandleNames{ "DummyHandle" }
		, HandleRevisions{ nullptr }
		, SubResourceIndicies{ ALL_SUBRESOURCE_INDICIES }
		, AreOutputResources{ false }
	{}

	/* assignment constructor from another resourcetable with SINFAE*/
	template
	<
		typename... Handles, 
		bool ContainsAllHandles = (ResourceTable<Handles...>::template Contains<TS>() && ...),
		typename = std::enable_if_t<ContainsAllHandles>
	>
	ResourceTable(const ResourceTable<Handles...>& Other)
		: ResourceTable(Other.GetName(), { Other.template GetSubResource<TS>()... })
	{ 
		(void)Other; 
	}

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
		return CompatibleTypes::template Contains<typename Handle::CompatibleType>();
	}
	/*                   StaticStuff                     */

	/*                  SetOperations                    */
	/* merge two Tables where the right table overwrites entries from the left if they both contain the same compatible type */
	template<typename OtherResourceTable>
	constexpr auto Union(const OtherResourceTable& Other) const
	{
		using ThisTypeMinusOtherType = decltype(Set::Filter<UnionFilterOp<OtherResourceTable>, ::ResourceTable>(typename ThisType::CompatibleTypes()));
		return Meld(Other.GetName(), ThisTypeMinusOtherType(*this), Other);
	}
	/*                  SetOperations                    */

	/*                ElementOperations                  */
	template<typename Handle>
	const typename Handle::DescriptorType GetDescriptor() const
	{
		return ResourceRevisionInterface<Handle>::GetDescriptor(GetSubResource<Handle>());
	}

	template<typename Handle>
	const U32 GetResourceCount() const
	{
		return GetSubResource<Handle>().RevisionCount;
	}

	void CheckAllValid() const
	{
		(GetSubResource<TS>().Revision.IsValid(), ...);
	}

	const char* GetName() const
	{
		return Name;
	}

private:
	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(ResourceRevisionInterface<TS>::OnExecute(GetSubResource<TS>(), Ctx), ...);
	}

	template<typename Handle>
	SubResourceRevision GetSubResource() const
	{
		static_assert(CompatibleTypes::template Contains<typename Handle::CompatibleType>(), "the Handle is not available");
		static_assert(Handle::CompatibleType::template IsConvertible<Handle>(), "the Handle is not convertible");
		constexpr int RevisionIndex = CompatibleTypes::template GetIndex<typename Handle::CompatibleType>();
		return { HandleRevisions[RevisionIndex], SubResourceIndicies[RevisionIndex] };
	}

	template<typename OtherResourceTable>
	struct UnionFilterOp
	{
		template<typename Handle>
		static constexpr bool Filter()
		{
			return !OtherResourceTable::CompatibleTypes::template Contains<typename Handle::CompatibleType>();
		}
	};

	template<typename... XS, typename... YS>
	static constexpr ResourceTable<XS..., YS...> Meld(const char* Name, const ResourceTable<XS...>& A, const ResourceTable<YS...>& B)
	{	
		(void)A; (void)B;
		return ResourceTable<XS..., YS...>{ Name, { A.template GetSubResource<XS>()..., B.template GetSubResource<YS>()... }};
	}
};

/* A ResourceTableEntry is a temporary object for loop itteration, this allows generic access to some parts of the data */
class ResourceTableEntry
{
	//Graph connectivity information 
	SubResourceRevision SubResource;
	//the current owner
	const class IResourceTableInfo* Owner = nullptr;
	//the name as given by the constexpr value of the Handle
	const char* Name = nullptr;
	bool IsOutputResource = false;

public:
	ResourceTableEntry() = default;
	ResourceTableEntry(const ResourceTableEntry& Entry) = default;
	ResourceTableEntry(const SubResourceRevision& InSubResource, bool InIsOutputResource, const class IResourceTableInfo* InOwner, const char* HandleName)
		: SubResource(InSubResource), Owner(InOwner), Name(HandleName), IsOutputResource(InIsOutputResource)
	{}

	const TransientResource* GetImaginaryResource() const
	{
		return SubResource.Revision.ImaginaryResource;
	}

	U32 GetSubResourceIndex() const
	{
		return SubResource.SubResourceIndex;
	}

	const class IResourceTableInfo* GetParent() const
	{
		return SubResource.Revision.Parent;
	}

	const class IResourceTableInfo* GetOwner() const
	{
		return Owner;
	}

	bool IsUndefined() const
	{
		return SubResource.Revision.ImaginaryResource == nullptr || SubResource.Revision.Parent == nullptr;
	}

	bool IsOutput() const
	{
		return IsOutputResource;
	}

	bool IsMaterialized() const
	{
		return SubResource.Revision.ImaginaryResource && SubResource.Revision.ImaginaryResource->IsMaterialized(SubResource.SubResourceIndex);
	}

	/* Materialization from for-each loops starts here */
	void Materialize() const
	{
		check(SubResource.Revision.IsValid());
		SubResource.Revision.ImaginaryResource->Materialize(SubResource.SubResourceIndex);
	}

	const char* GetName() const
	{
		return Name;
	}

	UintPtr ParentHash() const
	{
		return (((reinterpret_cast<UintPtr>(SubResource.Revision.ImaginaryResource) >> 3) * 805306457)
			+ ((reinterpret_cast<UintPtr>(SubResource.Revision.Parent) >> 3) * 1610612741));
	}

	UintPtr Hash() const
	{
		return (((reinterpret_cast<UintPtr>(SubResource.Revision.ImaginaryResource) >> 3) * 805306457)
			+ ((reinterpret_cast<UintPtr>(Owner) >> 3) * 1610612741));
	}

	/* External resourcers are not managed by the graph and the user has to provide an implementation to retrieve the resource */
	bool IsExternal() const
	{
		return IsMaterialized() && SubResource.Revision.ImaginaryResource->IsExternalResource();
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
		const ResourceRevision* HandleRevisions = nullptr;
		const U32* SubResourceIndicies = nullptr;
		const bool* AreOutputResources = nullptr;
		size_t NumHandles = 0;
		U32 CurrentHandleIndex = 0;

		bool Equals(const Iterator& Other) const
		{
			return ResourceTable == Other.ResourceTable
				&& CurrentHandleIndex == Other.CurrentHandleIndex;
		}

	public:
		Iterator(const IResourceTableInfo* InResourceTable, const char* const* InHandleNames, const ResourceRevision* InHandleRevisions, const U32* InSubResourceIndicies, const bool* InAreOutputResources, size_t InNumHandles, bool SetToEnd)
			: ResourceTable(InResourceTable), HandleNames(InHandleNames), HandleRevisions(InHandleRevisions), SubResourceIndicies(InSubResourceIndicies), AreOutputResources(InAreOutputResources), NumHandles(InNumHandles)
		{
			if (SetToEnd)
			{
				CurrentHandleIndex = U32(NumHandles);
			}
		}

		Iterator& operator++()
		{
			CurrentHandleIndex++;
			return *this;
		}

		bool operator==(const Iterator& Other) const { return Equals(Other); }
		bool operator!=(const Iterator& Other) const { return !Equals(Other); }

		ResourceTableEntry operator*() const
		{
			return ResourceTableEntry({ HandleRevisions[CurrentHandleIndex],  SubResourceIndicies[CurrentHandleIndex] }, AreOutputResources[CurrentHandleIndex], ResourceTable, HandleNames[CurrentHandleIndex]);
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
		return Iterator{ this, &this->HandleNames[0], &this->HandleRevisions[0], &this->SubResourceIndicies[0], &this->AreOutputResources[0], this->Size(), false };
	}

	Iterator end() const override
	{
		return Iterator{ this, &this->HandleNames[0], &this->HandleRevisions[0], &this->SubResourceIndicies[0], &this->AreOutputResources[0], this->Size(), true };
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
		static_assert(MergedTable::CompatibleTypes::template Contains<typename Handle::CompatibleType>(), "the Handle is not available");
		static_assert(Handle::CompatibleType::template IsConvertible<Handle>(), "the Handle is not convertible");
		constexpr int DestIndex = MergedTable::CompatibleTypes::template GetIndex<typename Handle::CompatibleType>();
		
		//check that the set has not been tampered with between pass creation and linkage
		check(MergedOutput.HandleRevisions[DestIndex].ImaginaryResource == this->template GetSubResource<Handle>().Revision.ImaginaryResource);
		//point to the new parent
		MergedOutput.HandleRevisions[DestIndex].Parent = this;
	}
};	
