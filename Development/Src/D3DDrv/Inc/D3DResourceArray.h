/*=============================================================================
	D3DResourceArray.h: D3D Resource array definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** alignment for supported resource types */
enum EResourceAlignment
{
	VERTEXBUFFER_ALIGNMENT	=DEFAULT_ALIGNMENT,
	INDEXBUFFER_ALIGNMENT	=DEFAULT_ALIGNMENT
};

#define TResourceArray TD3DResourceArray

/**
 * A array which allocates memory which can be used for UMA rendering resources.
 * This is currently just a placeholder.
 *
 * @param bNeedsCPUAccess -	TRUE if this array needs to be accessed by the CPU.  
 *							Prevents the UMA array from being unloaded on non-UMA systems.
 */
template< typename ElementType, UBOOL bNeedsCPUAccess = FALSE, DWORD Alignment=DEFAULT_ALIGNMENT >
class TD3DResourceArray : public TArray<ElementType,Alignment>, public FResourceArrayInterface
{
public:

	// FResourceArrayInterface

	/**
	* @return A pointer to the resource data.
	*/
	virtual const void* GetResourceData() const 
	{ 
		return &(*this)(0); 
	}

	/**
	* @return size of resource data allocation
	*/
	virtual DWORD GetResourceDataSize() const
	{
		return Num() * sizeof(ElementType);
	}

	/**
	* Called on non-UMA systems after the RHI has copied the resource data, and no longer needs the CPU's copy.
	*/
	virtual void Discard()
	{
		// Only discard the resource memory on clients, and if the CPU doesn't need access to it.
		// Non-clients can't discard the data because they may need to serialize it.
		if(!bNeedsCPUAccess && !GIsEditor && !GIsUCC)
		{
			this->Empty();
		}
	}

	/**
	* @return TRUE if the resource array is static and shouldn't be modified
	*/
	virtual UBOOL IsStatic() const
	{
		return FALSE;
	}

	// Assignment operators.

	TD3DResourceArray& operator=(const TD3DResourceArray& Other)
	{
		TArray<ElementType,Alignment>::operator=(Other);
		return *this;
	}
	TD3DResourceArray& operator=(const TArray<ElementType,Alignment>& Other)
	{
		TArray<ElementType,Alignment>::operator=(Other);
		return *this;
	}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar,TD3DResourceArray& ResourceArray)
	{
		return Ar << *(TArray<ElementType,Alignment>*)&ResourceArray;
	}
};


