/**
 * This class allows designers to change the value of a data field in a data store.  Whichever object
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIAction_SetDatafieldValue extends UIAction_DataStoreField
	native(inherit);

cpptext
{
	/**
	 * Resolves the datastore specified by DataFieldMarkupString, and copies the value from DataFieldStringValue/ImageValue
	 * to the resolved data provider.
	 */
	virtual void Activated();
}

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a string.
 */
var()			string			DataFieldStringValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to an image.
 */
var()			Surface			DataFieldImageValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a collection.
 */
var()			array<int>		DataFieldArrayValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a range value.
 */
var()			UIRoot.UIRangeData	DataFieldRangeValue;

/**
 * If TRUE, immediately calls Commit on the owning data store.
 */
var()			bool			bCommitValueImmediately;

DefaultProperties
{
	ObjName="Set Datastore Value"
	ObjClassVersion=5

	// add a variable link which will receive the value of the datafield if it corresonds to string data
	VariableLinks.Add((ExpectedType=class'SeqVar_String',LinkDesc="String Value",PropertyName=DataFieldStringValue,MaxVars=1))

	// add a variable link which will receive the value of the datafield, if it corresponds to image data
	VariableLinks.Add((ExpectedType=class'SeqVar_Object',LinkDesc="Image Value",PropertyName=DataFieldImageValue,MaxVars=1))

	// add a variable link which will receive the value of the datafield, if it corresponds to array data
	//@todo ronp - support this
//	VariableLinks.Add((ExpectedType=class'SeqVar_Array',LinkDesc="Image Value",PropertyName=DataFieldArrayValue,MaxVars=1))

	// add a variable link which will receive the avlue of the datafield if it corresponds to range data
	VariableLinks.Add((ExpectedType=class'SeqVar_UIRange',LinkDesc="Range Value",PropertyName=DataFieldRangeValue,MaxVars=1))

	OutputLinks.Add((LinkDesc="Success"))
}
