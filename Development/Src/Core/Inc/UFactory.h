/*=============================================================================
	UFactory.h: Factory class definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * An object responsible for creating and importing new objects.
 */
class UFactory : public UObject
{
	DECLARE_ABSTRACT_CLASS(UFactory,UObject,CLASS_Transient|CLASS_Intrinsic,Core)

	// Per-class variables.
	UClass*         SupportedClass;
	UClass*			ContextClass;
	FString			Description;
	TArray<FString> Formats;
	BITFIELD        bCreateNew		: 1;
	BITFIELD        bEditAfterNew	: 1;
	BITFIELD        bEditorImport	: 1;
	BITFIELD		bText			: 1;
	INT				AutoPriority;

	/** List of game names that this factory can be used for (if empty, all games valid) */
	TArray<FString>	ValidGameNames;

	static FString	CurrentFilename;

	// Constructors.
	UFactory();
	void StaticConstructor();

	/**
	 * @return		The object class supported by this factory.
	 */
	UClass* GetSupportedClass();

	/**
	 * Resolves SupportedClass for factories which support multiple classes.
	 * Such factories will have a NULL SupportedClass member.
	 */
	virtual UClass* ResolveSupportedClass();

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UFactory interface.
	virtual UObject* FactoryCreateText( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) {return NULL;}
	virtual UObject* FactoryCreateBinary( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn ) {return NULL;}
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) {return NULL;}
	virtual UBOOL FactoryCanImport( const FFilename& Filename ) { return( FALSE ); }

	// UFactory functions.
	static UObject* StaticImportObject( UClass* Class, UObject* InOuter, FName Name, EObjectFlags Flags, const TCHAR* Filename=TEXT(""), UObject* Context=NULL, UFactory* Factory=NULL, const TCHAR* Parms=NULL, FFeedbackContext* Warn=GWarn );
	UBOOL	ValidForCurrentGame();
};

/**
 * Import an object using a UFactory.
 */
template< class T > T* ImportObject( UObject* Outer, FName Name, EObjectFlags Flags, const TCHAR* Filename=TEXT(""), UObject* Context=NULL, UFactory* Factory=NULL, const TCHAR* Parms=NULL, FFeedbackContext* Warn=GWarn )
{
	return (T*)UFactory::StaticImportObject( T::StaticClass(), Outer, Name, Flags, Filename, Context, Factory, Parms, Warn );
}
