/**
 * The "container" component of the UI system's list functionality, which is composed fo three components:
 * data source, container widget, and formatter.
 *
 * The UIList acts as the conduit for list data to the UI.  UIList knows nothing about the type of data it contains.
 * It is responsible for tracking the number of elements it has, the size of each cell, handling input (including
 * tracking which elements are selected, changing the selected element, etc.), adding and removing elements from the
 * list, and passing data back and forth between the data source and the presenter components.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIList extends UIObject
	native(UIPrivate)
	implements(UIDataStorePublisher,CustomPropertyItemHandler);

/**
 * Provides information about which cells the mouse is currently hovering over.
 */
struct native transient CellHitDetectionInfo
{
	/**
	 * the column that was hit; INDEX_NONE if the location did not correspond to a valid column
	 */
	var	int HitColumn;

	/**
	 * the row that was hit; INDEX_NONE if the location did not correspond to a valid row
	 */
	var int HitRow;

	/**
	 * if the hit location was within the region used for resizing a column, indicates the column that will be resized;
	 */
	var int ResizeColumn;

	/**
	 * if the hit location was within the region used for resizing a column, indicates the column that will be resized;
	 */
	var int ResizeRow;

structcpptext
{
	/** Constructors */
	FCellHitDetectionInfo() {}
	FCellHitDetectionInfo(EEventParm)
	{
		appMemzero(this, sizeof(FCellHitDetectionInfo));
	}
}
};

/** how many pixels wide the region is that is used for resizing a column/row */
const ResizeBufferPixels=5;

/**
 * Default height for cells in the list.  A value of 0 indicates that the cell heights are dynamic.
 *
 * If rows are linked, this value is only applied to cells that have a value of 0 for CellSize.
 */
var(Presentation)					UIScreenValue_Extent		RowHeight;

/**
 * Default width for cells in the list.  A value of 0 indicates that the cell widths are dynamic.  Dynamic behavior is as follows:
 * Linked columns: columns are expanded to fill the width of the list
 * Non-linked columns: columns widths will be adjusted to fit the largest string in the list
 *
 * If columns are linked, this value is only applied to cells that have a value of 0 for CellSize.
 */
var(Presentation)					UIScreenValue_Extent		ColumnWidth;

/**
 * Number of pixels between each cell in the list.
 */
var(Presentation)					UIScreenValue_Extent		CellSpacing;

/**
 * Number of pixels to pad the inside of each cell.
 */
var(Presentation)					UIScreenValue_Extent		CellPadding;

/**
 * Index into the Items array for currently active item.  In most cases, the active item will be the same as the selected
 * item.  Active item does not imply selected, however.  A good example of this is a multi-select list that has no selected
 * items, but which has focus - corresponds to the item that would be selected if the user pressed 'space' or 'enter'.
 */
var	transient				int							Index;

/** The index of the element which is located at the beginning of the visible region. */
var	transient				int							TopIndex;

/**
 * Maximum number of items that can be visible at one time in the list.  Calculated using the current size of the list
 * and the list's cells.
 */
var(Presentation) editconst	transient	duplicatetransient int	MaxVisibleItems;

/**
 * Number of columns to display in the list.  How this is set is dependent on the value of CellLinkType.
 *
 * LINKED_None: Whatever value designer specifies is used.  Either the width of the list or the column widths must be dynamic.
 * LINKED_Rows: always the same value as MaxVisibleItems.
 * LINKED_Columns: determined by the number of cells which are bound to data providers.
 */
var(List)	protected{protected}	int					ColumnCount;

/**
 * Number of rows to display in the list.  How this is set is dependent on the value of CellLinkType.
 *
 * LINKED_None:		Whatever value designer specifies is used.  Either the height of the list or the column heights must be dynamic.
 * LINKED_Rows:		determined by the number of cells which are bound to data providers.
 * LINKED_Columns:	always the same value as MaxVisibleItems.
 */
var(List)	protected{protected}	int					RowCount;

/** Different ways to auto-size list cells */
enum ECellAutoSizeMode
{
	/** Auto-sizing not enabled */
	CELLAUTOSIZE_None<DisplayName=None>,

	/**
	 * Cells will be uniformly sized so that all cells can be displayed within the bounds of the list.  The configured
	 * cell size is ignored, and the bounds of the list are not adjusted.
	 */
	CELLAUTOSIZE_Uniform<DisplayName=Uniform Fill>,

	/**
	 * Cells will be sized to fit their contents.  The configured cell size is ignored, and the bounds of the list are
	 * not adjusted.
	 */
	CELLAUTOSIZE_Constrain<DisplayName=Best Fit>,

	/**
	 * Cells will be sized to fit their contents.  The configured cell size is ignored, and the bounds of the list are
	 * adjusted to display all cells
	 */
	CELLAUTOSIZE_AdjustList<DisplayName=Adjust List Bounds>,
};

/** Controls how columns are auto-sized. */
var(List)	ECellAutoSizeMode							ColumnAutoSizeMode;

/** Controls how rows are auto-sized */
var(List)	ECellAutoSizeMode							RowAutoSizeMode;

/** Determines how the cells in this list are linked. */
enum ECellLinkType
{
	/** no linking - one to one mapping between cells and elements */
	LINKED_None<DisplayName=Disabled>,

	/** rows are linked; each column in the list will represent a single element; not yet implemented */
	LINKED_Rows<DisplayName=Span Rows>,

	/** columns are linked; each row in the list represents a single element */
	LINKED_Columns<DisplayName=Span Columns>,
};

/**
 * Controls how the cells are mapped to elements.  If CellLinkType is not LINKED_None, the data provider for this list
 * must be capable of supplying multiple data fields for a single item.
 *
 * @todo - once this functionality is exposed through the UI, this variable should no longer be editable
 */
var(List)					ECellLinkType				CellLinkType;

/** Determines how list elements are wrapped */
enum EListWrapBehavior
{
	/**
	 * no wrapping (default); when the end of the list is reached, the user will not be able to scroll further
	 */
	LISTWRAP_None,

	/**
	 * Smooth wrapping; if space is available after rendering the last element, starts over at the first element and
	 * continues rendering elements until no more space is available.
	 * @todo - not yet implemented
	 */
	LISTWRAP_Smooth,

	/**
	 * Jump wrapping; list stops rendering at the last element, but if the user attempts to scroll past the end of the
	 * list, the index jumps back to the opposite side of the list.
	 */
	LISTWRAP_Jump,
};

/**
 * Controls the wrapping behavior of the list, or what happens when the use attempts to scroll past the last element
 */
var(Presentation)			EListWrapBehavior			WrapType;

/**
 * Determines whether more than one item can be selected at the same time.
 *
 * @todo - not yet implemented
 */
var(List)					bool						bEnableMultiSelect;

/**
 *	Determines if this list will display scrollbars
 */
var(List)					bool						bEnableVerticalScrollbar;

/** set to indicate that the scrollbars need to be initialized after in the ResolveFacePosition call */
var			transient		bool						bInitializeScrollbars;

/**
 * Controls whether items which are "disabled" can be selected.
 */
var(List)					bool						bAllowDisabledItemSelection;

/**
 * Controls how many clicks are required in order to submit the list selected item (triggers the kismet Submit List Selection event).
 * FALSE to require a double-click on an item; FALSE to require only a single click;
 */
var(List)					bool						bSingleClickSubmission<ToolTip=Enable to trigger the Submit List Selection kismet event with only a single click>;

/**
 * Controls whether the item currently under the cursor should be drawn using a different style.
 */
var(List) private{private}	bool						bUpdateItemUnderCursor<ToolTip=Item under the cursor will be in a different state; must be true for the fourth CellStyle to work>;

/**
 *	The UIScrollbar object which is allows the UIList to be scrolled up/down
 */
var							UIScrollbar					VerticalScrollbar;


/** The cell styles that are applied to any cells which do not have a custom cell style configured. */
var							UIStyleReference			GlobalCellStyle[EUIListElementState.ELEMENT_MAX];

/**
 * The style to use for the list's column headers.  If not valid, the GlobalCellStyle for the normal cell state will be
 * used instead
 */
var							UIStyleReference			ColumnHeaderStyle;

/** The style to apply to the SelectionOverlay texture. */
var	deprecated				UIStyleReference			SelectionOverlayStyle;

/**
 * The style to apply to the overlay textures for each cell state.
 */
var							UIStyleReference			ItemOverlayStyle[EUIListElementState.ELEMENT_MAX];

/**
 * if TRUE, the schema fields assigned to each column/row will be rendered, rather than the actual data.
 * Used primarily by the UI edtitor.
 */
var(Debug)	transient		bool						bDisplayDataBindings;

/**
 * The column currently being resized, or INDEX_NONE if no columns are being resized.
 */
var	const	transient		int							ResizeColumn;

/** TRUE if the user clicks on a column header - prevents the OnClick delegate from being fired */
var	const	transient		bool						bSortingList;

// ===============================================
// Data Binding
// ===============================================
/** The data store that this list is bound to */
var(Data)						UIDataStoreBinding		DataSource;

/** the list element provider referenced by DataSource */
var	const	transient			UIListElementProvider	DataProvider;

/**
 * The elements of the list. Corresponds to the array indexes of the whichever array this list's data comes from.
 */
var	const	transient			array<int>				Items;

/**
 * The items which are currently selected.  The int values are the array indexes of the whichever array this list's data comes from.
 *
 * @todo - not yet implemented
 */
var	transient 	public{private}	array<int>				SelectedItems;

// ===============================================
// Components
// ===============================================
/** Component for rendering an optional list background */
// @todo add support for a list background
//var(Presentation) editinline			UIComp_DrawImage			BackgroundImageComponent;

/** Determines how to sort the list's elements. */
var(Presentation) editinline	UIComp_ListElementSorter	SortComponent;

/**
 * Handles the interaction between the list and the list's elements.  Encapsulates any special behavior associated
 * with a particular type of list data and controls how the list formats its data.
 */
var(Data) editinline			UIComp_ListPresenter		CellDataComponent;

// ===============================================
// Sounds
// ===============================================

/** this sound is played when the user clicks or presses A on an item that is enabled */
var(Sound)						name					SubmitDataSuccessCue;
/** this sound is played when the user clicks or presses A on an item that is disabled */
var(Sound)						name					SubmitDataFailedCue;
/** this sound is played when the user decreases the list's index */
var(Sound)						name					DecrementIndexCue;
/** this sound is played when the user increases the list's index */
var(Sound)						name					IncrementIndexCue;
/** this sound is played when the user sorts the list in ascending order */
var(Sound)						name					SortAscendingCue;
/** this sound is played when the user sorts the list in descending order */
var(Sound)						name					SortDescendingCue;

cpptext
{
	friend class UUIComp_ListPresenter;

	/* === UUIList interface === */
	/**
	 * Resolves DataSource into the list element provider that it references.
	 *
	 * @return	a pointer to the list element provider indicated by DataSource, or NULL if it couldn't be resolved.
	 */
	virtual TScriptInterface<class IUIListElementProvider> ResolveListElementProvider();

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
	UBOOL SetCellBinding( FName CellDataBinding, const FString& ColumnHeader, INT BindingIndex );

	/**
	 * Inserts a new schema cell at the specified index and assigns the data binding.
	 *
	 * @param	InsertIndex			the column/row to insert the schema cell; must be a valid index.
	 * @param	CellDataBinding		a name corresponding to a tag from the UIListElementProvider currently bound to this list.
	 * @param	ColumnHeader	the string that should be displayed in the column header for this cell.
	 *
	 * @return	TRUE if the schema cell was successfully inserted into the list
	 */
	UBOOL InsertSchemaCell( INT InsertIndex, FName CellDataBinding, const FString& ColumnHeader );

	/**
	 * Removes all schema cells which are bound to the specified data field.
	 *
	 * @return	TRUE if one or more schema cells were successfully removed.
	 */
	UBOOL ClearCellBinding( FName CellDataBinding );

	/**
	 * Removes schema cells at the location specified.  If the list's columns are linked, this index should correspond to
	 * the column that should be removed; if the list's rows are linked, this index should correspond to the row that should
	 * be removed.
	 *
	 * @return	TRUE if the schema cell at BindingIndex was successfully removed.
	 */
	UBOOL ClearCellBinding( INT BindingIndex );

	/**
	 * Returns the menu state that should be used for rendering the specified element.  By default, returns the list's current
	 * menu state, but might return a different menu state in some special cases (for example, when specific elements should be
	 * rendered as though they were disabled)
	 *
	 * @param	ElementIndex	the index into the Items array for the element to retrieve the menu state for.
	 *
	 * @return	a pointer to the menu state that should be used for rendering the specified element; should correspond to one of the elements
	 *			of the UIList's InactiveStates array.
	 */
	UUIState* GetElementMenuState( INT ElementIndex );

	/**
	 * Refreshes the data for this list from the data store bound via DataSource.
	 *
	 * @param	bResolveDataSource	if TRUE, re-resolves DataSource into DataProvider prior to refilling the list's data
	 *
	 * @return	TRUE if the list data was successfully loaded; FALSE if the data source couldn't be resolved or it didn't
	 *			contain the data indicated by SourceData
	 */
	virtual UBOOL RefreshListData( UBOOL bResolveDataSource=FALSE );

	/**
	 * Retrieves the list of elements from the data provider and adds them to the list.
	 *
	 * @return	TRUE if the list was successfully populated.
	 */
	virtual UBOOL PopulateListElements();

	/**
	 * Inserts a new element into the list at the specified index
	 *
	 * @param	ElementToInsert		the index [into the data provider's data source array] of the element to insert.
	 * @param	InsertIndex			an index in the range of 0 - Items.Num() to use for inserting the element.  If the value is
	 *								not a valid index, the element will be added to the end of the list.
	 * @param	bSkipSorting		specify TRUE to prevent the list from being resorted after this element is added (useful when
	 *								adding items in bulk).
	 *
	 * @return	the index where the new element was inserted, or INDEX_NONE if the element wasn't added to the list.
	 */
	virtual INT InsertElement( INT ElementToInsert, INT InsertIndex=INDEX_NONE, UBOOL bSkipSorting=FALSE );

	/**
	 * Removes the element located at the specified index from the list.
	 *
	 * @param	RemovalIndex	the index for the element that should be removed from the list
	 *
	 * @return	the index [into the Items array] for the element that was removed, or INDEX_NONE if the element wasn't
	 *			part of the list.
	 */
	virtual INT RemoveElementAtIndex( INT RemovalIndex );

	/**
	 * Inserts multiple elements into the list at the specified index
	 *
	 * @param	ElementsToInsert	the elements to insert into the list
	 * @param	InsertIndex			an index in the range of 0 - Items.Num() to use for inserting the elements.  If the value is
	 *								not a valid index, the elements will be added to the end of the list.  Elements will be added
	 *								in the order they appear in the array, so ElementsToInsert(0) will be inserted at InsertIndex,
	 *								ElementsToInsert(1) will be inserted at InsertIndex+1, etc.
	 * @param	bSkipSorting		specify TRUE to prevent the list from being resorted after this element is added (useful when
	 *								adding items in bulk).
	 *
	 * @return	the number of elements that were added to the list
	 */
	virtual INT InsertElements( const TArray<INT>& ElementsToInsert, INT InsertIndex=INDEX_NONE, UBOOL bSkipSorting=FALSE );

	/**
	 * Removes the specified elements from the list.
	 *
	 * @param	ElementsToRemove	the elements to remove from the list (make sure this parameter creates a copy of the array, so
	 *								that the removal algorithm works correctly; i.e. don't change this to a const& for performance or something)
	 *
	 * @return	the number of elements that were removed from the list
	 */
	virtual INT RemoveElements( const TArray<INT>& ElementsToRemove );

	/**
	 * Clears all elements from the list.
	 */
	virtual void ClearElements();

	/**
	 * Moves the specified element by the specified number of items.
	 *
	 * @param	ElementToMove	the element to move. This is not an index into the Items array; rather, it is the value of an element
	 *							in the Items array, which corresponds to an index into data store collection this list is bound to.
	 * @param	MoveCount		the number of items to move the element.
	 *
	 * @param	TRUE if the element was moved successfully; FALSE otherwise
	 */
	virtual UBOOL MoveElement( INT ElementToMove, INT MoveCount );

	/**
	 * Moves the element at the specified index by the specified number of items.
	 *
	 * @param	ElementIndex	the index for the element to move.
	 * @param	MoveCount		the number of items to move the element.
	 *
	 * @param	TRUE if the element was moved successfully; FALSE otherwise
	 */
	virtual UBOOL MoveElementAtIndex( INT ElementIndex, INT MoveCount );

	/**
	 * Swaps the elements specified, reversing their positions in the Items array.
	 *
	 * @param	ElementA	the first element to swap. This is not an index into the Items array; rather, it is the value of an element
	 *						in the Items array, which corresponds to an index into data store collection this list is bound to.
	 * @param	ElementB	the second element to swap. This is not an index into the Items array; rather, it is the value of an element
	 *						in the Items array, which corresponds to an index into data store collection this list is bound to.
	 *
	 * @param	TRUE if the swap was successful
	 */
	virtual UBOOL SwapElementsByValue( INT ElementA, INT ElementB );

	/**
	 * Swaps the values at the specified indexes, reversing their positions in the Items array.
	 *
	 * @param	IndexA	the index into the Items array for the first element to swap
	 * @param	IndexB	the index into the Items array for the second element to swap
	 *
	 * @param	TRUE if the swap was successful
	 */
	virtual UBOOL SwapElementsByIndex( INT IndexA, INT IndexB );

	/**
	 * Finds the index for the element specified
	 *
	 * @param	ElementToFind	the element to search for
	 *
	 * @return	the index [into the Items array] for the element specified, or INDEX_NONE if the element wasn't
	 *			part of the list.
	 */
	virtual INT FindElementIndex( INT ElementToFind ) const;

	/**
	 * Calculates the row/column location of the cursor.
	 *
	 * @param	HitLocation		the point to use for calculating the hit information.
	 * @param	out_HitInfo		receives the results of the calculation.  The row/column that was hit does not necessarily
	 *							correspond to an actual item in the list (i.e. the row/column may be higher than the actual
	 *							number of rows or columns).
	 *
	 * @return	TRUE if HitLocation was located inside this list.
	 */
	virtual UBOOL CalculateCellFromPosition( const FIntPoint& HitLocation, struct FCellHitDetectionInfo& out_HitInfo ) const;

	/**
	 * If the mouse is over a column boundary, returns the index of the column that would be resized, or INDEX_NONE if the mouse is not
	 * hovering over a column boundary.
	 *
	 * @param	CellHitDetectionInfo	will be filled with information about which cells the cursor is currently over
	 *
	 * @return	if the cursor is within ResizeBufferPixels of a column boundary, the index of the column the left of the cursor; INDEX_NONE
	 *			otherwise.
	 */
	virtual INT GetResizeColumn( struct FCellHitDetectionInfo* CellHitDetectionInfo=NULL ) const;


	/**
	 *	Initializes the vertical and horizontal scrollbars for the current state of the List
	 */
	void InitializeScrollbars();

	/**
	 *	Sets up the positions of scrollbar markers according to which items are currently visible
	 */
	void UpdateScrollbars();

protected:
	/**
	 * Utility function for modifying the index in response to keyboard or gamepad input.
	 *
	 * @param	bIncrementIndex		TRUE if the index should be increased, FALSE if the index should be decreased
	 * @param	bFullPage			TRUE to change the index by a full page, FALSE to change the index by 1
	 * @param	bHorizontalNavigation	TRUE if the user pressed right or left, FALSE if the user pressed up or down.
	 *
	 * @return	TRUE if the index was successfully changed
	 */
	virtual UBOOL NavigateIndex( UBOOL bIncrementIndex, UBOOL bFullPage, UBOOL bHorizontalNavigation );

public:
	/**
	 * Determines whether elements should render the names of the fields they're bound to.
	 *
	 * @return	TRUE if list elements should render the names for the data fields they're bound to, FALSE
	 *			if list elements should render the actual data for the list element they're associated with.
	 */
	UBOOL ShouldRenderDataBindings() const;

	/**
	 * Sets the selection state of the specified element.
	 *
	 * @param	ElementIndex	the index of the element to change selection state for
	 * @param	bSelected		TRUE to select the element, FALSE to unselect the element.
	 */
	void SelectElement( INT ElementIndex, UBOOL bSelected=TRUE );

	/**
	 * Called when the list's index has changed.
	 *
	 * @param	PreviousIndex	the list's Index before it was changed
	 * @param	PlayerIndex		the index of the player associated with this index change.
	 */
	virtual void NotifyIndexChanged( INT PreviousIndex, INT PlayerIndex );

	/**
	 * Called when the list's top item has changed
	 *
	 * @param	PreviousIndex	the list's TopIndex before it was changed
	 * @param	PlayerIndex		the index of the player that generated this change
	 */
	void NotifyTopIndexChanged( INT PreviousTopIndex, INT PlayerIndex );

	/**
	 * Called when the number of elements in this list is changed.
	 *
	 * @param	PreviousNumberOfItems	the number of items previously in the list
	 * @param	PlayerIndex				the index of the player that generated this change.
	 */
	void NotifyItemCountChanged( INT PreviousNumberOfItems, INT PlayerIndex );

	/**
	 * Called whenever the user chooses an item while this list is focused.  Activates the SubmitSelection kismet event and calls
	 * the OnSubmitSelection delegate.
	 */
	virtual void NotifySubmitSelection( INT PlayerIndex );

	/**
	 * Called after this list's elements have been sorted.  Synchronizes the list's Items array to the data component's elements array.
	 */
	virtual void NotifyListElementsSorted();

	/**
	 * Changes whether this list renders colum headers or not.  Only applicable if the owning list's CellLinkType is LINKED_Columns
	 */
	void EnableColumnHeaderRendering(UBOOL bShouldRenderColHeaders=TRUE)
	{
		if ( CellDataComponent != NULL )
		{
			CellDataComponent->EnableColumnHeaderRendering(bShouldRenderColHeaders);
		}
	}

	/**
	 * Returns whether this list should render column headers
	 */
	UBOOL ShouldRenderColumnHeaders() const
	{
		if ( CellDataComponent != NULL )
		{
			return CellDataComponent->ShouldRenderColumnHeaders();
		}

		return FALSE;
	}

	/* === UUIObject interface === */
	/**
	 * Render this list.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas );

	/**
	 * Called from UGameUISceneClient::UpdateMousePosition; provides a hook for widgets to respond to the precise cursor
	 * position.  Only called on the scene's ActiveControl if the ActiveControl's bEnableActiveCursorUpdates is TRUE and
	 * the mouse is currently over the widget.
	 *
	 * This version ensures that the element under the mouse is in the proper cell state, if bTrackMouse is true.
	 */
	virtual void NotifyMouseOver( const FVector2D& MousePos );

	/**
	 * Adds the specified face to the DockingStack for the specified widget
	 *
	 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
	 * @param	Face			the face that should be added
	 *
	 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
	 *			existed in the stack for the specified face of this widget.
	 */
	virtual UBOOL AddDockingNode( TArray<struct FUIDockingNode>& DockingStack, EUIWidgetFace Face );

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
	 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
	 *
	 * @param	Face	the face that should be resolved
	 */
	virtual void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Change the value of bEnableActiveCursorUpdates to the specified value.
	 */
	virtual void SetActiveCursorUpdate( UBOOL bShouldReceiveCursorUpdates );

protected:

	/**
	 * Called when a style reference is resolved successfully.
	 *
	 * @param	ResolvedStyle			the style resolved by the style reference
	 * @param	StyleProperty			the name of the style reference property that was resolved.
	 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
	 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
	 */
	virtual void OnStyleResolved( class UUIStyle* ResolvedStyle, const struct FStyleReferenceId& StyleProperty, INT ArrayIndex, UBOOL bInvalidateStyleData );

public:

	/**
	 * Applies the value of bShouldBeDirty to the current style data for all style references in this widget.  Used to force
	 * updating of style data.
	 *
	 * @param	bShouldBeDirty	the value to use for marking the style data for the specified menu state of all style references
	 *							in this widget as dirty.
	 * @param	MenuState		if specified, the style data for that menu state will be modified; otherwise, uses the widget's current
	 *							menu state
	 */
	virtual void ToggleStyleDirtiness( UBOOL bShouldBeDirty, class UUIState* MenuState=NULL );

	/**
	 * Determines whether this widget references the specified style.
	 *
	 * @param	CheckStyle		the style to check for referencers
	 */
	virtual UBOOL UsesStyle( class UUIStyle* CheckStyle );

	/* === UUIScreenObject interface === */
	/**
	 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
	 * once the scene has been completely initialized.
	 * For widgets added at runtime, called after the widget has been inserted into its parent's
	 * list of children.
	 *
	 * Initializes the value of bDisplayDataBindings based on whether we're in the game or not.
	 *
	 * @param	inOwnerScene	the scene to add this widget to.
	 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
	 *							is being added to the scene's list of children.
	 */
	virtual void Initialize( class UUIScene* inOwnerScene, class UUIObject* inOwner=NULL );

	/**
	 * Generates a array of UI Action keys that this widget supports.
	 *
	 * @param	out_KeyNames	Storage for the list of supported keynames.
	 */
	virtual void GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames );

	/**
	 * Called when a property is modified that could potentially affect the widget's position onscreen.
	 */
	virtual void RefreshPosition();

	/**
	 * Called to globally update the formatting of all UIStrings.
	 */
	virtual void RefreshFormatting();

	/**
	 * Removes the specified state from the screen object's state stack.
	 *
	 * @param	StateToRemove	the state to be removed
	 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
	 *
	 * @return	TRUE if the state was successfully removed, or if the state didn't exist in the widget's list of states;
	 *			false if the state overrode the request to be removed
	 */
	virtual UBOOL DeactivateState( UUIState* StateToRemove, INT PlayerIndex );

protected:
	/**
	 * Handles input events for this list.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );

	/**
	 * Processes input axis movement. Only called while the list is in the pressed state; resizes a column if ResizeColumn
	 * is a valid value.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputAxis( const struct FSubscribedInputEventParameters& EventParms );

public:

	/* === UObject interface === */
	/**
	 * Called when a property value from a member struct or array has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called when a member property value has been changed in the editor.
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Copies the values from the deprecated SelectionOverlayStyle property into the appropriate element of the ItemOverlayStyle array.
	 */
	virtual void PostLoad();

	/* === CustomPropertyItemHandler interface === */
	/**
	 * Determines whether the specified property value matches the current value of the property.  Called after the user
	 * has changed the value of a property handled by a custom property window item.  Is used to determine whether Pre/PostEditChange
	 * should be called for the selected objects.
	 *
	 * @param	InProperty			the property whose value is being checked.
	 * @param	NewPropertyValue	the value to compare against the current value of the property.
	 *
	 * @return	TRUE if NewPropertyValue matches the current value of the property specified, indicating that no effective changes
	 *			were actually made.
	 */
	virtual UBOOL IsCustomPropertyValueIdentical( UProperty* InProperty, const union UPropertyValue& NewPropertyValue );

	/**
	 * Method for overriding the default behavior of applying property values received from a custom editor property window item.
	 *
	 * @param	InProperty		the property that is being edited
	 * @param	PropertyValue	the value to assign to the property
	 *
	 * @return	TRUE if the property was handled by this object and the property value was successfully applied to the
	 *			object's data.
	 */
	virtual UBOOL EditorSetPropertyValue( UProperty* InProperty, const UPropertyValue& PropertyValue );
}

/* == Delegates == */
/**
 * Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
 *
 * @param	Sender	the list that is submitting the selection
 */
delegate OnSubmitSelection( UIList Sender, optional int PlayerIndex=GetBestPlayerIndex() );

/**
 * Called anytime this list's elements are sorted.
 *
 * @param	Sender	the list that just sorted its elements.
 */
delegate OnListElementsSorted( UIList Sender );

/**
 * Handler for vertical scrolling activity
 * PositionChange should be a number of nudge values by which the slider was moved
 * The nudge value in the UIList slider is equal to one list Item.
 *
 * @param	Sender			the scrollbar that generated the event.
 * @param	PositionChange	indicates how many items to scroll the list by
 * @param	bPositionMaxed	indicates that the scrollbar's marker has reached its farthest available position,
 *                          unused in this function
 */
native final function bool ScrollVertical( UIScrollbar Sender, float PositionChange, optional bool bPositionMaxed=false );

/**
 * Removes the specified element from the list.
 *
 * @param	ElementToRemove		the element to remove from the list
 *
 * @return	the index [into the Items array] for the element that was removed, or INDEX_NONE if the element wasn't
 *			part of the list.
 */
native function int RemoveElement(int ElementToRemove);

/**
 * Returns the number of elements in the list.
 *
 * @return	the number of elements in the list
 */
native function int GetItemCount() const;

/**
 * Returns the maximum number of elements that can be displayed by the list given its current size and configuration.
 */
native function int GetMaxVisibleElementCount() const;

/**
 * Returns the maximum number of rows that can be displayed in the list, given its current size and configuration.
 */
native final function int GetMaxNumVisibleRows() const;

/**
 *  Returns the maximum number of columns that can be displayed in the list, given its current size and configuration.
 */
native final function int GetMaxNumVisibleColumns() const;

/**
 * Returns the total number of rows in this list.
 */
native final function int GetTotalRowCount() const;

/**
 * Returns the total number of columns in this list.
 */
native final function int GetTotalColumnCount() const;

/**
 * Changes the list's ColumnCount to the value specified.
 */
native final function SetColumnCount( int NewColumnCount );

/**
 * Changes the list's RowCount to the value specified.
 */
native final function SetRowCount( int NewRowCount );

/**
 * Returns the width of the specified column.
 *
 * @param	ColumnIndex		the index for the column to get the width for.  If the index is invalid, the list's configured CellWidth is returned instead.
 */
native final function float GetColumnWidth( optional int ColumnIndex=INDEX_NONE ) const;

/**
 * Returns the width of the specified row.
 *
 * @param	RowIndex		the index for the row to get the width for.  If the index is invalid, the list's configured RowHeight is returned instead.
 */
native virtual function float GetRowHeight( optional int RowIndex=INDEX_NONE ) const;

/**
 * Returns the width and height of the bounding region for rendering the cells, taking into account whether the scrollbar
 * and column header are displayed.
 */
native function vector2D GetClientRegion() const;

/**
 * Calculates the index of the element under the mouse or joystick cursor
 *
 * @param	bRequireValidIndex	specify FALSE to return the calculated index, regardless of whether the index is valid or not.
 *								Useful for e.g. drag-n-drop operations where you want to drop at the end of the list.
 *
 * @return	the index [into the Items array] for the element under the mouse/joystick cursor, or INDEX_NONE if the mouse is not
 *			over a valid element.
 */
native function int CalculateIndexFromCursorLocation( optional bool bRequireValidIndex=true ) const;

/**
 * If the mouse is over a column boundary, returns the index of the column that would be resized, or INDEX_NONE if the mouse is not
 * hovering over a column boundary.
 *
 * @param	ClickedCell	will be filled with information about which cells the cursor is currently over
 *
 * @return	if the cursor is within ResizeBufferPixels of a column boundary, the index of the column the left of the cursor; INDEX_NONE
 *			otherwise.
 *
 * @note: noexport to allow the C++ version of this function to have a slightly different signature.
 */
native noexport function int GetResizeColumn( optional out const CellHitDetectionInfo ClickedCell ) const;

/**
 * Returns the items that are currently selected.
 *
 * @return	an array of values that represent indexes into the data source's data array for the list elements that are selected.
 *			these indexes are NOT indexes into the UIList.Items array; rather, they are the values of the UIList.Items elements which
 *			correspond to the selected items
 */
native final function array<int> GetSelectedItems() const;

/**
 * Returns the value of the element associated with the current list index
 *
 * @return	the value of the element at Index; this is not necessarily an index into the UIList.Items array; rather, it is the value
 *			of the UIList.Items element located at Index
 */
native final function int GetCurrentItem() const;

/**
 * Returns the text value for the specified element.  (temporary)
 *
 * @param	ElementIndex	index [into the Items array] for the value to return.
 * @param	CellIndex		for lists which have linked columns or rows, indicates which column/row to retrieve.
 *
 * @return	the value of the specified element, or an empty string if that element doesn't have a text value.
 */
native final function string GetElementValue( int ElementIndex, optional int CellIndex=INDEX_NONE ) const;

/**
 * Sets the list's index to the value specified and activates the appropriate notification events.
 *
 * @param	NewIndex			An index into the Items array that should become the new Index for the list.
 * @param	bClampValue			if TRUE, NewIndex will be clamped to a valid value in the range of 0 -> ItemCount - 1
 * @param	bSkipNotification	if TRUE, no events are generated as a result of updating the list's index.
 *
 * @return	TRUE if the list's Index was successfully changed.
 */
native final virtual function bool SetIndex( int NewIndex, optional bool bClampValue=true, optional bool bSkipNotification=false );

/**
 * Changes the list's first visible item to the element at the index specified.
 *
 * @param	NewTopIndex		an index into the Items array that should become the new first visible item.
 * @param	bClampValue		if TRUE, NewTopIndex will be clamped to a valid value in the range of 0 - ItemCount - 1
 *
 * @return	TRUE if the list's TopIndex was successfully changed.
 */
native final virtual function bool SetTopIndex( int NewTopIndex, optional bool bClampValue=true );

/**
 * Determines whether the specified list element is disabled by the data source bound to this list.
 *
 * @param	ElementIndex	the index into the Items array for the element to retrieve the menu state for.
 */
native final function bool IsElementEnabled( INT ElementIndex );

/**
 * Determines whether the specified list element can be selected.
 *
 * @param	ElementIndex	the index into the Items array for the element to query
 *
 * @return	true if the specified element can enter the ELEMENT_Selected state.  FALSE if the index specified is invalid or
 *			cannot be selected.
 */
native final function bool CanSelectElement( int ElementIndex );

/**
 * Change the value of bUpdateItemUnderCursor to the specified value.
 */
native final function SetHotTracking( bool bShouldUpdateItemUnderCursor );

/**
 * Returns the value of bUpdateItemUnderCursor.
 */
native final function bool IsHotTrackingEnabled() const;

/** UIDataSourceSubscriber interface */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
native final virtual function SetDataStoreBinding( string MarkupText, optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
native final virtual function string GetDataStoreBinding( optional int BindingIndex=INDEX_NONE ) const;

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
native final virtual function GetBoundDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
native final function ClearBoundDataStores();

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
native final virtual function bool SaveSubscriberValue( out array<UIDataStore> out_BoundDataStores, optional int BindingIndex=INDEX_NONE );

/* == Kismet action handlers == */
protected final function OnSetListIndex( UIAction_SetListIndex Action )
{
	local int OutputLinkIndex;

	if ( Action != None )
	{
		if ( !SetIndex(Action.NewIndex, Action.bClampInvalidValues, !Action.bActivateListChangeEvent) )
		{
			// 1 is the index of the "Failed" output link
			OutputLinkIndex = 1;
		}

		// activate the appropriate output link on the action
		if ( !Action.OutputLinks[OutputLinkIndex].bDisabled )
		{
			Action.OutputLinks[OutputLinkIndex].bHasImpulse = true;
		}
	}
}

/**
 * Handler for GetTextValue action.
 */
function OnGetTextValue( UIAction_GetTextValue Action )
{
	Action.StringValue = GetElementValue(Index, 0);
}

/**
 * Sets up the scroll activity delegates in the scrollbars
 * @todo - this is a fix for the issue where delegates don't seem to be getting set properly in defaultproperties blocks.
 */
event Initialized()
{
	Super.Initialized();

	SetActiveCursorUpdate(bUpdateItemUnderCursor);
	if ( VerticalScrollbar != None )
	{
		VerticalScrollbar.OnScrollActivity = ScrollVertical;
		VerticalScrollbar.OnClickedScrollZone = ClickedScrollZone;
	}
}

/**
 * Changes whether this list renders colum headers or not.  Only applicable if the owning list's CellLinkType is LINKED_Columns
 */
final function EnableColumnHeaderRendering( bool bShouldRenderColHeaders=true )
{
	if ( CellDataComponent != None )
	{
		CellDataComponent.EnableColumnHeaderRendering(bShouldRenderColHeaders);
	}
}

/**
 * Returns whether this list should render column headers
 */
final function bool ShouldRenderColumnHeaders()
{
	if ( CellDataComponent != None )
	{
		return CellDataComponent.ShouldRenderColumnHeaders();
	}

	return false;
}

/**
 * Returns whether element size is determined by the elements themselves.  For lists with linked columns, returns whether
 * the item height is autosized; for lists with linked rows, returns whether item width is autosized.
 */
native final function bool IsElementAutoSizingEnabled() const;

/**
 * Handler for the vertical scrollbar's OnClickedScrollZone delegate.  Scrolls the list by a full page (MaxVisibleItems).
 *
 * @param	Sender			the scrollbar that was clicked.
 * @param	PositionPerc	a value from 0.0 - 1.0, representing the location of the click within the region between the increment
 *							and decrement buttons.  Values closer to 0.0 means that the user clicked near the decrement button; values closer
 *							to 1.0 are nearer the increment button.
 * @param	PlayerIndex		Player that performed the action that issued the event.
 */
function ClickedScrollZone( UIScrollbar Sender, float PositionPerc, int PlayerIndex )
{
	local int MouseX, MouseY;
	local float MarkerPosition;
	local bool bDecrement;

	local int NewTopItem;

	if ( GetCursorPosition(MouseX, MouseY) )
	{
		// this is the position of the marker's minor side (left or top)
		MarkerPosition = Sender.GetMarkerButtonPosition();

		// determine whether the user clicked in the region above or below the marker button.
		bDecrement = (Sender.ScrollbarOrientation == UIORIENT_Vertical)
			? MouseY < MarkerPosition
			: MouseX < MarkerPosition;

		NewTopItem = bDecrement ? (TopIndex - MaxVisibleItems) : (TopIndex + MaxVisibleItems);
		SetTopIndex(NewTopItem, true);
	}
}

/**
 * Called when a new UIState becomes the widget's currently active state, after all activation logic has occurred.
 *
 * @param	Sender					the widget that changed states.
 * @param	PlayerIndex				the index [into the GamePlayers array] for the player that activated this state.
 * @param	NewlyActiveState		the state that is now active
 * @param	PreviouslyActiveState	the state that used the be the widget's currently active state.
 */
final function OnStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	if ( UIState_Pressed(NewlyActiveState) != None )
	{
		SetMouseCaptureOverride(true);
	}
	else if ( UIState_Pressed(PreviouslyActiveState) != None )
	{
		SetMouseCaptureOverride(false);
	}
}

DefaultProperties
{
	NotifyActiveStateChanged=OnStateChanged

	PrimaryStyle=(DefaultStyleTag="DefaultListStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')
	DataSource=(RequiredFieldType=DATATYPE_Collection)
	bSupportsPrimaryStyle=false

	ColumnHeaderStyle=(DefaultStyleTag="DefaultColumnHeaderStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')

	GlobalCellStyle(ELEMENT_Normal)=(DefaultStyleTag="DefaultCellStyleNormal",RequiredStyleClass=class'Engine.UIStyle_Combo')
	GlobalCellStyle(ELEMENT_Active)=(DefaultStyleTag="DefaultCellStyleActive",RequiredStyleClass=class'Engine.UIStyle_Combo')
	GlobalCellStyle(ELEMENT_Selected)=(DefaultStyleTag="DefaultCellStyleSelected",RequiredStyleClass=class'Engine.UIStyle_Combo')
	GlobalCellStyle(ELEMENT_UnderCursor)=(DefaultStyleTag="DefaultCellStyleHover",RequiredStyleClass=class'Engine.UIStyle_Combo')

	ItemOverlayStyle(ELEMENT_Normal)=(DefaultStyleTag="ListItemBackgroundNormalStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
	ItemOverlayStyle(ELEMENT_Active)=(DefaultStyleTag="ListItemBackgroundActiveStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
	ItemOverlayStyle(ELEMENT_Selected)=(DefaultStyleTag="ListItemBackgroundSelectedStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
	ItemOverlayStyle(ELEMENT_UnderCursor)=(DefaultStyleTag="ListItemBackgroundHoverStyle",RequiredStyleClass=class'Engine.UIStyle_Image')

	Index=-1
	TopIndex=-1
	CellLinkType=LINKED_Columns
	bEnableVerticalScrollbar=true
	bInitializeScrollbars=true

	RowHeight=(Value=16)
	ColumnWidth=(Value=100)
	RowCount=4
	ColumnCount=1
	ColumnAutoSizeMode=CELLAUTOSIZE_Uniform
	RowAutoSizeMode=CELLAUTOSIZE_Constrain
	ResizeColumn=INDEX_NONE

	Begin Object Class=UIComp_ListPresenter Name=ListPresentationComponent
	End Object
	CellDataComponent=ListPresentationComponent

	SubmitDataSuccessCue=ListSubmit
	SubmitDataFailedCue=GenericError
	DecrementIndexCue=ListUp
	IncrementIndexCue=ListDown
	SortAscendingCue=SortAscending
	SortDescendingCue=SortDescending

	// States
	DefaultStates.Add(class'Engine.UIState_Focused')
	DefaultStates.Add(class'Engine.UIState_Active')
	DefaultStates.Add(class'Engine.UIState_Pressed')

	DebugBoundsColor=(R=255,G=255,B=255,A=255)
}
