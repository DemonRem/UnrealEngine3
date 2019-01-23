/**
 * Resonsible for how the data associated with this list is presented.  Updates the list's operating parameters
 * (CellHeight, CellWidth, etc.) according to the presentation type for the data contained by this list.
 *
 * Routes render messages from the list to the individual elements, adding any additional data necessary for the
 * element to understand how to render itself.  For example, a listdata component might add that the element being
 * rendered is the currently selected element, so that the element can adjust the way it renders itself accordingly.
 * For a tree-type list, the listdata component might add whether the element being drawn is currently open, has
 * children, etc.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIComp_ListPresenter extends UIComp_ListComponentBase
	native(inherit)
	DependsOn(UIDataStorePublisher);

/**
 * Corresponds to a single cell in a UIList (intersection of a row and column).  Generally maps directly to a
 * single item in the list, but in the case of multiple columns or rows, a single list item may be associated with
 * multiple UIListElementCells (where each column for that row is represented by a UIListElementCell).
 *
 * The data for a UIListElementCell is accessed using a UIString. Contains one UIListCellRegion per UIStringNode
 * in the UIString, which can be configured to manually controls the extent for each UIStringNode.
 */
struct native UIListElementCell
{
	/** pointer to the UIListElement that contains this UIListElementCell */
	var	const	native	transient	pointer				ContainerElement{struct FUIListItem};

	/** pointer to the list that contains this element cell */
	var	const	transient	UIList						OwnerList;

	/** A UIString which contains data for this cell */
	var	transient			UIListString				ValueString;

	/**
	 * The padding to apply to this cell.  The values specified here will reduce the space available
	 * for the region to be rendered in.
	 *
	 * @todo - this was originally designed to be a UIScreenValue_Bounds....is that necessary?
	 * @todo - PostEditChange for this
	 */
	var(Presentation)		float						Padding[EUIWidgetFace.UIFACE_MAX];

	/** The amount of scaling to apply to this cell. */
	var(Presentation)		float						Scaling[EUIOrientation.UIORIENT_MAX];

	/**
	 * Allows the designer to specify a different style for each cell in a column/row
	 */
	var						UIStyleReference			CellStyle[EUIListElementState.ELEMENT_MAX];

structcpptext
{
	/** Script Constructors */
	FUIListElementCell() {}
	FUIListElementCell(EEventParm);

	/**
	 * Called when this cell is created while populating the elements for the owning list. Creates the cell's UIListString.
	 */
	void OnCellCreated( struct FUIListItem* InContainerElement, class UUIList* inOwnerList );

	/**
	 * Resolves the value of the specified tag from the DataProvider and assigns the result to this cell's ValueString.
	 *
	 * @param	DataSource		the data source to use for populating this cell's data
	 * @param	CellBindingTag	the tag (from the list supported by DataProvider) that should be associated with this
	 *							UIListElementCell.
	 *
	 * @note: even though this method is overridden in FUIListElementCellTemplate, it is intended to be non-virtual!
	 */
	void AssignBinding( struct FUIListItemDataBinding& DataSource, FName CellBindingTag );

	/**
	 * Resolves the CellStyle for the specified element state using the currently active skin.  This function is called
	 * anytime the cached cell style no longer is out of date, such as when the currently active skin has been changed.
	 *
	 * @param	ElementState	the list element state to update the element style for
	 */
	void ResolveCellStyles( EUIListElementState ElementState );

	/**
	 * Propagates the style data for the current menu state and element state to each cell .  This function is called anytime
	 * the style data that is applied to each cell is no longer valid, such as when the cell's CellState changes or when the
	 * owning list's menu state is changed.
	 *
	 * @param	ElementState	the list element state to update the element style for
	 */
	void ApplyCellStyleData( EUIListElementState ElementState );
}

structdefaultproperties
{
	CellStyle(ELEMENT_Normal)=(RequiredStyleClass=class'Engine.UIStyle_Combo')
	CellStyle(ELEMENT_Active)=(RequiredStyleClass=class'Engine.UIStyle_Combo')
	CellStyle(ELEMENT_Selected)=(RequiredStyleClass=class'Engine.UIStyle_Combo')
	CellStyle(ELEMENT_UnderCursor)=(RequiredStyleClass=class'Engine.UIStyle_Combo')
}
};


/**
 * Contains the data binding information for a single row or column in a list.  Also used for rendering the list's column
 * headers, if configured to do so.
 */
struct native UIListElementCellTemplate extends UIListElementCell
{
	/**
	 * Contains the data binding for each cell group in the list (row if columns are linked, columns if
	 * rows are linked, individual cells if neither are linked
	 */
	var()	editinline editconst	name				CellDataField;

	/**
	 * The string that should be rendered in the header for the column which displays the data for this cell.
	 */
	var()				editconst	string				ColumnHeaderText;

	/**
	 * The custom size for the linked cell (column/row).  A value of 0 indicates that the row/column's size should be
	 * controlled by the owning list according to its cell auto-size configuration.
	 */
	var()					UIScreenValue_Extent		CellSize;

	/**
	 * The starting position of this cell, in absolute pixels.
	 */
	var								float				CellPosition;

structcpptext
{
	/** Script Constructor */
	FUIListElementCellTemplate() {}
	FUIListElementCellTemplate(EEventParm);

	/**
	 * Called when this cell is created while populating the elements for the owning list. Creates the cell's UIListString.
	 */
	void OnCellCreated( class UUIList* inOwnerList );

	/**
	 * Initializes the specified cell based on this cell template.
	 *
	 * @param	DataSource		the information about the data source for this element
	 * @param	TargetCell		the cell to initialize.
	 */
	void InitializeCell( struct FUIListItemDataBinding& DataSource, struct FUIListElementCell& TargetCell );

	/**
	 * Resolves the value of the specified tag from the DataProvider and assigns the result to this cell's ValueString.
	 *
	 * @param	DataProvider	the object which contains the data for this element cell.
	 * @param	CellBindingTag	the tag (from the list supported by DataProvider) that should be associated with this
	 *							UIListElementCell.
	 * @param	ColumnHeader	the string that should be displayed in the column header for this cell.
	 */
	void AssignBinding( TScriptInterface<class IUIListElementCellProvider> DataProvider, FName CellBindingTag, const FString& ColumnHeader );

	/**
	 * Applies the resolved style data for the column header style to the schema cells' strings.  This function is called anytime
	 * the header style data that is applied to the schema cells is no longer valid, such as when the owning list's menu state is changed.
	 *
	 * @param	ResolvedStyle			the style resolved by the style reference
	 */
	void ApplyHeaderStyleData( UUIStyle* ResolvedStyle );
}
};

struct native UIListItemDataBinding
{
	/**
	 * The data provider that contains the data for this list element
	 */
	var	UIListElementCellProvider	DataSourceProvider;

	/**
	 * The name of the field from DataSourceProvider that contains the array of data corresponding to this list element
	 */
	var	name						DataSourceTag;

	/**
	 * The index into the array [DataSourceTag] in DataSourceProvider that this list element represents.
	 */
	var	int							DataSourceIndex;

structcpptext
{
	/** Constructors */
	FUIListItemDataBinding() {}
	FUIListItemDataBinding(EEventParm)
	{
		appMemzero(this, sizeof(FUIListItemDataBinding));
	}

	FUIListItemDataBinding( TScriptInterface<class IUIListElementCellProvider> InDataSource, FName DataTag, INT InIndex )
	: DataSourceProvider(InDataSource)
	, DataSourceTag(DataTag)
	, DataSourceIndex(InIndex)
	{}
}

};

/**
 * Corresponds to a single item in a UIList, which may be any type of data structure.
 *
 * Contains a list of UIListElementCells, which correspond to one or more data fields of the underlying data
 * structure associated with the list item represented by this object.  For linked-column lists, each
 * UIListElementCell is typically associated with a different field from the underlying data structure.
 */
struct native UIListItem
{
	/** The list element associated with the cells contained by this UIElementCellList. */
	var	const						UIListItemDataBinding					DataSource;

	/** the cells associated with this list element */
	var()	editinline editconst editfixedsize	array<UIListElementCell>	Cells;

	/** The current state of this cell (selected, active, etc.) */
	var()	editconst	transient 	noimport EUIListElementState			ElementState;

structcpptext
{
	/** Script Constructors */
	FUIListItem() {}
	FUIListItem(EEventParm)
	{
		appMemzero(this, sizeof(FUIListItem));
	}

	/** Standard ctor */
	FUIListItem( const struct FUIListItemDataBinding& InDataSource );

	/**
	 * Changes the ElementState for this element and refreshes its cell's cached style references based on the new cell state
	 *
	 * @param	NewElementState	the new element state to use.
	 *
	 * @return	TRUE if the element state actually changed.
	 */
	UBOOL SetElementState( EUIListElementState NewElementState );
}
};

/**
 * Contains the data store bindings for the individual cells of a single element in this list.  This struct is used
 * for looking up the data required to fill the cells of a list element when a new element is added.
 */
struct native UIElementCellSchema
{
	/** contains the data store bindings used for creating new elements in this list */
	var() editinline	array<UIListElementCellTemplate>	Cells;

structcpptext
{
	/** Script Constructors */
	FUIElementCellSchema() {}
	FUIElementCellSchema(EEventParm)
	{
		appMemzero(this, sizeof(FUIElementCellSchema));
	}
}
};


/**
 * Contains the formatting information configured for each individual cell in the UI editor.
 * Private/const because changing the value of this property invalidates all data in this, requiring that all data be refreshed.
 */
var()		const private				UIElementCellSchema	ElementSchema;

/**
 * Contains the element cells for each list item.  Each item in the ElementCells array is the list of
 * UIListElementCells for the corresponding element in the list.
 */
var()	editconst	editinline	transient noimport	init array<UIListItem>	ListItems;

/**
 * The image to render over the selected elements - now handled by the ELEMENT_Selected element of the ListItemOverlay array.
 */
var deprecated UITexture			SelectionOverlay;
var deprecated TextureCoordinates	SelectionOverlayCoordinates;

var(Image)	instanced	editinlineuse	UITexture			ListItemOverlay[EUIListElementState.ELEMENT_MAX];

/**
 * the texture atlas coordinates for the SelectionOverlay. Values of 0 indicate that the texture is not part of an atlas.
 */
var(Image)								TextureCoordinates	ListItemOverlayCoordinates[EUIListElementState.ELEMENT_MAX];

/** Controls whether column headers are rendered for this list */
var()		private{private}			bool				bDisplayColumnHeaders;

/** set to indicate that the cells in this list needs to recalculate their extents */
var			transient					bool				bReapplyFormatting;

cpptext
{
	friend class UUIList;

	/**
	 * Called when a new element is added to the list that owns this component.  Creates a UIElementCellList for the specified element.
	 *
	 * @param	InsertIndex			an index in the range of 0 - Items.Num() to use for inserting the element.  If the value is
	 *								not a valid index, the element will be added to the end of the list.
	 * @param	ElementValue		the index [into the data provider's collection] for the element that is being inserted into the list.
	 *
	 * @return	the index where the new element was inserted, or INDEX_NONE if the element wasn't added to the list.
	 */
	virtual INT InsertElement( INT InsertIndex, INT ElementValue );

	/**
	 * Called when an element is removed from the list that owns this component.  Removes the UIElementCellList located at the
	 * specified index.
	 *
	 * @param	RemovalIndex	the index for the element that should be removed from the list
	 *
	 * @return	the index [into the ElementCells array] for the element that was removed, or INDEX_NONE if RemovalIndex was invalid
	 *			or that element couldn't be removed from this list.
	 */
	virtual INT RemoveElement( INT RemovalIndex );

	/**
	 * Swaps the values at the specified indexes, reversing their positions in the ListItems array.
	 *
	 * @param	IndexA	the index into the ListItems array for the first element to swap
	 * @param	IndexB	the index into the ListItems array for the second element to swap
	 *
	 * @param	TRUE if the swap was successful
	 */
	virtual UBOOL SwapElements( INT IndexA, INT IndexB );

	/**
	 * Allows the list presenter to override the menu state that is used for rendering a specific element in the list.  Used for those
	 * lists which need to render some elements using the disabled state, for example.
	 *
	 * @param	ElementIndex		the index into the Elements array for the element to retrieve the menu state for.
	 * @param	out_OverrideState	receives the value of the menu state that should be used for rendering this element. if a specific
	 *								menu state is desired for the specified element, this value should be set to a child of UIState corresponding
	 *								to the menu state that should be used;  only used if the return value for this method is TRUE.
	 *
	 * @return	TRUE if the list presenter assigned a value to out_OverrideState, indicating that the element should be rendered using that menu
	 *			state, regardless of which menu state the list is currently in.  FALSE if the list presenter doesn't want to override the menu
	 *			state for this element.
	 */
	virtual UBOOL GetOverrideMenuState( INT ElementIndex, UClass*& out_OverrideState );

	/**
	 * Resolves the element schema provider based on the owning list's data source binding, and repopulates the element schema based on
	 * the available data fields in that element schema provider.
	 */
	virtual void RefreshElementSchema();

	/**
	 * Assigns the style for the cell specified and refreshes the cell's resolved style.
	 *
	 * @param	NewStyle		the new style to assign to this widget
	 * @param	ElementState	the list element state to set the element style for
	 * @param	CellIndex		indicates the column (if columns are linked) or row (if rows are linked) to apply the style to
	 *
	 * @return	TRUE if the style was successfully applied to the cell.
	 */
	virtual UBOOL SetCustomCellStyle( class UUIStyle* NewStyle, EUIListElementState ElementState, INT CellIndex );

	/**
	 * Applies the resolved style data for the column header style to the schema cells' strings.  This function is called anytime
	 * the header style data that is applied to the schema cells is no longer valid, such as when the owning list's menu state is changed.
	 *
	 * @param	ResolvedStyle			the style resolved by the style reference
	 */
	virtual void ApplyColumnHeaderStyle( UUIStyle* ResolvedStyle );

	/**
	 * Notification that the list's style has been changed.  Updates the cached cell styles for all elements for the specified
	 * list element state.
	 *
	 * @param	ElementState	the list element state to update the element style for
	 */
	virtual void OnListStyleChanged( EUIListElementState ElementState );

	/**
	 * Notification that the list's menu state has changed.  Reapplies the specified cell style for all elements based on the
	 * new menu state.
	 *
	 * @param	ElementState	the list element state to update the element style for
	 */
	virtual void OnListMenuStateChanged( EUIListElementState ElementState );

	/**
	 * Renders the elements in this list.
	 *
	 * @param	RI					the render interface to use for rendering
	 */
	virtual void Render_List( FCanvas* Canvas );

	/**
	 * Notifies the owning widget that the formatting and render parameters for the list need to be updated.
	 */
	virtual void ReapplyFormatting();

	/**
	 * Adds the specified face to the owning scene's DockingStack for the owning widget.  Takes list configuration into account,
	 * ensuring that all widget faces are added to the scene's docking stack in the appropriate order.
	 *
	 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
	 * @param	Face			the face that should be added
	 *
	 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
	 *			existed in the stack for the specified face of this widget.
	 */
	UBOOL AddDockingNode( TArray<struct FUIDockingNode>& DockingStack, EUIWidgetFace Face );

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Adjusts the owning widget's bounds
	 * according to the wrapping mode and autosize behaviors.
	 *
	 * @param	Face	the face that should be resolved
	 */
	void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Returns the number of rows the list can dislay
	 */
	INT GetMaxNumVisibleRows() const;

	/**
	 * Returns the number of columns the list can display
	 */
	INT GetMaxNumVisibleColumns() const;

	/**
	 * Returns the total number of rows in this list.
	 */
	INT GetTotalRowCount() const;

	/**
	 * Returns the total number of columns in this list.
	 */
	INT GetTotalColumnCount() const;

	/**
	 * Changes the cell state for the specified element.
	 *
	 * @param	ElementIndex	the index of the element to change states for
	 * @param	NewElementState	the new state to place the element in
	 *
	 * @return	TRUE if the new state was successfully applied to the new element, FALSE otherwise.
	 */
	UBOOL SetElementState( INT ElementIndex, EUIListElementState NewElementState );

	/**
	 * Returns whether element size is determined by the elements themselves.  For lists with linked columns, returns whether
	 * the item height is autosized; for lists with linked rows, returns whether item width is autosized.
	 */
	UBOOL IsElementAutoSizingEnabled() const;

protected:
	/**
	 * Determines the maximum number of elements which can be rendered given the owning list's bounding region.
	 */
	virtual void CalculateVisibleElements( struct FRenderParameters& Parameters );

	/**
	 * Initializes the render parameters that will be used for formatting the list elements.
	 *
	 * @param	Face			the face that was being resolved
	 * @param	out_Parameters	[out] the formatting parameters to use when calling ApplyFormatting.
	 *
	 * @return	TRUE if the formatting data is ready to be applied to the list elements, taking into account the autosize settings.
	 */
	virtual UBOOL GetListRenderParameters( EUIWidgetFace Face, FRenderParameters& out_Parameters ) const;

	/**
	 * Adjusts the owning widget's bounds according to the autosize settings.
	 */
	virtual void UpdateOwnerBounds( struct FRenderParameters& Parameters );

	/**
	 * Calculates the maximum number of visible elements and calls ApplyElementFormatting for all elements.
	 *
	 * @param	Parameters		@see UUIString::ApplyFormatting())
	 */
	virtual void ApplyListFormatting( struct FRenderParameters& Parameters );

	/**
	 * Updates the formatting parameters for all cells of the specified element.
	 *
	 * @param	ElementIndex	the list element to apply formatting for.
	 * @param	Parameters		@see UUIString::ApplyFormatting())
	 */
	virtual void ApplyElementFormatting( INT ElementIndex, struct FRenderParameters& Parameters );

	/**
	 * Wrapper for applying formatting to the schema cells.
	 */
	void FormatSchemaCells( struct FRenderParameters& Parameters );

	/**
	 * Updates the formatting parameters for all cells of the specified element.
	 *
	 * @param	Cells			the list of cells to render
	 * @param	Parameters		@see UUIString::ApplyFormatting())
	 */
	virtual void ApplyCellFormatting( TArray<struct FUIListElementCell*> Cells, struct FRenderParameters& Parameters );

	/**
	 * Renders the list element specified.
	 *
	 * @param	Canvas			the canvas to use for rendering
	 * @param	ElementIndex	the list element to render
	 * @param	Parameters		Used for various purposes:
	 *							DrawX:		[in]	specifies the pixel location of the start of the horizontal bounding region that should be used for
	 *												rendering this element
	 *										[out]	unused
	 *							DrawY:		[in]	specifies the pixel Y location of the bounding region that should be used for rendering this list element.
	 *										[out]	Will be set to the Y position of the rendering "pen" after rendering this element.  This is the Y position for rendering
	 *												the next element should be rendered
	 *							DrawXL:		[in]	specifies the pixel location of the end of the horizontal bounding region that should be used for rendering this element.
	 *										[out]	unused
	 *							DrawYL:		[in]	specifies the height of the bounding region, in pixels.  If this value is not large enough to render the specified element,
	 *												the element will not be rendered.
	 *										[out]	Will be reduced by the height of the element that was rendered. Thus represents the "remaining" height available for rendering.
	 *							DrawFont:	[in]	specifies the font to use for retrieving the size of the characters in the string
	 *							Scale:		[in]	specifies the amount of scaling to apply when rendering the element
	 */
	virtual void Render_ListElement( FCanvas* Canvas, INT ElementIndex, struct FRenderParameters& Parameters );

	/**
	 * Renders the list element cells specified.
	 *
	 * @param	Canvas			the canvas to use for rendering
	 * @param	Cells			the list of cells to render
	 * @param	Parameters		Used for various purposes:
	 *							DrawX:		[in]	specifies the location of the start of the horizontal bounding region that should be used for
	 *												rendering the cells, in absolute screen pixels
	 *										[out]	unused
	 *							DrawY:		[in]	specifies the location of the start of the vertical bounding region that should be used for rendering
	 *												the cells, in absolute screen pixels
	 *										[out]	Will be set to the Y position of the rendering "pen" after rendering all cells.
	 *							DrawXL:		[in]	specifies the location of the end of the horizontal bounding region that should be used for rendering this element, in absolute screen pixels
	 *										[out]	unused
	 *							DrawYL:		[in]	specifies the height of the bounding region, in absolute screen pixels.  If this value is not large enough to render the cells, they will not be
	 *												rendered
	 *										[out]	Will be reduced by the height of the cells that were rendered. Thus represents the "remaining" height available for rendering.
	 *							DrawFont:	[in]	specifies the font to use for retrieving the size of the characters in the string
	 *							Scale:		[in]	specifies the amount of scaling to apply when rendering the cells
	 */
	virtual void Render_Cells( FCanvas* Canvas, const TArray<struct FUIListElementCell*> Cells, struct FRenderParameters& CellParameters );

	/**
	 * Changes the data binding for the specified cell index.
	 *
	 * @param	CellDataBinding		a name corresponding to a tag from the UIListElementProvider currently bound to this list.
	 * @param	ColumnHeader		the string that should be displayed in the column header for this cell.
	 * @param	BindingIndex		the column or row to bind this data field to.  If BindingIndex is greater than the number
	 *								schema cells, empty schema cells will be added to meet the number required to place the data
	 *								at BindingIndex.
	 *								If a value of INDEX_NONE is specified, the cell binding will only occur if there are no other
	 *								schema cells bound to that data field.  In this case, a new schema cell will be appended and
	 *								it will be bound to the data field specified.
	 */
	virtual UBOOL SetCellBinding( FName CellDataBinding, const FString& ColumnHeader, INT BindingIndex );

	/**
	 * Inserts a new schema cell at the specified index and assigns the data binding.
	 *
	 * @param	InsertIndex			the column/row to insert the schema cell; must be a valid index.
	 * @param	CellDataBinding		a name corresponding to a tag from the UIListElementProvider currently bound to this list.
	 * @param	ColumnHeader	the string that should be displayed in the column header for this cell.
	 *
	 * @return	TRUE if the schema cell was successfully inserted into the list
	 */
	virtual UBOOL InsertSchemaCell( INT InsertIndex, FName CellDataBinding, const FString& ColumnHeader );

	/**
	 * Removes all schema cells which are bound to the specified data field.
	 *
	 * @return	TRUE if one or more schema cells were successfully removed.
	 */
	virtual UBOOL ClearCellBinding( FName CellDataBinding );

	/**
	 * Removes schema cells at the location specified.  If the list's columns are linked, this index should correspond to
	 * the column that should be removed; if the list's rows are linked, this index should correspond to the row that should
	 * be removed.
	 *
	 * @return	TRUE if the schema cell at BindingIndex was successfully removed.
	 */
	virtual UBOOL ClearCellBinding( INT BindingIndex );

public:

	/* === UObject interface === */
	/**
	 * Called when a property value has been changed in the editor.  When the data source for the cell schema is changed,
	 * refreshes the list's data.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called when a member property value has been changed in the editor.
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Copies the value of the deprecated SelectionOverlay/Coordinates into the appropriate element of the ItemOverlay array.
	 */
	virtual void PostLoad();
}

/**
 * Changes whether this list renders colum headers or not.  Only applicable if the owning list's CellLinkType is LINKED_Columns
 */
native final function EnableColumnHeaderRendering( bool bShouldRenderColHeaders=true );

/**
 * Returns whether this list should render column headers
 */
native final function bool ShouldRenderColumnHeaders() const;

/**
 * Returns whether the list's bounds will be adjusted for the specified orientation considering the list's configured
 * autosize and cell link type values.
 *
 * @param	Orientation		the orientation to check auto-sizing for
 */
native final function bool ShouldAdjustListBounds( EUIOrientation Orientation ) const;

/**
 * Returns the object that provides the cell schema for this component's owner list (usually the class default object for
 * the class of the owning list's list element provider)
 */
native final function UIListElementCellProvider GetCellSchemaProvider() const;

/**
 * Find the index of the list item which corresponds to the data element specified.
 *
 * @param	DataSourceIndex		the index into the list element provider's data source collection for the element to find.
 *
 * @return	the index [into the ListItems array] for the element which corresponds to the data element specified, or INDEX_NONE
 * if none where found or DataSourceIndex is invalid.
 */
native final function int FindElementIndex( int DataSourceIndex ) const;

DefaultProperties
{
	bDisplayColumnHeaders=true
	bReapplyFormatting=true

	Begin Object Class=UITexture Name=NormalOverlayTemplate
	End Object
	Begin Object Class=UITexture Name=ActiveOverlayTemplate
	End Object
	Begin Object Class=UITexture Name=SelectionOverlayTemplate
	End Object
	Begin Object Class=UITexture Name=HoverOverlayTemplate
	End Object

	ListItemOverlay(ELEMENT_Normal)=NormalOverlayTemplate
	ListItemOverlay(ELEMENT_Active)=ActiveOverlayTemplate
	ListItemOverlay(ELEMENT_Selected)=SelectionOverlayTemplate
	ListItemOverlay(ELEMENT_UnderCursor)=HoverOverlayTemplate
}
