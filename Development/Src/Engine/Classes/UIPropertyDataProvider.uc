/**
 * Base class for data providers which provide data pulled directly from member UProperties.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIPropertyDataProvider extends UIDataProvider
	native(inherit)
	abstract;

/**
 * the list of property classes for which values cannot be automatically derived; if your script-only child class has a member
 * var of one of these types, you'll need to provide the value yourself via the GetCustomPropertyValue event
 */
var const	array<class<Property> >		ComplexPropertyTypes;

cpptext
{
	/* === UUIPropertyDataProvider interface === */
	/**
	 * Returns whether the specified property type is renderable in the UI.
	 *
	 * @return	TRUE if this property type is something that can be rendered in the UI.
	 */
	virtual UBOOL IsValidProperty( UProperty* Property ) const;

	/**
	 * Builds a list of UProperties that are flagged for exposure to data stores from the specified class.
	 *
	 * @param	SourceClass		a pointer to a UClass that contains properties which are marked with the "databinding" keyword.
	 *							Must be a child of the class assigned as the value for DataClass.
	 * @param	out_Properties	will contain pointers to the properties of SourceClass which can be exposed to the data store system.
	 */
	void GetProviderDataBindings( UClass* SourceClass, TArray<UProperty*>& out_Properties );

protected:
	/**
	 * Creates a UIStringNode_Text using the source and render text specified.
	 *
	 * @param	PropertyPathName	the path name for the property this text node will represent.  This value is set as the
	 *								source text for the text node.
	 * @param	RenderString		the text that should will be rendered by this text node.
	 *
	 * @return	a pointer to UIStringNode_Text which will render the string specified.
	 */
	FUIStringNode_Text* CreateTextNode( const FString& PropertyPathName, const TCHAR* RenderString ) const;

	/**
	 * Creates a UIStringNode_Text using the source and render text specified.
	 *
	 * @param	PropertyPathName	the path name for the property this image node will represent.  This value is set as the
	 *								source text for the image node.
	 * @param	RenderImage			the image that should be rendered by this image node.
	 *
	 * @return	a pointer to UIStringNode_Image which will render the image specified.
	 */
	FUIStringNode_Image* CreateImageNode( const FString& PropertyPathName, USurface* RenderImage ) const;
}

/**
 * Gets the value for the property specified.  Child classes only need to override this function if it contains data fields
 * which do not correspond to a member property in the class, or if the data corresponds to a complex data type, such as struct,
 * array, etc.
 *
 * @param	PropertyValue	[in] the name of the property to get the value for.
 *							[out] should be filled with the value for the specified property tag.
 * @param	ArrayIndex		optional array index for use with data collections
*
 * @return	return TRUE if either the StringValue or ImageValue fields of PropertyValue were set by script.
 */
event bool GetCustomPropertyValue( out UIProviderScriptFieldValue PropertyValue, optional int ArrayIndex=INDEX_NONE );

DefaultProperties
{
	ComplexPropertyTypes(0)=class'StructProperty'
	ComplexPropertyTypes(1)=class'MapProperty'
	ComplexPropertyTypes(2)=class'ArrayProperty'
	ComplexPropertyTypes(3)=class'DelegateProperty'
}
