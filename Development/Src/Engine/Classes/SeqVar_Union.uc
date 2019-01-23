/**
 * This class is used for linking variables of different types.  It contains a variable of each supported type and can
 * be connected to most types of variables links.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class SeqVar_Union extends SequenceVariable
	native(inherit);

cpptext
{
	virtual INT* GetIntRef();
	virtual UBOOL* GetBoolRef();
	virtual FLOAT* GetFloatRef();
	virtual FString* GetStringRef();
	virtual UObject** GetObjectRef( INT Idx );
	virtual FString GetValueStr();

	/**
	 * Union should never be used as the ExpectedType in a variable link, so it doesn't support any property classes.
	 */
	virtual UBOOL SupportsProperty(UProperty *Property);

	/**
	 * Copies the value stored by this SequenceVariable to the SequenceOp member variable that it's associated with.
	 *
	 * @param	Op			the sequence op that contains the value that should be copied from this sequence variable
	 * @param	Property	the property in Op that will receive the value of this sequence variable
	 * @param	VarLink		the variable link in Op that this sequence variable is linked to
	 */
	virtual void PublishValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);

	/**
	 * Copy the value from the member variable this VariableLink is associated with to this VariableLink's value.
	 *
	 * @param	Op			the sequence op that contains the value that should be copied to this sequence variable
	 * @param	Property	the property in Op that contains the value to copy into this sequence variable
	 * @param	VarLink		the variable link in Op that this sequence variable is linked to
	 */
	virtual void PopulateValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);

	/**
	 * Allows the sequence variable to execute additional logic after copying values from the SequenceOp's members to the sequence variable.
	 *
	 * @param	SourceOp	the sequence op that contains the value that should be copied to this sequence variable
	 * @param	VarLink		the variable link in Op that this sequence variable is linked to
	 */
	virtual void PostPopulateValue( USequenceOp* SourceOp, FSeqVarLink& VarLink );
}

/**
 * The list of sequence variable classes that are supported by SeqVar_Union
 */
var		array<class<SequenceVariable> >	SupportedVariableClasses;

var()	int			IntValue;
var()	int			BoolValue;
var()	float		FloatValue;
var()	string		StringValue;
var()	Object		ObjectValue;

DefaultProperties
{
	ObjName="Union"
	ObjColor=(R=255,G=255,B=255,A=255)

	SupportedVariableClasses(0)=class'SeqVar_Bool'
	SupportedVariableClasses(1)=class'SeqVar_Int'
	SupportedVariableClasses(2)=class'SeqVar_Object'
	SupportedVariableClasses(3)=class'SeqVar_String'
	SupportedVariableClasses(4)=class'SeqVar_Float'
}
