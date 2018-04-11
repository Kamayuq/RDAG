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
	static TransientResourceImpl<Handle>* OnCreate(const typename Handle::DescriptorType& InDescriptor) 
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
	static constexpr U32 ResourceCount = Handle::ResourceCount;
	
	/* each resource carries it's own revision */
	typedef ResourceRevision RevisionArray[ResourceCount];

	/* make friends with other classes as we are unrelated to each other */
	template<typename>
	friend struct Wrapped;

	template<typename, int, typename...>
	friend class ResourceTableIterator;

	friend struct RenderPassBuilder;

	template<typename...>
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

	bool IsValid() const
	{
		return Revision.ImaginaryResource != nullptr;
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
		return ResourceTableEntry(Wrap.Revisions[Index], Owner, Wrap.GetHandle());
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
		return ResourceTableEntry(ResourceRevision(nullptr), nullptr, ResourceHandleBase());
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

namespace Internal
{
	//the Microsoft compiler is a bomb (it blows trying to deduce differnt return type in constexpr if)
	template<bool B>
	struct VisualStudioDeductionHelper
	{
		template<typename X, template<typename...> class RightType, typename... RS>
		static constexpr auto Select(const IResourceTableBase&, const RightType<RS...>& Rhs)
		{
			// see VisualStudioDeductionHelper<false>::Select
			// if the right table contains our result than use it 
			// but first restore the original RealType to be able to extract it 
			// because we checked compatible types which might not be the same
			using RealType = decltype(SetOperation<RS...>::template GetOriginalType<typename X::CompatibleType>());
			return Rhs.template GetWrapped<RealType>();
		}

		template<typename X, template<typename...> class RightType, typename... RS>
		static constexpr auto CollectSelect(const RightType<RS...>& Rhs)
		{
			//Rhs might contain a compatible type so we look for a compatible type and get its RealType (in Rhs) first 
			//before we cast it to the type that we want to fill the new table with
			using RealType = decltype(SetOperation<RS...>::template GetOriginalType<typename X::CompatibleType>());
			return Wrapped<X>::ConvertFrom(Rhs.template GetWrapped<RealType>());
		}

		template<template<typename...> class Derived, typename... XS, typename RightType>
		static constexpr Derived<XS...> Collect(const RightType& Rhs)
		{
			(void)Rhs; //silly MSVC thinks it's unreferenced
			//for all XSs try to collect their values
			//we know the definite type here therefore there is no need to deduce from any compatible type
			return Derived<XS...>
			(
				"Collect", CollectSelect<XS>(Rhs)...
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
				//this assignment will error and therefore print the values that are missing from the table
				static int Error = Derived<XS...>();
				Error++;
				static_assert(false, "missing entry: cannot collect");
			}
		};

		template<typename X, template<typename...> class LeftType, typename... LS>
		static constexpr auto Select(const LeftType<LS...>& Lhs, const IResourceTableBase&)
		{
			// see VisualStudioDeductionHelper<true>::Select
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
/* ResourceTables are the main payload of the graph implementation */
/* they are similar to compile time sets and they can be easily stored on the stack */
template<typename... TS>
class ResourceTable : protected Wrapped<TS>..., public IResourceTableBase
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
		: ResourceTable(RTT.GetName(), RTT) {};

	explicit ResourceTable(const char* Name, const ThisType& RTT)
		: ResourceTable(Name, RTT.GetWrapped<TS>()...) {};

	explicit ResourceTable(const char* Name, const Wrapped<TS>&... xs)
		: Wrapped<TS>(xs)...
		, IResourceTableBase(Name)
	{};

	/* assignment constructor from another resourcetable */
	template<typename... YS>
	ResourceTable(const ResourceTable<YS...>& RTT) : ResourceTable(RTT.template Populate<TS...>()) {}

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
	constexpr bool IsValid(U32 i = 0) const
	{
		return GetWrapped<Handle>().IsValid(i);
	}

	template<typename Handle>
	constexpr bool IsUndefined(U32 i = 0) const
	{
		return GetWrapped<Handle>().IsUndefined(i);
	}

	template<typename Handle>
	const auto& GetHandle() const
	{
		return GetWrapped<Handle>().GetHandle();
	}

	template<typename Handle>
	const auto& GetDescriptor(U32 i = 0) const
	{
		return GetWrapped<Handle>().GetDescriptor(i);
	}

	void CheckAllValid() const
	{
		(check(GetWrapped<TS>().IsValid()), ...);
	}

	template<typename Handle, typename RealType = decltype(SetOperation<TS...>::template GetOriginalType<typename Handle::CompatibleType>())>
	const Wrapped<RealType>& GetWrapped() const
	{ 
		constexpr bool contains = this->template Contains<Handle>();
		Wrapped<Handle>::template Test<contains>();
		return *static_cast<const Wrapped<RealType>*>(this);
	}

protected:
	template<typename Handle, typename RealType = decltype(SetOperation<TS...>::template GetOriginalType<typename Handle::CompatibleType>())>
	Wrapped<RealType>& GetWrapped()
	{
		return *static_cast<Wrapped<RealType>*>(this);
	}
	/*                ElementOperations                  */

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
		using ReturnType = ResourceTable<typename decltype(ThisType::MergeSelect<XS>(Lhs, Rhs))::HandleType...>;
		return ReturnType
		{
			"MergeToLeft", ThisType::MergeSelect<XS>(Lhs, Rhs)...
		};
	}

	/* use the first argument to define the list of elements we are looking for, the second argument contains the table we collect from */
	template<typename... XS, typename RightType>
	static constexpr auto CollectInternal(const Set::Type<XS...>&, const RightType& Rhs)
	{
		constexpr bool ContainsAll = (RightType::template Contains<XS>() && ...);
		return Internal::VisualStudioDeductionHelper<ContainsAll>::template Collect<::ResourceTable, XS...>(Rhs);
	}

	/* Populate will generate a new Resourcetable from a set of HandleTypes and another Table that must contain all those Handles */
	/* given another table itterate though all its elements and fill a new table that only contains the current set of compatible handles */
	template<typename... XS>
	constexpr ResourceTable<XS...> Populate() const
	{
		using ReturnType = ResourceTable<XS...>;
		//collect the results otherwise fail
		return ReturnType(ReturnType::CollectInternal(ReturnType::GetSetType(), *this));
	}

protected:
	/* forward OnExecute callback for all the handles the Table contains */
	void OnExecute(struct ImmediateRenderContext& Ctx) const
	{
		(void)Ctx; //silly MSVC thinks it's unreferenced
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
	template<typename... XS, typename... YS>
	constexpr auto Link(const Set::Type<XS...>&, const ResourceTable<YS...>& Parent) const
	{
		auto MergedOutput = Parent.Union(*this);
		/* incrementally link all the Handles from the intersecting set*/
		(LinkInternal<XS>(MergedOutput, *this), ...);
		return MergedOutput;
	}

	template<typename X, typename... ZS, typename... TS>
	constexpr void LinkInternal(ResourceTable<ZS...>& MergedOutput, const ResourceTable<TS...>& /* this */) const
	{
		using RealDestType = decltype(SetOperation<ZS...>::template GetOriginalType<typename X::CompatibleType>());
		using RealSrcType = decltype(SetOperation<TS...>::template GetOriginalType<typename X::CompatibleType>());
		MergedOutput.template GetWrapped<RealDestType>().Link(this->template GetWrapped<RealSrcType>(), this);
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