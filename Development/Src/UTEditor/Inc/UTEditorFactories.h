/**
 * UTEditor's factory types.
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef _UTEDITOR_FACTORIES_H_
#define _UTEDITOR_FACTORIES_H_


class UUTMapMusicInfoFactoryNew : public UFactory
{
	DECLARE_CLASS_INTRINSIC(UUTMapMusicInfoFactoryNew,UFactory,CLASS_CollapseCategories,UTEditor);
public:

	void StaticConstructor();
	/**
	* Initializes property values for intrinsic classes.  It is called immediately after the class default object
	* is initialized against its archetype, but before any objects of this class are created.
	*/
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};


#endif // _UTEDITOR_FACTORIES_H_




