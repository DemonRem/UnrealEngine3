/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineMaterialClasses.h"
#include "UnLinkedObjDrawUtils.h"
#include "MaterialEditorContextMenus.h"
#include "NewMaterialEditor.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static helpers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

/**
 * Appends entries to the specified menu for creating new material expressions.
 *
 * @param	Menu			The menu to append items to.
 * @param	MaterialEditor	The material editor the menu appears in.
 */
static void AppendNewShaderExpressionsToMenu(wxMenu* Menu, WxMaterialEditor* MaterialEditor)
{
	check( MaterialEditor->bMaterialExpressionClassesInitialized );

	INT ID = IDM_NEW_SHADER_EXPRESSION_START;

	// Create a menu based on the sorted list.
	for( INT x = 0 ; x < MaterialEditor->MaterialExpressionClasses.Num() ; ++x )
	{
		UClass* MaterialExpressionClass = MaterialEditor->MaterialExpressionClasses(x);
		Menu->Append( ID++,
			*FString::Printf(*LocalizeUnrealEd("NewF"),*FString(*MaterialExpressionClass->GetName()).Mid(appStrlen(TEXT("MaterialExpression")))),
			TEXT("")
			);
	}
	checkMsg( ID <= IDM_NEW_SHADER_EXPRESSION_END, "Insuffcient number of material expression IDs" );
}

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditorContextMenu_NewNode
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Presented when the user right-clicks on an empty region of the material editor viewport.
 */
WxMaterialEditorContextMenu_NewNode::WxMaterialEditorContextMenu_NewNode(WxMaterialEditor* MaterialEditor)
{
	// New expressions.
	AppendNewShaderExpressionsToMenu( this, MaterialEditor );

	// New comment.
	AppendSeparator();
	Append( ID_MATERIALEDITOR_NEW_COMMENT, *LocalizeUnrealEd("NewCommentWithHotkey"), TEXT("") );

	// Paste here.
	AppendSeparator();
	Append( ID_MATERIALEDITOR_PASTE_HERE, *LocalizeUnrealEd("PasteHere"), TEXT("") );

	// Compound expression.
	const INT NumSelected = MaterialEditor->GetNumSelected();
	if( NumSelected > 0 )
	{
		//Append( ID_MATERIALEDITOR_NEW_COMPOUND_EXPRESSION, *LocalizeUnrealEd("CreateCompoundExpression"), TEXT("") );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditorContextMenu_NodeOptions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Presented when the user right-clicks on a material expression node in the material editor.
 */
WxMaterialEditorContextMenu_NodeOptions::WxMaterialEditorContextMenu_NodeOptions(WxMaterialEditor* MaterialEditor)
{
	const INT NumSelected = MaterialEditor->GetNumSelected();

	if( NumSelected > 0 )
	{
		// Compound expression.
		//Append( ID_MATERIALEDITOR_NEW_COMPOUND_EXPRESSION, *LocalizeUnrealEd("CreateCompoundExpression"), TEXT("") );
		//AppendSeparator();

		// Present "Use Current Texture" if a texture is selected in the generic browser and at
		// least on Texture Sample node is selected in the material editor.
		UTexture* SelectedTexture = GEditor->GetSelectedObjects()->GetTop<UTexture>();
		if ( SelectedTexture )
		{
			for ( INT ExpressionIndex = 0 ; ExpressionIndex < MaterialEditor->SelectedExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* Expression = MaterialEditor->SelectedExpressions(ExpressionIndex);
				if ( Expression->IsA(UMaterialExpressionTextureSample::StaticClass()) )
				{
					//AppendSeparator();
					Append( IDM_USE_CURRENT_TEXTURE, *LocalizeUnrealEd("UseCurrentTexture"), *LocalizeUnrealEd("UseCurrentTexture") );
					AppendSeparator();
					break;
				}
			}
		}
	}

	// Duplicate selected.
	if( NumSelected == 1 )
	{
		Append( ID_MATERIALEDITOR_DUPLICATE_NODES, *LocalizeUnrealEd("DuplicateSelectedItem"), TEXT("") );
	}
	else
	{
		Append( ID_MATERIALEDITOR_DUPLICATE_NODES, *LocalizeUnrealEd("DuplicateSelectedItems"), TEXT("") );
	}
	AppendSeparator();

	// Break all links.
	Append( ID_MATERIALEDITOR_BREAK_ALL_LINKS, *LocalizeUnrealEd("BreakAllLinks"), TEXT("") );
	AppendSeparator();

	// Delete selected.
	if( NumSelected == 1 )
	{
		Append( ID_MATERIALEDITOR_DELETE_NODE, *LocalizeUnrealEd("DeleteSelectedItem"), TEXT("") );
	}
	else
	{
		Append( ID_MATERIALEDITOR_DELETE_NODE, *LocalizeUnrealEd("DeleteSelectedItems"), TEXT("") );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditorContextMenu_ConnectorOptions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Presented when the user right-clicks on an object connector in the material editor viewport.
 */
WxMaterialEditorContextMenu_ConnectorOptions::WxMaterialEditorContextMenu_ConnectorOptions(WxMaterialEditor* MaterialEditor)
{
	UMaterialExpression* MaterialExpression = Cast<UMaterialExpression>( MaterialEditor->ConnObj );
	UMaterial* MaterialNode = Cast<UMaterial>( MaterialEditor->ConnObj );

	UBOOL bHasExpressionConnection = FALSE;
	UBOOL bHasMaterialConnection = FALSE;

	if( MaterialEditor->ConnType == LOC_OUTPUT ) // Items on right side of node.
	{
		if ( MaterialExpression )
		{
			bHasExpressionConnection = TRUE;
		}
		if ( MaterialNode )
		{
			bHasMaterialConnection = TRUE;
		}
	}
	else if ( MaterialEditor->ConnType == LOC_INPUT ) // Items on left side of node.
	{
		// Only expressions can have connectors on the left side.
		check( MaterialExpression );
		bHasExpressionConnection = TRUE;
	}

	// Break link.
	if( bHasExpressionConnection || bHasMaterialConnection )
	{
		Append( ID_MATERIALEDITOR_BREAK_LINK, *LocalizeUnrealEd("BreakLink"), TEXT("") );

		// add menu items to expression output for material connection
		if ( MaterialEditor->ConnType == LOC_INPUT ) // left side
		{
			AppendSeparator();
			Append( ID_MATERIALEDITOR_CONNECT_TO_DiffuseColor, *LocalizeUnrealEd("ConnectToDiffuseColor"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_EmissiveColor, *LocalizeUnrealEd("ConnectToEmissiveColor"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_SpecularColor, *LocalizeUnrealEd("ConnectToSpecularColor"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_SpecularPower, *LocalizeUnrealEd("ConnectToSpecularPower"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_Opacity, *LocalizeUnrealEd("ConnectToOpacity"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_OpacityMask, *LocalizeUnrealEd("ConnectToOpacityMask"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_Distortion, *LocalizeUnrealEd("ConnectToDistortion"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_TransmissionMask, *LocalizeUnrealEd("ConnectToTransmissionMask"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_TransmissionColor, *LocalizeUnrealEd("ConnectToTransmissionColor"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_Normal, *LocalizeUnrealEd("ConnectToNormal"), TEXT("") );
			Append( ID_MATERIALEDITOR_CONNECT_TO_CustomLighting, *LocalizeUnrealEd("ConnectToCustomLighting"), TEXT("") );
		}
	}

	// If an input connect was clicked on, append options to create new shader expressions.
	if(MaterialEditor->ConnObj)
	{
		if ( MaterialEditor->ConnType == LOC_OUTPUT )
		{
			AppendSeparator();
			wxMenu* NewConnectorMenu = new wxMenu();
			AppendNewShaderExpressionsToMenu( NewConnectorMenu, MaterialEditor );
			Append(wxID_ANY, *LocalizeUnrealEd("AddNewMaterialExpression"), NewConnectorMenu);
		}
	}
}
