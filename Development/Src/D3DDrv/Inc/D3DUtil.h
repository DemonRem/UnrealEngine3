/*=============================================================================
	D3DUtil.h: D3D RHI utility definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if USE_D3D_RHI || USE_NULL_RHI

/**
 * A version of FRefCountedObject which implements the COM reference counting interface:
 * AddRef and Release instead of AddRef and RemoveRef.
 */
class FD3DRefCountedObject
{
public:
	FD3DRefCountedObject(): NumRefs(0) {}
	virtual ~FD3DRefCountedObject() { check(!NumRefs); }
	void AddRef() const
	{
		NumRefs++;
	}
	void Release() const
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
 * Encapsulates a reference to a D3D COM object.
 */
template<typename ReferencedType>
class TD3DRef
{
	typedef ReferencedType* ReferenceType;
public:

	TD3DRef():
		Reference(NULL)
	{}

	TD3DRef(ReferencedType* InReference,UBOOL bAddRef = TRUE)
	{
		Reference = InReference;
		if(Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}

	TD3DRef(const TD3DRef& Copy)
	{
		Reference = Copy.Reference;
		if(Reference)
		{
			Reference->AddRef();
		}
	}

	~TD3DRef()
	{
		if(Reference)
		{
			Reference->Release();
		}
	}

	TD3DRef& operator=(ReferencedType* InReference)
	{
		ReferencedType* OldReference = Reference;
		Reference = InReference;
		if(Reference)
		{
			Reference->AddRef();
		}
		if(OldReference)
		{
			OldReference->Release();
		}
		return *this;
	}

	TD3DRef& operator=(const TD3DRef& InPtr)
	{
		return *this = InPtr.Reference;
	}

	UBOOL operator==(const TD3DRef& Other) const
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

	ReferencedType** GetInitReference()
	{
		*this = NULL;
		return &Reference;
	}

	// RHI reference interface.

	friend UBOOL IsValidRef(const TD3DRef& Ref)
	{
		return Ref.Reference != NULL;
	}

	void Release()
	{
		*this = NULL;
	}

private:
	ReferencedType* Reference;
};

/**
 * Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
 * @param	Result - The result code to check
 * @param	Code - The code which yielded the result.
 * @param	Filename - The filename of the source file containing Code.
 * @param	Line - The line number of Code within Filename.
 */
extern void VerifyD3DResult(HRESULT Result,const ANSICHAR* Code,const ANSICHAR* Filename,UINT Line);

/**
* Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
* @param	Result - The result code to check
* @param	Code - The code which yielded the result.
* @param	Filename - The filename of the source file containing Code.
* @param	Line - The line number of Code within Filename.	
*/
extern void VerifyD3DCreateTextureResult(HRESULT D3DResult,const ANSICHAR* Code,const ANSICHAR* Filename,UINT Line,
										 UINT SizeX,UINT SizeY,BYTE D3DFormat,UINT NumMips,DWORD Flags);

/**
 * A macro for using VerifyD3DResult that automatically passes in the code and filename/line.
 */
#if DO_CHECK
#define VERIFYD3DRESULT(x) VerifyD3DResult(x,#x,__FILE__,__LINE__);
#define VERIFYD3DCREATETEXTURERESULT(x,SizeX,SizeY,Format,NumMips,Flags) VerifyD3DCreateTextureResult(x,#x,__FILE__,__LINE__,SizeX,SizeY,Format,NumMips,Flags);
#else
#define VERIFYD3DRESULT(x) (x);
#define VERIFYD3DCREATETEXTURERESULT(x,SizeX,SizeY,Format,NumMips,Flags) (x);
#endif

/**
* Convert from ECubeFace to D3DCUBEMAP_FACES type
* @param Face - ECubeFace type to convert
* @return D3D cube face enum value
*/
FORCEINLINE D3DCUBEMAP_FACES GetD3DCubeFace(ECubeFace Face)
{
	switch(Face)
	{
	case CubeFace_PosX:
	default:
		return D3DCUBEMAP_FACE_POSITIVE_X;
	case CubeFace_NegX:
		return D3DCUBEMAP_FACE_NEGATIVE_X;
	case CubeFace_PosY:
		return D3DCUBEMAP_FACE_POSITIVE_Y;
	case CubeFace_NegY:
		return D3DCUBEMAP_FACE_NEGATIVE_Y;
	case CubeFace_PosZ:
		return D3DCUBEMAP_FACE_POSITIVE_Z;
	case CubeFace_NegZ:
		return D3DCUBEMAP_FACE_NEGATIVE_Z;
	};
}

#endif

