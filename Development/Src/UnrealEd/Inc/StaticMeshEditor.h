/*=============================================================================
	StaticMeshEditor.h: StaticMesh editor definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __STATICMESHEDITOR_H__
#define __STATICMESHEDITOR_H__

#include "ConvexDecompTool.h"

struct FStaticMeshEditorViewportClient;

class WxViewportHolder;
class WxPropertyWindow;
class WxStaticMeshEditorBar;
class WxStaticMeshEditMenu;

class WxStaticMeshEditor : public wxFrame, public FSerializableObject, public FDockingParent, public FConvexDecompOptionHook
{
public:
	DECLARE_DYNAMIC_CLASS(WxStaticMeshEditor)

	WxStaticMeshEditor();
	WxStaticMeshEditor(wxWindow* parent, wxWindowID id, UStaticMesh* InStaticMesh);
	virtual ~WxStaticMeshEditor();

	UStaticMesh* StaticMesh;
	UBOOL DrawOpenEdges;
	UBOOL DrawUVOverlay;
	UINT NumTriangles, NumUVChannels;
	WxViewportHolder* ViewportHolder;
	WxPropertyWindow* PropertyWindow;
	FStaticMeshEditorViewportClient* ViewportClient;
	WxStaticMeshEditorBar* ToolBar;
	WxStaticMeshEditMenu* MenuBar;
	WxConvexDecompOptions* DecompOptions;

	void LockCamera(UBOOL bInLock);

	/** Updates the toolbars*/
	void UpdateToolbars();
	void GenerateKDop(const FVector* Directions, UINT NumDirections);
	void GenerateSphere();

	/** Clears the collision data for the static mesh */
	void RemoveCollision(void);

	/** Handler for the FConvexDecompOptionHook hook */
	virtual void DoDecomp(INT Depth, INT MaxVerts, FLOAT CollapseThresh);
	virtual void DecompOptionsClosed();


	virtual void Serialize(FArchive& Ar);

protected:
	/**
	 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
	 *  @return A string representing a name to use for this docking parent.
	 */
	virtual const TCHAR* GetDockingParentName() const;

	/**
	 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
	 */
	virtual const INT GetDockingParentVersion() const;

private:
	void OnSize( wxSizeEvent& In );
	void OnPaint( wxPaintEvent& In );
	void UI_ShowEdges( wxUpdateUIEvent& In );
	void UI_ShowUVOverlay( wxUpdateUIEvent& In );
	void UI_ShowWireframe( wxUpdateUIEvent& In );
	void UI_ShowBounds( wxUpdateUIEvent& In );
	void UI_ShowCollision( wxUpdateUIEvent& In );
	void UI_LockCamera( wxUpdateUIEvent& In );
	void OnShowEdges( wxCommandEvent& In );
	void OnShowUVOverlay( wxCommandEvent& In );
	void OnShowWireframe( wxCommandEvent& In );
	void OnShowBounds( wxCommandEvent& In );
	void OnShowCollision( wxCommandEvent& In );
	void OnLockCamera( wxCommandEvent& In );
	void OnSaveThumbnailAngle( wxCommandEvent& In );
	void OnCollision6DOP( wxCommandEvent& In );
	void OnCollision10DOPX( wxCommandEvent& In );
	void OnCollision10DOPY( wxCommandEvent& In );
	void OnCollision10DOPZ( wxCommandEvent& In );
	void OnCollision18DOP( wxCommandEvent& In );
	void OnCollision26DOP( wxCommandEvent& In );
	void OnCollisionSphere( wxCommandEvent& In );

	/** Handles the remove collision menu option */
	void OnCollisionRemove( wxCommandEvent& In );

	/** When chosen from the menu, pop up the 'convex decomposition' dialog. */
	void OnCollisionConvexDecomp( wxCommandEvent& In );

	/** Event for forcing an LOD */
	void OnForceLODLevel( wxCommandEvent& In );
	/** Event for removing an LOD */
	void OnRemoveLOD( wxCommandEvent& In );
	/** Event for generating an LOD */
	void OnGenerateLOD( wxCommandEvent& In );
	/** Event for generating UV's */
	void OnGenerateUVs( wxCommandEvent& In );
	/** Event for importing an LOD */
	void OnImportMeshLOD( wxCommandEvent& In );
	/** Updates NumTriangles, NumOpenEdges, NumDoubleSidedShadowTriangles and NumUVChannels for the given LOD */
	void UpdateLODStats(INT CurrentLOD);

	DECLARE_EVENT_TABLE()
};

#endif // __STATICMESHEDITOR_H__
