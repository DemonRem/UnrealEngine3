/*=============================================================================
	MaterialEditorBase.cpp:	Base class for the material editor and material instance editor.  
							Contains info needed for previewing materials.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "MaterialEditorPreviewScene.h"
#include "MaterialEditorBase.h"
#include "UnLinkedObjDrawUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditorPrevew
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * wxWindows container for FMaterialEditorPreviewVC.
 */
class WxMaterialEditorPreview : public wxWindow
{
public:
	FMaterialEditorPreviewVC*	MaterialEditorPrevewVC;

	WxMaterialEditorPreview(wxWindow* InParent, wxWindowID InID, WxMaterialEditorBase* InMaterialEditor, const FVector& InViewLocation, const FRotator& InViewRotation)
		:	wxWindow( InParent, InID )
	{
		MaterialEditorPrevewVC = new FMaterialEditorPreviewVC( InMaterialEditor, GUnrealEd->GetThumbnailManager()->TexPropSphere, NULL );
		MaterialEditorPrevewVC->Viewport = GEngine->Client->CreateWindowChildViewport( MaterialEditorPrevewVC, (HWND)GetHandle() );
		MaterialEditorPrevewVC->Viewport->CaptureJoystickInput( FALSE );

		MaterialEditorPrevewVC->ViewLocation = InViewLocation;
		MaterialEditorPrevewVC->ViewRotation = InViewRotation;
		MaterialEditorPrevewVC->SetViewLocationForOrbiting( FVector(0.f,0.f,0.f) );
	}

	virtual ~WxMaterialEditorPreview()
	{
		GEngine->Client->CloseViewport( MaterialEditorPrevewVC->Viewport );
		MaterialEditorPrevewVC->Viewport = NULL;
		delete MaterialEditorPrevewVC;
	}

private:
	void OnSize(wxSizeEvent& In)
	{
		MaterialEditorPrevewVC->Viewport->Invalidate();
		const wxRect rc = GetClientRect();
		::MoveWindow( (HWND)MaterialEditorPrevewVC->Viewport->GetWindow(), 0, 0, rc.GetWidth(), rc.GetHeight(), 1 );
		In.Skip();
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WxMaterialEditorPreview, wxWindow )
	EVT_SIZE( WxMaterialEditorPreview::OnSize )
END_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditorBase
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(WxMaterialEditorBase, wxFrame)
	EVT_TOOL( IDM_SHOW_BACKGROUND, WxMaterialEditorBase::OnShowBackground )
	EVT_TOOL( ID_MATERIALEDITOR_TOGGLEGRID, WxMaterialEditorBase::OnToggleGrid )

	EVT_TOOL( ID_PRIMTYPE_CYLINDER, WxMaterialEditorBase::OnPrimTypeCylinder )
	EVT_TOOL( ID_PRIMTYPE_CUBE, WxMaterialEditorBase::OnPrimTypeCube )
	EVT_TOOL( ID_PRIMTYPE_SPHERE, WxMaterialEditorBase::OnPrimTypeSphere )
	EVT_TOOL( ID_MATERIALEDITOR_REALTIME_PREVIEW, WxMaterialEditorBase::OnRealTimePreview )
	EVT_TOOL( ID_MATERIALEDITOR_SET_PREVIEW_MESH_FROM_SELECTION, WxMaterialEditorBase::OnSetPreviewMeshFromSelection )

	EVT_UPDATE_UI( ID_PRIMTYPE_CYLINDER, WxMaterialEditorBase::UI_PrimTypeCylinder )
	EVT_UPDATE_UI( ID_PRIMTYPE_CUBE, WxMaterialEditorBase::UI_PrimTypeCube )
	EVT_UPDATE_UI( ID_PRIMTYPE_SPHERE, WxMaterialEditorBase::UI_PrimTypeSphere )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_REALTIME_PREVIEW, WxMaterialEditorBase::UI_RealTimePreview )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_TOGGLEGRID, WxMaterialEditorBase::UI_ShowGrid )
END_EVENT_TABLE()

WxMaterialEditorBase::WxMaterialEditorBase(wxWindow* InParent, wxWindowID InID, UMaterialInterface* InMaterialInterface)
	:	wxFrame( InParent, InID, TEXT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR )
	,	PreviewPrimType( TPT_None )
	,	bUseSkeletalMeshAsPreview( FALSE )
	,	bShowBackground( FALSE )
	,	bShowGrid( TRUE )
{
	// Create 3D preview window.
	const FRotator ViewAngle( 0.0f, 0.0f, 0.0f );
	PreviewWin = new WxMaterialEditorPreview( this, -1, this, FVector(0.f,0.f,0.f), ViewAngle );
	PreviewVC = PreviewWin->MaterialEditorPrevewVC;
	SetPreviewMaterial( InMaterialInterface );

	// Default to the sphere.
	SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropSphere, NULL );
}

void WxMaterialEditorBase::Serialize(FArchive& Ar)
{
	PreviewVC->Serialize(Ar);

	Ar << MaterialInterface;
	Ar << PreviewMeshComponent;
	Ar << PreviewSkeletalMeshComponent;
}

/**
 * Sets the mesh on which to preview the material.  One of either InStaticMesh or InSkeletalMesh must be non-NULL!
 * Does nothing if a skeletal mesh was specified but the material has bUsedWithSkeletalMesh=FALSE.
 *
 * @return	TRUE if a mesh was set successfully, FALSE otherwise.
 */
UBOOL WxMaterialEditorBase::SetPreviewMesh(UStaticMesh* InStaticMesh, USkeletalMesh* InSkeletalMesh)
{
	// Allow derived types to veto setting the mesh.
	const UBOOL bApproved = ApproveSetPreviewMesh( InStaticMesh, InSkeletalMesh );
	if ( !bApproved )
	{
		return FALSE;
	}

	bUseSkeletalMeshAsPreview = InSkeletalMesh != NULL;
	if ( bUseSkeletalMeshAsPreview )
	{
		// Remove the static mesh from the preview scene.
		PreviewVC->PreviewScene->AddComponent( PreviewSkeletalMeshComponent, FMatrix::Identity );
		PreviewVC->PreviewScene->RemoveComponent( PreviewMeshComponent );

		// Update the toolbar state implicitly through PreviewPrimType.
		PreviewPrimType = TPT_None;

		// Set the new preview skeletal mesh.
		PreviewSkeletalMeshComponent->SetSkeletalMesh( InSkeletalMesh );
	}
	else
	{
		// Remove the skeletal mesh from the preview scene.
		PreviewVC->PreviewScene->RemoveComponent( PreviewSkeletalMeshComponent );
		PreviewVC->PreviewScene->AddComponent( PreviewMeshComponent, FMatrix::Identity );

		// Set the new preview static mesh.
		FComponentReattachContext ReattachContext( PreviewMeshComponent );
		PreviewMeshComponent->StaticMesh = InStaticMesh;

		// Update the toolbar state implicitly through PreviewPrimType.
		if ( InStaticMesh == GUnrealEd->GetThumbnailManager()->TexPropCylinder )
		{
			PreviewPrimType = TPT_Cylinder;
		}
		else if ( InStaticMesh == GUnrealEd->GetThumbnailManager()->TexPropCube )
		{
			PreviewPrimType = TPT_Cube;
		}
		else if ( InStaticMesh == GUnrealEd->GetThumbnailManager()->TexPropSphere )
		{
			PreviewPrimType = TPT_Sphere;
		}
		else
		{
			PreviewPrimType = TPT_None;
		}
	}

	return TRUE;
}

/**
 * Sets the preview mesh to the named mesh, if it can be found.  Checks static meshes first, then skeletal meshes.
 * Does nothing if the named mesh is not found or if the named mesh is a skeletal mesh but the material has
 * bUsedWithSkeletalMesh=FALSE.
 *
 * @return	TRUE if the named mesh was found and set successfully, FALSE otherwise.
 */
UBOOL WxMaterialEditorBase::SetPreviewMesh(const TCHAR* InMeshName)
{
	UBOOL bSuccess = FALSE;
	if ( InMeshName )
	{
		UStaticMesh* StaticMesh = FindObject<UStaticMesh>( ANY_PACKAGE, InMeshName );
		if ( StaticMesh )
		{
			bSuccess = SetPreviewMesh( StaticMesh, NULL );
		}
		else
		{
			USkeletalMesh* SkeletalMesh = FindObject<USkeletalMesh>( ANY_PACKAGE, InMeshName );
			if ( SkeletalMesh )
			{
				bSuccess = SetPreviewMesh( NULL, SkeletalMesh );
			}
		}
	}
	return bSuccess;
}

/**
 * Called by SetPreviewMesh, allows derived types to veto the setting of a preview mesh.
 *
 * @return	TRUE if the specified mesh can be set as the preview mesh, FALSE otherwise.
 */
UBOOL WxMaterialEditorBase::ApproveSetPreviewMesh(UStaticMesh* InStaticMesh, USkeletalMesh* InSkeletalMesh)
{
	// Default impl is to always accept.
	return TRUE;
}

/**
 *
 */
void WxMaterialEditorBase::SetPreviewMaterial(UMaterialInterface* InMaterialInterface)
{
	check( PreviewMeshComponent );
	check( PreviewSkeletalMeshComponent );

	MaterialInterface = InMaterialInterface;

	PreviewMeshComponent->Materials.Empty();
	PreviewMeshComponent->Materials.AddItem( InMaterialInterface );
	PreviewSkeletalMeshComponent->Materials.Empty();
	PreviewSkeletalMeshComponent->Materials.AddItem( InMaterialInterface );
}

/** Refreshes the viewport containing the preview mesh. */
void WxMaterialEditorBase::RefreshPreviewViewport()
{
	//reattach the preview components, so if the preview material changed it will be propagated to the render thread
	PreviewMeshComponent->BeginDeferredReattach();
	PreviewSkeletalMeshComponent->BeginDeferredReattach();
	PreviewVC->Viewport->Invalidate();
}


/**
 * Draws the instruction count for the material's instance of a specific shader type.
 * @param Canvas - The canvas to draw the instruction count to.
 * @param FMaterialShaderMap - The shader map.
 * @param ShaderType - The shader type to draw the instruction count for.
 * @param FriendlyDescription - The user-friendly description of the shader type.
 * @param X - The X position to draw the instruction count to.
 * @param InOutY - The Y position to draw the instruction count to.
 *			Upon return, contains the Y position to draw the next instruction count to.
 */
static void DrawShaderInstructionCount(
	FCanvas* Canvas,
	const FMaterialShaderMap* MaterialShaderMap,
	FShaderType* ShaderType,
	const TCHAR* FriendlyDescription,
	INT X,
	INT& InOutY
	)
{
	check(ShaderType);
	check(MaterialShaderMap);
	// Use the local vertex factory shaders.
	const FMeshMaterialShaderMap* MeshShaderMap = MaterialShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
	check(MeshShaderMap);

	const FShader* Shader = MeshShaderMap->GetShader(ShaderType);
	if(Shader)
	{
		FString InstructionCountString = FString::Printf(TEXT("%s: %u instructions"),FriendlyDescription,Shader->GetNumInstructions());
		FLinkedObjDrawUtils::DrawShadowedString( Canvas, X, InOutY, *InstructionCountString, GEngine->SmallFont, FLinearColor(1,1,0) );
		InOutY += 20;
	}
}

/**
 * Draws material info strings such as instruction count and current errors onto the canvas.
 */
void WxMaterialEditorBase::DrawMaterialInfoStrings(FCanvas* Canvas, const FMaterialResource* MaterialResource, INT &DrawPositionY)
{
	if(MaterialResource)
	{
		// Display any errors in the upper left corner of the viewport.

		const FMaterialShaderMap* MaterialShaderMap = MaterialResource->GetShaderMap();

		const TArray<FString>& CompileErrors = MaterialResource->GetCompileErrors();

		if (MaterialShaderMap)
		{
			// Draw the emissive wo/ light-map instruction count.
			DrawShaderInstructionCount(
				Canvas,
				MaterialShaderMap,
				FindShaderTypeByName(TEXT("TBasePassPixelShaderFNoLightMapPolicyNoSkyLight")),
				TEXT("Emissive pixel shader wo/ light-map wo/ sky-light"),
				5,
				DrawPositionY
				);

			// Draw the directional light w/ shadow-map instruction count.
			DrawShaderInstructionCount(
				Canvas,
				MaterialShaderMap,
				FindShaderTypeByName(TEXT("TLightPixelShaderFDirectionalLightPolicyFShadowTexturePolicy")),
				TEXT("Directional light pixel shader w/ shadow-map"),
				5,
				DrawPositionY
				);
		}

		// Display the number of samplers used by the material.
		const INT SamplersUsed = MaterialResource->GetSamplerUsage();
		FLinkedObjDrawUtils::DrawShadowedString(
			Canvas,
			5,
			DrawPositionY,
			*FString::Printf(TEXT("Texture samplers: %u/%u"), SamplersUsed, MAX_ME_PIXELSHADER_SAMPLERS),
			GEngine->SmallFont,
			(SamplersUsed > MAX_ME_PIXELSHADER_SAMPLERS) ? FLinearColor(1,0,0) : FLinearColor(1,1,0)
			);
		DrawPositionY += 20;

		// Determine the maximum texture dependency length.
		const INT MaxTextureDependencyLength = MaterialResource->GetMaxTextureDependencyLength();

		if(MaxTextureDependencyLength > 1)
		{
			// Draw the the material's texture dependency length.
			FLinkedObjDrawUtils::DrawShadowedString(
				Canvas,
				5,
				DrawPositionY,
				*FString::Printf(TEXT("Maximum texture dependency length: %u"),MaxTextureDependencyLength - 1),
				GEngine->SmallFont,
				FLinearColor(1,0,1)
				);
			DrawPositionY += 20;
		}

		for(INT ErrorIndex = 0; ErrorIndex < CompileErrors.Num(); ErrorIndex++)
		{
			FLinkedObjDrawUtils::DrawShadowedString( Canvas, 5, DrawPositionY, *CompileErrors(ErrorIndex), GEngine->SmallFont, FLinearColor(1,0,0) );
			DrawPositionY += 20;
		}

	}
}

void WxMaterialEditorBase::OnShowBackground(wxCommandEvent& In)
{
	ToggleShowBackground();
}

void WxMaterialEditorBase::OnToggleGrid(wxCommandEvent& In)
{
	bShowGrid = In.IsChecked() ? TRUE : FALSE;
	PreviewVC->SetShowGrid( bShowGrid );
	RefreshPreviewViewport();
}

/**
 * Sets the preview mesh for the preview window to the selected primitive type.
 */
void WxMaterialEditorBase::SetPrimitivePreview()
{
	switch(PreviewPrimType)
	{
	case TPT_Cube:
		SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropCube, NULL );
		break;
	case TPT_Plane:
		SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropPlane, NULL );
		break;
	case TPT_Cylinder:
		SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropCylinder, NULL );
		break;
	default:
		SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropSphere, NULL );
	}

	RefreshPreviewViewport();
}

void WxMaterialEditorBase::OnPrimTypeCylinder(wxCommandEvent& In)
{
	PreviewPrimType = TPT_Cylinder;
	SetPrimitivePreview();
}

void WxMaterialEditorBase::OnPrimTypeCube(wxCommandEvent& In)
{
	PreviewPrimType = TPT_Cube;
	SetPrimitivePreview();
}

void WxMaterialEditorBase::OnPrimTypeSphere(wxCommandEvent& In)
{
	PreviewPrimType = TPT_Sphere;
	SetPrimitivePreview();
}

void WxMaterialEditorBase::OnRealTimePreview(wxCommandEvent& In)
{
	PreviewVC->ToggleRealtime();
}

void WxMaterialEditorBase::ToggleShowBackground()
{
	bShowBackground = !bShowBackground;
	// @todo DB: Set the background mesh for the preview viewport.
	RefreshPreviewViewport();
}


void WxMaterialEditorBase::OnSetPreviewMeshFromSelection(wxCommandEvent& In)
{
	UBOOL bFoundPreviewMesh = FALSE;

	// Look for a selected static mesh.
	UStaticMesh* SelectedStaticMesh = GEditor->GetSelectedObjects()->GetTop<UStaticMesh>();
	if ( SelectedStaticMesh )
	{
		SetPreviewMesh( SelectedStaticMesh, NULL );
		MaterialInterface->PreviewMesh = SelectedStaticMesh->GetPathName();
		bFoundPreviewMesh = TRUE;
	}
	else
	{
		// No static meshes were selected; look for a selected skeletal mesh.
		USkeletalMesh* SelectedSkeletalMesh = GEditor->GetSelectedObjects()->GetTop<USkeletalMesh>();
		if ( SelectedSkeletalMesh )
		{
			SetPreviewMesh( NULL, SelectedSkeletalMesh );
			MaterialInterface->PreviewMesh = SelectedSkeletalMesh->GetPathName();
			bFoundPreviewMesh = TRUE;
		}
	}

	if ( bFoundPreviewMesh )
	{
		MaterialInterface->MarkPackageDirty();
		RefreshPreviewViewport();
	}
}

void WxMaterialEditorBase::UI_PrimTypeCylinder(wxUpdateUIEvent& In)
{
	In.Check( PreviewPrimType == TPT_Cylinder );
}

void WxMaterialEditorBase::UI_PrimTypeCube(wxUpdateUIEvent& In)
{
	In.Check( PreviewPrimType == TPT_Cube );
}

void WxMaterialEditorBase::UI_PrimTypeSphere(wxUpdateUIEvent& In)
{
	In.Check( PreviewPrimType == TPT_Sphere );
}

void WxMaterialEditorBase::UI_RealTimePreview(wxUpdateUIEvent& In)
{
	In.Check( PreviewVC->IsRealtime() == TRUE );
}

void WxMaterialEditorBase::UI_ShowGrid(wxUpdateUIEvent& In)
{
	In.Check( bShowGrid == TRUE );
}

