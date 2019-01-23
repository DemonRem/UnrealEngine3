/**
 * cast this 
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


// Dynamically cast an object type-safely.
//class UClass;


template< typename T > T* Cast( UObject* Src )
{
	return Src && Src->IsA(T::StaticClass()) ? (T*)Src : NULL;
}

template< class T > FORCEINLINE T* Cast( UObject* Src, EClassFlags Flag )
{
	return Src && (Src->GetClass()->ClassFlags & Flag) ? (T*)Src : NULL;
}

template< class T > FORCEINLINE T* ExactCast( UObject* Src )
{
	return Src && (Src->GetClass() == T::StaticClass()) ? (T*)Src : NULL;
}

template< class T > FORCEINLINE const T* ExactCastConst( const UObject* Src )
{
	return Src && (Src->GetClass() == T::StaticClass()) ? (T*)Src : NULL;
}


template< class T, class U > T* CastChecked( U* Src )
{
#if !FINAL_RELEASE
	if( !Src || !Src->IsA(T::StaticClass()) )
	{
		appErrorf( TEXT("Cast of %s to %s failed"), Src ? *Src->GetFullName() : TEXT("NULL"), *T::StaticClass()->GetName() );
	}
#endif
	return (T*)Src;
}
template< class T > const T* ConstCast( const UObject* Src )
{
	return Src && Src->IsA(T::StaticClass()) ? (T*)Src : NULL;
}

template< class T > FORCEINLINE const T* ConstCast( const UObject* Src, EClassFlags Flag )
{
	return Src && (Src->GetClass()->ClassFlags & Flag) ? (T*)Src : NULL;
}

template< class T, class U > const T* ConstCastChecked( const U* Src )
{
#if !FINAL_RELEASE
	if( !Src || !Src->IsA(T::StaticClass()) )
	{
		appErrorf( TEXT("Cast of %s to %s failed"), Src ? *Src->GetFullName() : TEXT("NULL"), *T::StaticClass()->GetName() );
	}
#endif
	return (T*)Src;
}




