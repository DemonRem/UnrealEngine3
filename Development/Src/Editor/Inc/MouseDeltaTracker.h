/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MOUSEDELTATRACKER_H__
#define __MOUSEDELTATRACKER_H__

// Forward declarations.
class FDragTool;
class FScopedTransaction;
struct FEditorLevelViewportClient;

/**
 * Keeps track of mouse movement deltas in the viewports.
 */
class FMouseDeltaTracker
{
public:
	FMouseDeltaTracker();
	~FMouseDeltaTracker();

	/**
	 * Begin tracking at the specified location for the specified viewport.
	 */
	void StartTracking(FEditorLevelViewportClient* InViewportClient, const INT InX, const INT InY, UBOOL bArrowMovement = FALSE);

	/**
	 * Called when a mouse button has been released.  If there are no other
	 * mouse buttons being held down, the internal information is reset.
	 */
	UBOOL EndTracking(FEditorLevelViewportClient* InViewportClient);

	/**
	 * Adds delta movement into the tracker.
	 */
	void AddDelta(FEditorLevelViewportClient* InViewportClient, const FName InKey, const INT InDelta, UBOOL InNudge);

	/**
	* Returns the current delta.
	*/
	const FVector GetDelta() const;

	/**
	 * Returns the current snapped delta.
	 */
	const FVector GetDeltaSnapped() const;

	/**
	* Returns the absolute delta since dragging started.
	*/
	const FVector GetAbsoluteDelta() const;

	/**
	* Returns the absolute snapped delta since dragging started. 
	*/
	const FVector GetAbsoluteDeltaSnapped() const;


	/**
	 * Converts the delta movement to drag/rotation/scale based on the viewport type or widget axis.
	 */
	void ConvertMovementDeltaToDragRot(FEditorLevelViewportClient* InViewportClient, const FVector& InDragDelta, FVector& InDrag, FRotator& InRotation, FVector& InScale);

	/**
	 * Subtracts the specified value from End and EndSnapped.
	 */
	void ReduceBy(const FVector& In);

	/**
	 * @return		TRUE if a drag tool is being used by the tracker, FALSE otherwise.
	 */
	UBOOL UsingDragTool() const;

	/**
	 * Renders the drag tool.  Does nothing if no drag tool exists.
	 */
	void Render3DDragTool(const FSceneView* View, FPrimitiveDrawInterface* PDI);

	/**
	 * Renders the drag tool.  Does nothing if no drag tool exists.
	 */
	void RenderDragTool(const FSceneView* View, FCanvas* Canvas);

private:
	/** The unsnapped start position of the current mouse drag. */
	FVector Start;
	/** The snapped start position of the current mouse drag. */
	FVector StartSnapped;
	/** The unsnapped end position of the current mouse drag. */
	FVector End;
	/** The snapped end position of the current mouse drag. */
	FVector EndSnapped;

	/** The amount that the End vectors have been reduced by since dragging started, this is added to the deltas to get an absolute delta. */
	FVector ReductionAmount;

	/**
	 * If there is a dragging tool being used, this will point to it.
	 * Gets newed/deleted in StartTracking/EndTracking.
	 */
	FDragTool* DragTool;

	/** Count how many transactions we've started. */
	INT		TransCount;

	/** The current transaction. */
	FScopedTransaction*	ScopedTransaction;

	/**
	 * Sets the current axis of the widget for the specified viewport.
	 *
	 * @param	InViewportClient		The viewport whose widget axis is to be set.
	 */
	void DetermineCurrentAxis(FEditorLevelViewportClient* InViewportClient);

	/**
	 * Initiates a transaction.
	 */
	void BeginTransaction(const TCHAR* SessionName);

	/**
	 * Ends the current transaction, if one exists.
	 */
	void EndTransaction();
};

#endif // __MOUSEDELTATRACKER_H__
