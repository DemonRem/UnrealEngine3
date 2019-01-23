/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __TWODEESHAPEEDITOR_H__
#define __TWODEESHAPEEDITOR_H__

class F2DSEVector : public FVector
{
public:
	F2DSEVector();
	F2DSEVector( FLOAT x, FLOAT y, FLOAT z );
	~F2DSEVector();

	inline F2DSEVector operator=( F2DSEVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		bSelected = Other.bSelected;
		return *this;
	}

	inline F2DSEVector operator=( FVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		return *this;
	}

	inline UBOOL operator!=( F2DSEVector Other )
	{
		return (X != Other.X && Y != Other.Y && Z != Other.Z);
	}

	inline UBOOL operator==( F2DSEVector Other )
	{
		return (X == Other.X && Y == Other.Y && Z == Other.Z);
	}

	void SelectToggle();
	void Select( UBOOL bSel );
	UBOOL IsSelected( void );

	UBOOL bSelected;
};


class FSegment
{
public:
	FSegment();
	FSegment( F2DSEVector vtx1, F2DSEVector vtx2 );
	FSegment( FVector vtx1, FVector vtx2 );
	~FSegment();

	inline FSegment operator=( FSegment Other )
	{
		Vertex[0] = Other.Vertex[0];
		Vertex[1] = Other.Vertex[1];
		ControlPoint[0] = Other.ControlPoint[0];
		ControlPoint[1] = Other.ControlPoint[1];
		return *this;
	}
	inline UBOOL operator==( FSegment Other )
	{
		return( Vertex[0] == Other.Vertex[0] && Vertex[1] == Other.Vertex[1] );
	}

	FVector GetHalfwayPoint();
	void GenerateControlPoint();
	void SetSegType( INT InType );
	UBOOL IsSelected();
	void GetBezierPoints(TArray<FVector>& BezierPoints);

	F2DSEVector Vertex[2], ControlPoint[2];
	INT SegType,		// eSEGTYPE_
		DetailLevel;	// Detail of bezier curve
	UBOOL bUsed,
		bSelected;
};

class FShape
{
public:
	FShape();
	~FShape();

	F2DSEVector Handle;
	TArray<FSegment> Segments;
	TArray<FVector> Verts;

	void ComputeHandlePosition();
	void Breakdown( F2DSEVector InOrigin, FPolyBreaker* InBreaker );
};

struct FTwoDeeShapeEditorViewportClient;

class WxViewportHolder;
class wxTwoDeeShapEditorPrimitives;
class WxTwoDeeShapeEditorMenu;
class WxTwoDeeShapeEditorBar;

class WxTwoDeeShapeEditor : public wxFrame, public FSerializableObject
{
public:
	DECLARE_DYNAMIC_CLASS(WxTwoDeeShapeEditor);

	WxTwoDeeShapeEditor();
	WxTwoDeeShapeEditor(wxWindow* parent, wxWindowID id);
	virtual ~WxTwoDeeShapeEditor();

	/** The filename of the currently loaded 2DS file */
	FFilename Filename;

	/** List of all the shapes in the editor */
	TArray<FShape> Shapes;

	/** The origin used for revolved shapes */
	F2DSEVector Origin;

	WxViewportHolder* ViewportHolder;
	wxSplitterWindow* SplitterWnd;
	FTwoDeeShapeEditorViewportClient* ViewportClient;
	wxTwoDeeShapEditorPrimitives* PrimitiveWindow;
	
	WxTwoDeeShapeEditorMenu* MenuBar;
	WxTwoDeeShapeEditorBar* ToolBar;

	/**
	 * Must be called on each map change to update the shape editors GWorld-dependent objects.
	 */
	void ResetPrimitiveWindow();

	void New();
	void InsertNewShape();
	void ComputeHandlePositions( UBOOL InAlwaysCompute = 0 );
	void SelectNone();
	UBOOL HasSelection();
	void ApplyDelta( FVector InDelta );
	void ApplyDeltaToVertex( FShape* InShape, F2DSEVector InVertex, FVector InDelta );
	void SplitEdges();
	void DeleteSegment( FShape* InShape, INT InSegIdx );
	void Delete();
	void RotateBuilderBrush( INT Axis );
	void ProcessSheet();
	void ProcessExtrude( INT Depth );
	void ProcessExtrudeToPoint( INT Depth );
	void ProcessExtrudeToBevel( INT Depth, INT CapHeight );
	void ProcessRevolve( INT TotalSides, INT Sides );
	void Flip( UBOOL InHoriz );
	void Load( const TCHAR* Filename );
	void Save( const TCHAR* Filename );
	void SaveAs();
	void SetCaption();

	virtual void Serialize(FArchive& Ar);

	void OnSize( wxSizeEvent& In );
	void OnPaint( wxPaintEvent& In );
	void OnEditDelete( wxCommandEvent& In );
	void OnEditSplitEdges( wxCommandEvent& In );
	void OnEditBezier( wxCommandEvent& In );
	void OnEditLinear( wxCommandEvent& In );
	void OnShapeNew( wxCommandEvent& In );
	void OnFileNew( wxCommandEvent& In );
	void OnFileOpen( wxCommandEvent& In );
	void OnFileSave( wxCommandEvent& In );
	void OnFileSaveAs( wxCommandEvent& In );

	DECLARE_EVENT_TABLE();;
};

#endif // __TWODEESHAPEEDITOR_H__
