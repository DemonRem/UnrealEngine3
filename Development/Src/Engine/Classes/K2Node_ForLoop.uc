/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

class K2Node_ForLoop extends K2Node_Code
	native(K2);


cpptext
{
#if WITH_EDITOR
	virtual FString GetDisplayName();

	virtual void GetCodeText(struct FK2CodeGenContext& Context, TArray<struct FK2CodeLine>& OutCode);	

	virtual void CreateAutoConnectors();
#endif
};

