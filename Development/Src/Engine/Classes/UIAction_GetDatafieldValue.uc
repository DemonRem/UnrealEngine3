/**
 * This class allows designers to retrieve the value of data fields from a data store.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIAction_GetDatafieldValue extends UIAction_DataStoreField
	native(inherit);

cpptext
{
	/**
	 * Resolves the datastore specified by DataFieldMarkupString, and copies the value the resolved data provider to
	 * DataFieldStringValue/ImageValue.
	 */
	virtual void Activated();
}

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a string.
 */
var				string			DataFieldStringValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to an image.
 */
var				Surface			DataFieldImageValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a collection.
 */
var				array<int>		DataFieldArrayValue;

/**
 * The value of the specified field in the resolved data provider, if the datafield's value corresponds to a range value.
 */
var				UIRoot.UIRangeData	DataFieldRangeValue;

DefaultProperties
{
	ObjName="Get Datastore Value"
	ObjClassVersion=5

	// add a variable link which will receive the value of the datafield if it corresonds to string data
	VariableLinks.Add((ExpectedType=class'SeqVar_String',LinkDesc="String Value",PropertyName=DataFieldStringValue,bWriteable=true))

	// add a variable link which will receive the value of the datafield, if it corresponds to image data
	VariableLinks.Add((ExpectedType=class'SeqVar_Object',LinkDesc="Image Value",PropertyName=DataFieldImageValue,bWriteable=true))

	// add a variable link which will receive the value of the datafield, if it corresponds to array data
	//@todo ronp - support this
//	VariableLinks.Add((ExpectedType=class'SeqVar_Array',LinkDesc="Array Value",PropertyName=DataFieldArrayValue,MaxVars=1,bWriteable=true))

	// add a variable link which will receive the value of the datafield if it corresponds to range data
	VariableLinks.Add((ExpectedType=class'SeqVar_UIRange',LinkDesc="Range Value",PropertyName=DataFieldRangeValue,MaxVars=1,bWriteable=true))

	OutputLinks.Add((LinkDesc="String Value"))
	OutputLinks.Add((LinkDesc="Image Value"))
	OutputLinks.Add((LinkDesc="Array Value"))
	OutputLinks.Add((LinkDesc="Range Value"))
}
