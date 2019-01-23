/*=============================================================================
	RefCounting.h: Reference counting definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * The base class of reference counted objects.
 */
class FRefCountedObject
{
public:
	FRefCountedObject(): NumRefs(0) {}
	virtual ~FRefCountedObject() { check(!NumRefs); }
	void AddRef() const
	{
		NumRefs++;
	}
	void RemoveRef() const
	{
		if(--NumRefs == 0)
		{
			delete this;
		}
	}
private:
	mutable INT NumRefs;
};

/**
 * A smart pointer to an object which implements AddRef/RemoveRef.
 */
template<typename ReferencedType>
class TRefCountPtr
{
	typedef ReferencedType* ReferenceType;
public:

	TRefCountPtr():
		Reference(NULL)
	{}

	TRefCountPtr(ReferencedType* InReference,UBOOL bAddRef = TRUE)
	{
		Reference = InReference;
		if(Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}

	TRefCountPtr(const TRefCountPtr& Copy)
	{
		Reference = Copy.Reference;
		if(Reference)
		{
			Reference->AddRef();
		}
	}

	~TRefCountPtr()
	{
		if(Reference)
		{
			Reference->RemoveRef();
		}
	}

	TRefCountPtr& operator=(ReferencedType* InReference)
	{
		// Call AddRef before RemoveRef, in case the new reference is the same as the old reference.
		ReferencedType* OldReference = Reference;
		Reference = InReference;
		if(Reference)
		{
			Reference->AddRef();
		}
		if(OldReference)
		{
			OldReference->RemoveRef();
		}
		return *this;
	}
	
	TRefCountPtr& operator=(const TRefCountPtr& InPtr)
	{
		return *this = InPtr.Reference;
	}

	UBOOL operator==(const TRefCountPtr& Other) const
	{
		return Reference == Other.Reference;
	}

	ReferencedType* operator->() const
	{
		return Reference;
	}

	operator ReferenceType() const
	{
		return Reference;
	}

	friend FArchive& operator<<(FArchive& Ar,TRefCountPtr& Ptr)
	{
		ReferenceType Reference = Ptr.Reference;
		Ar << Reference;
		if(Ar.IsLoading())
		{
			Ptr = Reference;
		}
		return Ar;
	}

private:
	ReferencedType* Reference;
};
