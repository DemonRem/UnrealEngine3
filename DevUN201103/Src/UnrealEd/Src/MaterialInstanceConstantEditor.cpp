/**
 * MaterialInstanceConstantEditor.cpp: Material instance editor class.
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"
#include "EngineMaterialClasses.h"
#include "PropertyWindow.h"
#include "GenericBrowser.h"
#include "MaterialEditorBase.h"
#include "MaterialEditorToolBar.h"
#include "MaterialInstanceConstantEditor.h"
#include "MaterialEditorPreviewScene.h"
#include "NewMaterialEditor.h"
#include "PropertyWindowManager.h"	// required for access to GPropertyWindowManager


//////////////////////////////////////////////////////////////////////////
// UMaterialEditorInstanceConstant
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UMaterialEditorInstanceConstant);


/**Fix up for deprecated properties*/
void UMaterialEditorInstanceConstant::PostLoad ()
{
	Super::PostLoad();

	if (FlattenedTexture_DEPRECATED != NULL)
	{
		MobileBaseTexture = FlattenedTexture_DEPRECATED;
		FlattenedTexture_DEPRECATED = NULL;
	}
}


void UMaterialEditorInstanceConstant::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if(PropertyThatChanged && PropertyThatChanged->GetName()==TEXT("Parent"))
	{
		// If the parent was changed to the source instance, set it to NULL.
		if(Parent==SourceInstance)
		{
			Parent = NULL;
		}

		{
			FGlobalComponentReattachContext RecreateComponents;
			SourceInstance->SetParent(Parent);
			// Fully update static parameters before reattaching the scene's components
			SetSourceInstance(SourceInstance);
		}
	}

	CopyToSourceInstance();

	// Tell our source instance to update itself so the preview updates.
	SourceInstance->PostEditChangeProperty(PropertyChangedEvent);
}

/** Regenerates the parameter arrays. */
void UMaterialEditorInstanceConstant::RegenerateArrays()
{
	// Clear out existing parameters.
	VectorParameterValues.Empty();
	ScalarParameterValues.Empty();
	TextureParameterValues.Empty();
	FontParameterValues.Empty();
	StaticSwitchParameterValues.Empty();
	StaticComponentMaskParameterValues.Empty();
	VisibleExpressions.Empty();

	if(Parent)
	{
		// Only operate on base materials
		UMaterial* ParentMaterial = Parent->GetMaterial(MSP_SM3);
		SourceInstance->UpdateParameterNames();	// Update any parameter names that may have changed.

		// Loop through all types of parameters for this material and add them to the parameter arrays.
		TArray<FName> ParameterNames;
		TArray<FGuid> Guids;

		// Vector Parameters.
		ParentMaterial->GetAllVectorParameterNames(ParameterNames, Guids);
		VectorParameterValues.AddZeroed(ParameterNames.Num());

		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FEditorVectorParameterValue& ParameterValue = VectorParameterValues(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			FLinearColor Value;
			
			ParameterValue.bOverride = FALSE;
			ParameterValue.ParameterName = ParameterName;
			ParameterValue.ExpressionId = Guids(ParameterIdx);

			if(SourceInstance->GetVectorParameterValue(ParameterName, Value))
			{
				ParameterValue.ParameterValue = Value;
			}

			// @todo: This is kind of slow, maybe store these in a map for lookup?
			// See if this keyname exists in the source instance.
			for(INT ParameterIdx=0; ParameterIdx<SourceInstance->VectorParameterValues.Num(); ParameterIdx++)
			{
				FVectorParameterValue& SourceParam = SourceInstance->VectorParameterValues(ParameterIdx);
				if(ParameterName==SourceParam.ParameterName)
				{
					ParameterValue.bOverride = TRUE;
					ParameterValue.ParameterValue = SourceParam.ParameterValue;
				}
			}
		}

		// Scalar Parameters.
		ParentMaterial->GetAllScalarParameterNames(ParameterNames, Guids);
		ScalarParameterValues.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FEditorScalarParameterValue& ParameterValue = ScalarParameterValues(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			FLOAT Value;
			
			ParameterValue.bOverride = FALSE;
			ParameterValue.ParameterName = ParameterName;
			ParameterValue.ExpressionId = Guids(ParameterIdx);

			if(SourceInstance->GetScalarParameterValue(ParameterName, Value))
			{
				ParameterValue.ParameterValue = Value;
			}


			// @todo: This is kind of slow, maybe store these in a map for lookup?
			// See if this keyname exists in the source instance.
			for(INT ParameterIdx=0; ParameterIdx<SourceInstance->ScalarParameterValues.Num(); ParameterIdx++)
			{
				FScalarParameterValue& SourceParam = SourceInstance->ScalarParameterValues(ParameterIdx);
				if(ParameterName==SourceParam.ParameterName)
				{
					ParameterValue.bOverride = TRUE;
					ParameterValue.ParameterValue = SourceParam.ParameterValue;
				}
			}
		}

		// Texture Parameters.
		ParentMaterial->GetAllTextureParameterNames(ParameterNames, Guids);
		TextureParameterValues.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FEditorTextureParameterValue& ParameterValue = TextureParameterValues(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UTexture* Value;

			ParameterValue.bOverride = FALSE;
			ParameterValue.ParameterName = ParameterName;
			ParameterValue.ExpressionId = Guids(ParameterIdx);

			if(SourceInstance->GetTextureParameterValue(ParameterName, Value))
			{
				ParameterValue.ParameterValue = Value;
			}


			// @todo: This is kind of slow, maybe store these in a map for lookup?
			// See if this keyname exists in the source instance.
			for(INT ParameterIdx=0; ParameterIdx<SourceInstance->TextureParameterValues.Num(); ParameterIdx++)
			{
				FTextureParameterValue& SourceParam = SourceInstance->TextureParameterValues(ParameterIdx);
				if(ParameterName==SourceParam.ParameterName)
				{
					ParameterValue.bOverride = TRUE;
					ParameterValue.ParameterValue = SourceParam.ParameterValue;
				}
			}
		}

		// Font Parameters.
		ParentMaterial->GetAllFontParameterNames(ParameterNames, Guids);
		FontParameterValues.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FEditorFontParameterValue& ParameterValue = FontParameterValues(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UFont* FontValue;
			INT FontPage;

			ParameterValue.bOverride = FALSE;
			ParameterValue.ParameterName = ParameterName;
			ParameterValue.ExpressionId = Guids(ParameterIdx);

			if(SourceInstance->GetFontParameterValue(ParameterName, FontValue,FontPage))
			{
				ParameterValue.FontValue = FontValue;
				ParameterValue.FontPage = FontPage;
			}


			// @todo: This is kind of slow, maybe store these in a map for lookup?
			// See if this keyname exists in the source instance.
			for(INT ParameterIdx=0; ParameterIdx<SourceInstance->FontParameterValues.Num(); ParameterIdx++)
			{
				FFontParameterValue& SourceParam = SourceInstance->FontParameterValues(ParameterIdx);
				if(ParameterName==SourceParam.ParameterName)
				{
					ParameterValue.bOverride = TRUE;
					ParameterValue.FontValue = SourceParam.FontValue;
					ParameterValue.FontPage = SourceParam.FontPage;
				}
			}
		}

		// Get all static parameters from the source instance.  This will handle inheriting parent values.
		FStaticParameterSet SourceStaticParameters;
		SourceInstance->GetStaticParameterValues(&SourceStaticParameters);

		// Copy Static Switch Parameters
		StaticSwitchParameterValues.AddZeroed(SourceStaticParameters.StaticSwitchParameters.Num());
		for(INT ParameterIdx=0; ParameterIdx<SourceStaticParameters.StaticSwitchParameters.Num(); ParameterIdx++)
		{
			StaticSwitchParameterValues(ParameterIdx) = FEditorStaticSwitchParameterValue(SourceStaticParameters.StaticSwitchParameters(ParameterIdx));
		}

		// Copy Static Component Mask Parameters
		StaticComponentMaskParameterValues.AddZeroed(SourceStaticParameters.StaticComponentMaskParameters.Num());
		for(INT ParameterIdx=0; ParameterIdx<SourceStaticParameters.StaticComponentMaskParameters.Num(); ParameterIdx++)
		{
			StaticComponentMaskParameterValues(ParameterIdx) = FEditorStaticComponentMaskParameterValue(SourceStaticParameters.StaticComponentMaskParameters(ParameterIdx));
		}

		WxMaterialEditor::GetVisibleMaterialParameters(ParentMaterial, SourceInstance, VisibleExpressions);
	}
}

/** Copies the parameter array values back to the source instance. */
void UMaterialEditorInstanceConstant::CopyToSourceInstance()
{
	if(SourceInstance->IsTemplate(RF_ClassDefaultObject) == FALSE )
	{
		SourceInstance->MarkPackageDirty();
		SourceInstance->ClearParameterValues();

		// Scalar Parameters
		for(INT ParameterIdx=0; ParameterIdx<ScalarParameterValues.Num(); ParameterIdx++)
		{
			FEditorScalarParameterValue& ParameterValue = ScalarParameterValues(ParameterIdx);
			if(ParameterValue.bOverride)
			{
				SourceInstance->SetScalarParameterValue(ParameterValue.ParameterName, ParameterValue.ParameterValue);
			}
		}

		// Texture Parameters
		for(INT ParameterIdx=0; ParameterIdx<TextureParameterValues.Num(); ParameterIdx++)
		{
			FEditorTextureParameterValue& ParameterValue = TextureParameterValues(ParameterIdx);
			if(ParameterValue.bOverride)
			{
				SourceInstance->SetTextureParameterValue(ParameterValue.ParameterName, ParameterValue.ParameterValue);
			}
		}

		// Font Parameters
		for(INT ParameterIdx=0; ParameterIdx<FontParameterValues.Num(); ParameterIdx++)
		{
			FEditorFontParameterValue& ParameterValue = FontParameterValues(ParameterIdx);

			if(ParameterValue.bOverride)
			{
				SourceInstance->SetFontParameterValue(ParameterValue.ParameterName,ParameterValue.FontValue,ParameterValue.FontPage);
			}
		}

		// Vector Parameters
		for(INT ParameterIdx=0; ParameterIdx<VectorParameterValues.Num(); ParameterIdx++)
		{
			FEditorVectorParameterValue& ParameterValue = VectorParameterValues(ParameterIdx);

			if(ParameterValue.bOverride)
			{
				SourceInstance->SetVectorParameterValue(ParameterValue.ParameterName, ParameterValue.ParameterValue);
			}
		}

		CopyStaticParametersToSourceInstance();

		// Copy phys material back to source instance
		SourceInstance->PhysMaterial = PhysMaterial;

		// Copy physical material mask info back to the source instance
		SourceInstance->PhysMaterialMask = PhysicalMaterialMask.PhysMaterialMask;
		SourceInstance->PhysMaterialMaskUVChannel = PhysicalMaterialMask.PhysMaterialMaskUVChannel;
		SourceInstance->BlackPhysicalMaterial = PhysicalMaterialMask.BlackPhysicalMaterial;
		SourceInstance->WhitePhysicalMaterial = PhysicalMaterialMask.WhitePhysicalMaterial;

		// Copy mobile settings
		SourceInstance->MobileBaseTexture = MobileBaseTexture;

		// Copy the Lightmass settings...
		SourceInstance->SetOverrideCastShadowAsMasked(LightmassSettings.CastShadowAsMasked.bOverride);
		SourceInstance->SetCastShadowAsMasked(LightmassSettings.CastShadowAsMasked.ParameterValue);
		SourceInstance->SetOverrideEmissiveBoost(LightmassSettings.EmissiveBoost.bOverride);
		SourceInstance->SetEmissiveBoost(LightmassSettings.EmissiveBoost.ParameterValue);
		SourceInstance->SetOverrideDiffuseBoost(LightmassSettings.DiffuseBoost.bOverride);
		SourceInstance->SetDiffuseBoost(LightmassSettings.DiffuseBoost.ParameterValue);
		SourceInstance->SetOverrideSpecularBoost(LightmassSettings.SpecularBoost.bOverride);
		SourceInstance->SetSpecularBoost(LightmassSettings.SpecularBoost.ParameterValue);
		SourceInstance->SetOverrideExportResolutionScale(LightmassSettings.ExportResolutionScale.bOverride);
		SourceInstance->SetExportResolutionScale(LightmassSettings.ExportResolutionScale.ParameterValue);
		SourceInstance->SetOverrideDistanceFieldPenumbraScale(LightmassSettings.DistanceFieldPenumbraScale.bOverride);
		SourceInstance->SetDistanceFieldPenumbraScale(LightmassSettings.DistanceFieldPenumbraScale.ParameterValue);

		// Update object references and parameter names.
		SourceInstance->UpdateParameterNames();
	}
}

/** Copies static parameters to the source instance, which will be marked dirty if a compile was necessary */
void UMaterialEditorInstanceConstant::CopyStaticParametersToSourceInstance()
{
	if(SourceInstance->IsTemplate(RF_ClassDefaultObject) == FALSE )
	{
		//build a static parameter set containing all static parameter settings
		FStaticParameterSet StaticParameters;

		// Static Switch Parameters
		for(INT ParameterIdx = 0; ParameterIdx < StaticSwitchParameterValues.Num(); ParameterIdx++)
		{
			FEditorStaticSwitchParameterValue& EditorParameter = StaticSwitchParameterValues(ParameterIdx);
			UBOOL SwitchValue = EditorParameter.ParameterValue;
			FGuid ExpressionIdValue = EditorParameter.ExpressionId;
			if (!EditorParameter.bOverride)
			{
				if (Parent)
				{
					//use the parent's settings if this parameter is not overridden
					SourceInstance->Parent->GetStaticSwitchParameterValue(EditorParameter.ParameterName, SwitchValue, ExpressionIdValue);
				}
			}
			FStaticSwitchParameter * NewParameter = 
				new(StaticParameters.StaticSwitchParameters) FStaticSwitchParameter(EditorParameter.ParameterName, SwitchValue, EditorParameter.bOverride, ExpressionIdValue);
		}

		// Static Component Mask Parameters
		for(INT ParameterIdx = 0; ParameterIdx < StaticComponentMaskParameterValues.Num(); ParameterIdx++)
		{
			FEditorStaticComponentMaskParameterValue& EditorParameter = StaticComponentMaskParameterValues(ParameterIdx);
			UBOOL MaskR = EditorParameter.ParameterValue.R;
			UBOOL MaskG = EditorParameter.ParameterValue.G;
			UBOOL MaskB = EditorParameter.ParameterValue.B;
			UBOOL MaskA = EditorParameter.ParameterValue.A;
			FGuid ExpressionIdValue = EditorParameter.ExpressionId;

			if (!EditorParameter.bOverride)
			{
				if (Parent)
				{
					//use the parent's settings if this parameter is not overridden
					SourceInstance->Parent->GetStaticComponentMaskParameterValue(EditorParameter.ParameterName, MaskR, MaskG, MaskB, MaskA, ExpressionIdValue);
				}
			}
			FStaticComponentMaskParameter * NewParameter = new(StaticParameters.StaticComponentMaskParameters) 
				FStaticComponentMaskParameter(EditorParameter.ParameterName, MaskR, MaskG, MaskB, MaskA, EditorParameter.bOverride, ExpressionIdValue);
		}

		// Normal Parameters
		if (Parent)
		{
			for(INT ParameterIdx = 0; ParameterIdx < TextureParameterValues.Num(); ParameterIdx++)
			{
				FEditorTextureParameterValue& EditorParameter = TextureParameterValues(ParameterIdx);

				// Try to read the parent's settings of this parameter
				FGuid ExpressionIdValue;
				BYTE CompressionSettings;
				if( SourceInstance->Parent->GetNormalParameterValue(EditorParameter.ParameterName, CompressionSettings, ExpressionIdValue) )
				{
					// Succeeded! This texture parameter is a normal map parameter.
					// Use the values set in the editor if we should override.
					if( EditorParameter.bOverride && EditorParameter.ParameterValue )
					{
						CompressionSettings = EditorParameter.ParameterValue->CompressionSettings;
						ExpressionIdValue = EditorParameter.ExpressionId;
					}
					FNormalParameter* NewParameter = new(StaticParameters.NormalParameters) 
						FNormalParameter(EditorParameter.ParameterName, CompressionSettings, EditorParameter.bOverride, ExpressionIdValue);
				}
			}
		}

		if (SourceInstance->SetStaticParameterValues(&StaticParameters))
		{
			//mark the package dirty if a compile was needed
			SourceInstance->MarkPackageDirty();
		}
	}
}


/**  
 * Sets the source instance for this object and regenerates arrays. 
 *
 * @param MaterialInterface		Instance to use as the source for this material editor instance.
 */
void UMaterialEditorInstanceConstant::SetSourceInstance(UMaterialInstanceConstant* MaterialInterface)
{
	check(MaterialInterface);
	SourceInstance = MaterialInterface;
	Parent = SourceInstance->Parent;
	PhysMaterial = SourceInstance->PhysMaterial;

	// Copy physical material mask info from the source instance
	PhysicalMaterialMask.PhysMaterialMask = SourceInstance->PhysMaterialMask;
	PhysicalMaterialMask.PhysMaterialMaskUVChannel = SourceInstance->PhysMaterialMaskUVChannel;
	PhysicalMaterialMask.BlackPhysicalMaterial = SourceInstance->BlackPhysicalMaterial;
	PhysicalMaterialMask.WhitePhysicalMaterial = SourceInstance->WhitePhysicalMaterial;

	MobileBaseTexture = SourceInstance->MobileBaseTexture;
	// Copy the Lightmass settings...
	LightmassSettings.CastShadowAsMasked.bOverride = SourceInstance->GetOverrideCastShadowAsMasked();
	LightmassSettings.CastShadowAsMasked.ParameterValue = SourceInstance->GetCastShadowAsMasked();
	LightmassSettings.EmissiveBoost.bOverride = SourceInstance->GetOverrideEmissiveBoost();
	LightmassSettings.EmissiveBoost.ParameterValue = SourceInstance->GetEmissiveBoost();
	LightmassSettings.DiffuseBoost.bOverride = SourceInstance->GetOverrideDiffuseBoost();
	LightmassSettings.DiffuseBoost.ParameterValue = SourceInstance->GetDiffuseBoost();
	LightmassSettings.SpecularBoost.bOverride = SourceInstance->GetOverrideSpecularBoost();
	LightmassSettings.SpecularBoost.ParameterValue = SourceInstance->GetSpecularBoost();
	LightmassSettings.ExportResolutionScale.bOverride = SourceInstance->GetOverrideExportResolutionScale();
	LightmassSettings.ExportResolutionScale.ParameterValue = SourceInstance->GetExportResolutionScale();
	LightmassSettings.DistanceFieldPenumbraScale.bOverride = SourceInstance->GetOverrideDistanceFieldPenumbraScale();
	LightmassSettings.DistanceFieldPenumbraScale.ParameterValue = SourceInstance->GetDistanceFieldPenumbraScale();

	RegenerateArrays();

	//propagate changes to the base material so the instance will be updated if it has a static permutation resource
	CopyStaticParametersToSourceInstance();
	SourceInstance->UpdateStaticPermutation();
}


//////////////////////////////////////////////////////////////////////////
// WxCustomPropertyItem_MaterialInstanceConstantParameter
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS(WxCustomPropertyItem_MaterialInstanceConstantParameter, WxCustomPropertyItem_ConditionalItem);

BEGIN_EVENT_TABLE(WxCustomPropertyItem_MaterialInstanceConstantParameter, WxCustomPropertyItem_ConditionalItem)
	EVT_BUTTON(ID_MATERIALINSTANCE_CONSTANT_EDITOR_RESETTODEFAULT, OnResetToDefault)
	EVT_MENU(ID_PROP_RESET_TO_DEFAULT, WxCustomPropertyItem_MaterialInstanceConstantParameter::OnResetToDefault)
END_EVENT_TABLE()

WxCustomPropertyItem_MaterialInstanceConstantParameter::WxCustomPropertyItem_MaterialInstanceConstantParameter() : 
WxCustomPropertyItem_ConditionalItem()
{
	ResetToDefault = NULL;
	bAllowEditing = FALSE;
}

/**
 * Initialize this property window.  Must be the first function called after creating the window.
 */
void WxCustomPropertyItem_MaterialInstanceConstantParameter::Create(wxWindow* InParent)
{
	WxCustomPropertyItem_ConditionalItem::Create(InParent);

	// Set the tooltip for this custom property if we do not already have one
	if(GetToolTip() == NULL)
	{
		UMaterialEditorInstanceConstant* Instance = GetInstanceObject();
		if(Instance && Instance->Parent)
		{
			FName TempDisplayName(*DisplayName);
			FString ToolTipText;
			// Use the Description set in the Material editor
			if(Instance->Parent->GetParameterDesc(TempDisplayName, ToolTipText))
			{
				SetToolTip(*ToolTipText);
			}
		}
	}
	// Create a new button and add it to the button array.
	if(ResetToDefault==NULL)
	{
		ResetToDefault = new WxPropertyItemButton( this, ID_MATERIALINSTANCE_CONSTANT_EDITOR_RESETTODEFAULT, GPropertyWindowManager->Prop_ResetToDefaultB );
		INT OldIndentX = PropertyNode->GetIndentX();
		PropertyNode->SetIndentX(OldIndentX + 15 + PROP_ARROW_Width);

		// Generate tooltip text for this button.
		UMaterialEditorInstanceConstant* Instance = GetInstanceObject();

		if(Instance && Instance->Parent)
		{
			FString ToolTipText = *LocalizeUnrealEd("PropertyWindow_ResetToDefault");
			FName PropertyName = PropertyStructName;

			FName ScalarArrayName(TEXT("ScalarParameterValues"));
			FName TextureArrayName(TEXT("TextureParameterValues"));
			FName FontArrayName(TEXT("FontParameterValues"));
			FName VectorArrayName(TEXT("VectorParameterValues"));
			FName StaticSwitchArrayName(TEXT("StaticSwitchParameterValues"));
			FName StaticComponentMaskArrayName(TEXT("StaticComponentMaskParameterValues"));

			FName TempDisplayName(*DisplayName);
			if(PropertyName==ScalarArrayName)
			{
				FLOAT OutValue;
				if(Instance->Parent->GetScalarParameterValue(TempDisplayName, OutValue))
				{
					ToolTipText += TEXT(" ");
					ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceFloatValue_F"), OutValue));
				}
			}
			else if(PropertyName==TextureArrayName)
			{
				UTexture* OutValue;
				if(Instance->Parent->GetTextureParameterValue(TempDisplayName, OutValue))
				{
					if(OutValue)
					{
						ToolTipText += TEXT(" ");
						ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceTextureValue_F"), *OutValue->GetName()));
					}
				}				
			}
			else if(PropertyName==FontArrayName)
			{
				UFont* OutFontValue;
				INT OutFontPage;
				if(Instance->Parent->GetFontParameterValue(TempDisplayName, OutFontValue,OutFontPage))
				{
					if(OutFontValue)
					{
						ToolTipText += TEXT(" ");
						ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceFontValue_F"), *OutFontValue->GetName(),OutFontPage));
					}
				}				
			}
			else if(PropertyName==VectorArrayName)
			{
				FLinearColor OutValue;
				if(Instance->Parent->GetVectorParameterValue(TempDisplayName, OutValue))
				{
					ToolTipText += TEXT(" ");
					ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceVectorValue_F"), OutValue.R, OutValue.G, OutValue.B, OutValue.A));
				}				
			}
			else if(PropertyName==StaticSwitchArrayName)
			{
				UBOOL OutValue;
				FGuid TempGuid(0,0,0,0);
				if(Instance->Parent->GetStaticSwitchParameterValue(TempDisplayName, OutValue, TempGuid))
				{
					ToolTipText += TEXT(" ");
					ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceStaticSwitchValue_F"), (INT)OutValue));
				}				
			}
			else if(PropertyName==StaticComponentMaskArrayName)
			{
				UBOOL OutValue[4];
				FGuid TempGuid(0,0,0,0);

				if(Instance->Parent->GetStaticComponentMaskParameterValue(TempDisplayName, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
				{
					ToolTipText += TEXT(" ");
					ToolTipText += FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialInstanceStaticComponentMaskValue_F"), (INT)OutValue[0], (INT)OutValue[1], (INT)OutValue[2], (INT)OutValue[3]));
				}				
			}

			ResetToDefault->SetToolTip(*ToolTipText);
		}
	}
}

/**
 * Returns TRUE if MouseX and MouseY fall within the bounding region of the checkbox used for displaying the value of this property's edit condition.
 */
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::ClickedCheckbox( INT MouseX, INT MouseY ) const
{
	if( SupportsEditConditionCheckBox() )
	{
		//Disabling check box
		check(PropertyNode);
		wxRect ConditionalRect = GetConditionalRect();

		// Account for checkbox offset
		ConditionalRect.x += 15;

		if (ConditionalRect.Contains(MouseX, MouseY))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * Toggles the value of the property being used as the condition for editing this property.
 *
 * @return	the new value of the condition (i.e. TRUE if the condition is now TRUE)
 */
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::ToggleConditionValue()
{	
	UMaterialEditorInstanceConstant* Instance = GetInstanceObject();

	if(Instance)
	{
		FName PropertyName = PropertyStructName;
		FName ScalarArrayName(TEXT("ScalarParameterValues"));
		FName TextureArrayName(TEXT("TextureParameterValues"));
		FName FontArrayName(TEXT("FontParameterValues"));
		FName VectorArrayName(TEXT("VectorParameterValues"));
		FName StaticSwitchArrayName(TEXT("StaticSwitchParameterValues"));
		FName StaticComponentMaskArrayName(TEXT("StaticComponentMaskParameterValues"));

		if(PropertyName==ScalarArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->ScalarParameterValues.Num();ParamIdx++)
			{
				FEditorScalarParameterValue& Param = Instance->ScalarParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==TextureArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->TextureParameterValues.Num();ParamIdx++)
			{
				FEditorTextureParameterValue& Param = Instance->TextureParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==FontArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->FontParameterValues.Num();ParamIdx++)
			{
				FEditorFontParameterValue& Param = Instance->FontParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==VectorArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->VectorParameterValues.Num();ParamIdx++)
			{
				FEditorVectorParameterValue& Param = Instance->VectorParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==StaticSwitchArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->StaticSwitchParameterValues.Num();ParamIdx++)
			{
				FEditorStaticSwitchParameterValue& Param = Instance->StaticSwitchParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==StaticComponentMaskArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->StaticComponentMaskParameterValues.Num();ParamIdx++)
			{
				FEditorStaticComponentMaskParameterValue& Param = Instance->StaticComponentMaskParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					Param.bOverride = !Param.bOverride;
					break;
				}
			}
		}

		// Notify the instance that we modified an override so it needs to update itself.
		FPropertyNode* PropertyNode = GetPropertyNode();
		FPropertyChangedEvent PropertyEvent(PropertyNode->GetProperty());
		Instance->PostEditChangeProperty(PropertyEvent);
	}

	// Always allow editing even if we aren't overriding values.
	return TRUE;
}


/**
 * Returns TRUE if the value of the conditional property matches the value required.  Indicates whether editing or otherwise interacting with this item's
 * associated property should be allowed.
 */
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::IsOverridden()
{
	UMaterialEditorInstanceConstant* Instance = GetInstanceObject();

	if(Instance)
	{
		FName PropertyName = PropertyStructName;
		FName ScalarArrayName(TEXT("ScalarParameterValues"));
		FName TextureArrayName(TEXT("TextureParameterValues"));
		FName FontArrayName(TEXT("FontParameterValues"));
		FName VectorArrayName(TEXT("VectorParameterValues"));
		FName StaticSwitchArrayName(TEXT("StaticSwitchParameterValues"));
		FName StaticComponentMaskArrayName(TEXT("StaticComponentMaskParameterValues"));

		if(PropertyName==ScalarArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->ScalarParameterValues.Num();ParamIdx++)
			{
				FEditorScalarParameterValue& Param = Instance->ScalarParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==TextureArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->TextureParameterValues.Num();ParamIdx++)
			{
				FEditorTextureParameterValue& Param = Instance->TextureParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==FontArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->FontParameterValues.Num();ParamIdx++)
			{
				FEditorFontParameterValue& Param = Instance->FontParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==VectorArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->VectorParameterValues.Num();ParamIdx++)
			{
				FEditorVectorParameterValue& Param = Instance->VectorParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==StaticSwitchArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->StaticSwitchParameterValues.Num();ParamIdx++)
			{
				FEditorStaticSwitchParameterValue& Param = Instance->StaticSwitchParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
		else if(PropertyName==StaticComponentMaskArrayName)
		{
			for(INT ParamIdx=0; ParamIdx<Instance->StaticComponentMaskParameterValues.Num();ParamIdx++)
			{
				FEditorStaticComponentMaskParameterValue& Param = Instance->StaticComponentMaskParameterValues(ParamIdx);

				if(Param.ParameterName.ToString() == DisplayName)
				{
					bAllowEditing = Param.bOverride;
					break;
				}
			}
		}
	}

	return bAllowEditing;
}


/**
 * Returns TRUE if the value of the conditional property matches the value required.  Indicates whether editing or otherwise interacting with this item's
 * associated property should be allowed.
 */
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::IsConditionMet()
{
	return TRUE;
}

/** @return Returns the instance object this property is associated with. */
UMaterialEditorInstanceConstant* WxCustomPropertyItem_MaterialInstanceConstantParameter::GetInstanceObject()
{
	FObjectPropertyNode* ItemParent = PropertyNode->FindObjectItemParent();
	UMaterialEditorInstanceConstant* MaterialInterface = NULL;

	if(ItemParent)
	{
		for(FObjectPropertyNode::TObjectIterator It(ItemParent->ObjectIterator()); It; ++It)
		{
			MaterialInterface = Cast<UMaterialEditorInstanceConstant>(*It);
			break;
		}
	}

	return MaterialInterface;
}

/**
 * Renders the left side of the property window item.
 *
 * This version is responsible for rendering the checkbox used for toggling whether this property item window should be enabled.
 *
 * @param	RenderDeviceContext		the device context to use for rendering the item name
 * @param	ClientRect				the bounding region of the property window item
 */
void WxCustomPropertyItem_MaterialInstanceConstantParameter::RenderItemName( wxBufferedPaintDC& RenderDeviceContext, const wxRect& ClientRect )
{
	const UBOOL bItemEnabled = IsOverridden();

	// determine which checkbox image to render
	const WxMaskedBitmap& bmp = bItemEnabled
		? GPropertyWindowManager->CheckBoxOnB
		: GPropertyWindowManager->CheckBoxOffB;

	INT CurrentX = ClientRect.x + PropertyNode->GetIndentX() - PROP_Indent;
	INT CurrentY = ClientRect.y + ((PROP_DefaultItemHeight - bmp.GetHeight()) / 2);

	ResetToDefault->SetSize(CurrentX, CurrentY, 15, 15);
	CurrentX += 15;

	// render the checkbox bitmap
	RenderDeviceContext.DrawBitmap( bmp, CurrentX, CurrentY, 1 );
	CurrentX += PROP_Indent;

	INT W, H;
	RenderDeviceContext.GetTextExtent( *DisplayName, &W, &H );

	const INT YOffset = (PROP_DefaultItemHeight - H) / 2;
	const UBOOL bCanBeExpanded = PropertyNode->HasNodeFlags(EPropertyNodeFlags::CanBeExpanded);
	RenderDeviceContext.DrawText( *DisplayName, CurrentX+( bCanBeExpanded ? PROP_ARROW_Width : 0 ),ClientRect.y+YOffset );

	RenderDeviceContext.DestroyClippingRegion();
}

/** Reset to default button event. */
void WxCustomPropertyItem_MaterialInstanceConstantParameter::OnResetToDefault(wxCommandEvent &Event)
{
	UMaterialEditorInstanceConstant* Instance = GetInstanceObject();

	if(Instance && Instance->Parent)
	{
		FName PropertyName = PropertyStructName;

		FName ScalarArrayName(TEXT("ScalarParameterValues"));
		FName TextureArrayName(TEXT("TextureParameterValues"));
		FName FontArrayName(TEXT("FontParameterValues"));
		FName VectorArrayName(TEXT("VectorParameterValues"));
		FName StaticSwitchArrayName(TEXT("StaticSwitchParameterValues"));
		FName StaticComponentMaskArrayName(TEXT("StaticComponentMaskParameterValues"));

		FName TempDisplayName(*DisplayName);
		if(PropertyName==ScalarArrayName)
		{
			FLOAT OutValue;
			if(Instance->Parent->GetScalarParameterValue(TempDisplayName, OutValue))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->ScalarParameterValues.Num(); PropertyIdx++)
				{
					FEditorScalarParameterValue& Value = Instance->ScalarParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.ParameterValue = OutValue;
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}
		}
		else if(PropertyName==TextureArrayName)
		{
			UTexture* OutValue;
			if(Instance->Parent->GetTextureParameterValue(TempDisplayName, OutValue))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->TextureParameterValues.Num(); PropertyIdx++)
				{
					FEditorTextureParameterValue& Value = Instance->TextureParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.ParameterValue = OutValue;
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}				
		}
		else if(PropertyName==FontArrayName)
		{
			UFont* OutFontValue;
			INT OutFontPage;
			if(Instance->Parent->GetFontParameterValue(TempDisplayName, OutFontValue,OutFontPage))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->FontParameterValues.Num(); PropertyIdx++)
				{
					FEditorFontParameterValue& Value = Instance->FontParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.FontValue = OutFontValue;
						Value.FontPage = OutFontPage;
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}				
		}
		else if(PropertyName==VectorArrayName)
		{
			FLinearColor OutValue;
			if(Instance->Parent->GetVectorParameterValue(TempDisplayName, OutValue))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->VectorParameterValues.Num(); PropertyIdx++)
				{
					FEditorVectorParameterValue& Value = Instance->VectorParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.ParameterValue = OutValue;
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}				
		}
		else if(PropertyName==StaticSwitchArrayName)
		{
			UBOOL OutValue;
			FGuid TempGuid(0,0,0,0);
			if(Instance->Parent->GetStaticSwitchParameterValue(TempDisplayName, OutValue, TempGuid))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->StaticSwitchParameterValues.Num(); PropertyIdx++)
				{
					FEditorStaticSwitchParameterValue& Value = Instance->StaticSwitchParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.ParameterValue = OutValue;
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}				
		}
		else if(PropertyName==StaticComponentMaskArrayName)
		{
			UBOOL OutValue[4];
			FGuid TempGuid(0,0,0,0);

			if(Instance->Parent->GetStaticComponentMaskParameterValue(TempDisplayName, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
			{
				for(INT PropertyIdx=0; PropertyIdx<Instance->StaticComponentMaskParameterValues.Num(); PropertyIdx++)
				{
					FEditorStaticComponentMaskParameterValue& Value = Instance->StaticComponentMaskParameterValues(PropertyIdx);
					if(Value.ParameterName == TempDisplayName)
					{
						Value.ParameterValue.R = OutValue[0];
						Value.ParameterValue.G = OutValue[1];
						Value.ParameterValue.B = OutValue[2];
						Value.ParameterValue.A = OutValue[3];
						Instance->CopyToSourceInstance();
						break;
					}
				}
			}				
		}

		// Rebuild property window to update the values.
		GetPropertyWindow()->Rebuild();
		GetPropertyWindow()->RequestMainWindowTakeFocus();
		check(PropertyNode);
		PropertyNode->InvalidateChildControls();
	}
}

/**
* Overriden function to allow hiding when not referenced.
*/
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::IsDerivedForcedHide (void) const
{
	const WxPropertyWindowHost* PropertyWindowHost = GetPropertyWindow()->GetParentHostWindow();
	check(PropertyWindowHost);
	const wxWindow *HostParent   = PropertyWindowHost->GetParent();
	const WxMaterialInstanceConstantEditor *Win = wxDynamicCast( HostParent, WxMaterialInstanceConstantEditor );
	UBOOL ForceHide = TRUE;

	// When property window is floating (not docked) the parent is an wxAuiFloatingFrame.
	// We must get at the Editor via the docking system:
	//                      wxAuiFloatingFrame -> wxAuiManager -> ManagedWindow
	if (Win == NULL)
	{
		wxAuiFloatingFrame *OwnerFloatingFrame = wxDynamicCast( HostParent, wxAuiFloatingFrame );
		check(OwnerFloatingFrame);
		wxAuiManager *AuiManager = OwnerFloatingFrame->GetOwnerManager();
		Win = wxDynamicCast(AuiManager->GetManagedWindow(), WxMaterialInstanceConstantEditor);
	}
	check(Win);

	UMaterialEditorInstanceConstant* MaterialInterface = Win->MaterialEditorInstance;
	check(MaterialInterface);

	if(Win->ToolBar->GetToolState(ID_MATERIALINSTANCE_CONSTANT_EDITOR_SHOWALLPARAMETERS) || MaterialInterface->VisibleExpressions.ContainsItem(ExpressionId))
	{
		ForceHide = FALSE;
	}

	return ForceHide;
}


/**
 * Called when an property window item receives a left-mouse-button press which wasn't handled by the input proxy.  Typical response is to gain focus
 * and (if the property window item is expandable) to toggle expansion state.
 *
 * @param	Event	the mouse click input that generated the event
 *
 * @return	TRUE if this property window item should gain focus as a result of this mouse input event.
 */
UBOOL WxCustomPropertyItem_MaterialInstanceConstantParameter::ClickedPropertyItem( wxMouseEvent& Event )
{
	FPropertyNode* PropertyNode = GetPropertyNode();
	UProperty* NodeProperty = GetProperty();
	check(PropertyNode);

	UBOOL bShouldGainFocus = TRUE;

	// if this property is edit-const, it can't be changed
	// or if we couldn't find a valid condition property, also use the base version
	if ( NodeProperty == NULL || PropertyNode->IsEditConst() )
	{
		bShouldGainFocus = WxItemPropertyControl::ClickedPropertyItem(Event);
	}

	// if they clicked on the checkbox, toggle the edit condition
	else if ( ClickedCheckbox(Event.GetX(), Event.GetY()) )
	{
		
		NotifyPreChange(NodeProperty);
		bShouldGainFocus = !HasNodeFlags(EPropertyNodeFlags::CanBeExpanded);
		if ( ToggleConditionValue() == FALSE )
		{
			bShouldGainFocus = FALSE;

			// if we just disabled the condition which allows us to edit this control
			// collapse the item if this is an expandable item
			const UBOOL bExpand   = FALSE;
			const UBOOL bRecurse  = FALSE;
			PropertyNode->SetExpanded(bExpand, bRecurse);
		}

		// Note the current property window so that CALLBACK_ObjectPropertyChanged
		// doesn't destroy the window out from under us.
		WxPropertyWindow* MainWindow = GetPropertyWindow();
		check(MainWindow);
		MainWindow->ChangeActiveCallbackCount(1);

		const UBOOL bTopologyChange = FALSE;
		FPropertyChangedEvent ChangeEvent(NodeProperty, bTopologyChange);
		NotifyPostChange(ChangeEvent);

		// Unset, effectively making this property window updatable by CALLBACK_ObjectPropertyChanged.
		MainWindow->ChangeActiveCallbackCount(-1);
	}
	// if the condition for editing this control has been met (i.e. the checkbox is checked), pass the event back to the base version, which will do the right thing
	// based on where the user clicked
	else if ( IsConditionMet() )
	{
		bShouldGainFocus = WxItemPropertyControl::ClickedPropertyItem(Event);
	}
	else
	{
		// the condition is false, so this control isn't allowed to do anything - swallow the event.
		bShouldGainFocus = FALSE;
	}

	return bShouldGainFocus;
}


//////////////////////////////////////////////////////////////////////////
// WxPropertyWindow_MaterialInstanceConstantParameters
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(WxPropertyWindow_MaterialInstanceConstantParameters, WxItemPropertyControl);

// Called by Expand(), creates any child items for any properties within this item.
void WxPropertyWindow_MaterialInstanceConstantParameters::InitWindowDefinedChildNodes(void)
{
	check(PropertyNode);
	WxPropertyWindow* MainWindow = GetPropertyWindow();
	check(MainWindow);
	UProperty* Property = GetProperty();
	check(Property);

	FName PropertyName = Property->GetFName();
	UStructProperty* StructProperty = Cast<UStructProperty>(Property,CLASS_IsAUStructProperty);
	UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
	UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property,CLASS_IsAUObjectProperty);

	// Copy IsSorted() flag from parent.  Default sorted to TRUE if no parent exists.

	// Expand array.
	PropertyNode->SetNodeFlags(EPropertyNodeFlags::SortChildren, FALSE);
	if( Property->ArrayDim > 1 && PropertyNode->GetArrayIndex() == -1 )
	{
		for( INT i = 0 ; i < Property->ArrayDim ; i++ )
		{
			FItemPropertyNode* NewItemNode = new FItemPropertyNode;//;//CreatePropertyItem(ArrayProperty,i,this);
			WxItemPropertyControl* pwi = PropertyNode->CreatePropertyItem(Property,i);
			NewItemNode->InitNode(pwi, PropertyNode, MainWindow, Property, i*Property->ElementSize, i);
			PropertyNode->AddChildNode(NewItemNode);
		}
	}
	else if( ArrayProperty )
	{
		FScriptArray* Array = NULL;
		TArray<BYTE*> Addresses;
		if ( PropertyNode->GetReadAddress( PropertyNode, PropertyNode->HasNodeFlags(EPropertyNodeFlags::SingleSelectOnly), Addresses ) )
		{
			Array = (FScriptArray*)Addresses(0);
		}

		FObjectPropertyNode* ObjectNode = PropertyNode->FindObjectItemParent();
		UMaterialEditorInstanceConstant* MaterialInterface = NULL;
		UMaterial* Material = NULL;


		if(ObjectNode)
		{
			for(FObjectPropertyNode::TObjectIterator It(ObjectNode->ObjectIterator()); It; ++It)
			{
				MaterialInterface = Cast<UMaterialEditorInstanceConstant>(*It);
				Material = MaterialInterface->SourceInstance->GetMaterial(MSP_SM3);
				break;
			}
		}

		if( Array && Material )
		{
			FName ParameterValueName(TEXT("ParameterValue"));
			FName ScalarArrayName(TEXT("ScalarParameterValues"));
			FName TextureArrayName(TEXT("TextureParameterValues"));
			FName FontArrayName(TEXT("FontParameterValues"));
			FName VectorArrayName(TEXT("VectorParameterValues"));
			FName StaticSwitchArrayName(TEXT("StaticSwitchParameterValues"));
			FName StaticComponentMaskArrayName(TEXT("StaticComponentMaskParameterValues"));

			// Make sure that the inner of this array is a material instance parameter struct.
			UStructProperty* StructProperty = Cast<UStructProperty>(ArrayProperty->Inner);
		
			if(StructProperty)
			{	
				// Iterate over all possible fields of this struct until we find the value field, we want to combine
				// the name and value of the parameter into one property window item.  We do this by adding a item for the value
				// and overriding the name of the item using the name from the parameter.
				for( TFieldIterator<UProperty> It(StructProperty->Struct); It; ++It )
				{
					UProperty* StructMember = *It;
					if( MainWindow->HasFlags(EPropertyWindowFlags::ShouldShowNonEditable) || (StructMember->PropertyFlags&CPF_Edit) )
					{
						// Loop through all elements of this array and add properties for each one.
						for( INT ArrayIdx = 0 ; ArrayIdx < Array->Num() ; ArrayIdx++ )
						{	
							WxItemPropertyControl* pwi = PropertyNode->CreatePropertyItem(StructMember,INDEX_NONE);
							WxCustomPropertyItem_MaterialInstanceConstantParameter* PropertyWindowItem = wxDynamicCast(pwi, WxCustomPropertyItem_MaterialInstanceConstantParameter);
							if( PropertyWindowItem == NULL )
							{
								debugf( TEXT( "PropertyName was NULL: %s "), *StructMember->GetName() );
								continue;
							}

							if(StructMember->GetFName() == ParameterValueName)
							{
								// Find a name for the parameter property we are adding.
								FName OverrideName = NAME_None;
								BYTE* ElementData = ((BYTE*)Array->GetData())+ArrayIdx*ArrayProperty->Inner->ElementSize;

								if(PropertyName==ScalarArrayName)
								{
									FEditorScalarParameterValue* Param = (FEditorScalarParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}
								else if(PropertyName==TextureArrayName)
								{
									FEditorTextureParameterValue* Param = (FEditorTextureParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}
								else if(PropertyName==FontArrayName)
								{
									FEditorFontParameterValue* Param = (FEditorFontParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}
								else if(PropertyName==VectorArrayName)
								{
									FEditorVectorParameterValue* Param = (FEditorVectorParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}
								else if(PropertyName==StaticSwitchArrayName)
								{
									FEditorStaticSwitchParameterValue* Param = (FEditorStaticSwitchParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}
								else if(PropertyName==StaticComponentMaskArrayName)
								{
									FEditorStaticComponentMaskParameterValue* Param = (FEditorStaticComponentMaskParameterValue*)(ElementData);
									OverrideName = (Param->ParameterName);
									PropertyWindowItem->ExpressionId = Param->ExpressionId;
								}

								WxPropertyWindowHost* PropertyWindowHost = GetPropertyWindow()->GetParentHostWindow();
								check(PropertyWindowHost);
								wxWindow *HostParent   = PropertyWindowHost->GetParent();
								WxMaterialInstanceConstantEditor *Win = wxDynamicCast( HostParent, WxMaterialInstanceConstantEditor );
								// When property window is floating (not docked) the parent is an wxAuiFloatingFrame.
								// We must get at the Editor via the docking system:
								//                      wxAuiFloatingFrame -> wxAuiManager -> ManagedWindow
								if (Win == NULL)
								{
									wxAuiFloatingFrame *OwnerFloatingFrame = wxDynamicCast( HostParent, wxAuiFloatingFrame );
									check(OwnerFloatingFrame);
									wxAuiManager *AuiManager = OwnerFloatingFrame->GetOwnerManager();
									Win = wxDynamicCast(AuiManager->GetManagedWindow(), WxMaterialInstanceConstantEditor);
								}
								check(Win);

								//All nodes need to get created the first time through
								//if(Win->ToolBar->GetToolState(ID_MATERIALINSTANCE_CONSTANT_EDITOR_SHOWALLPARAMETERS) || MaterialInterface->VisibleExpressions.ContainsItem(ExpressionId))
								{
									// Add the property.
									PropertyWindowItem->PropertyStructName = PropertyName;
									PropertyWindowItem->SetDisplayName(OverrideName.ToString());

									FItemPropertyNode* NewItemNode = new FItemPropertyNode;//;//CreatePropertyItem(ArrayProperty,i,this);
									NewItemNode->InitNode(pwi, PropertyNode, MainWindow, StructMember, ArrayIdx*ArrayProperty->Inner->ElementSize+StructMember->Offset, ArrayIdx);
									PropertyNode->AddChildNode(NewItemNode);
								}
							}
						}
					}
				}
			}
		}
	}
	PropertyNode->SetNodeFlags(EPropertyNodeFlags::SortChildren, TRUE);
}


//////////////////////////////////////////////////////////////////////////
//
//	WxMaterialInstanceConstantEditor
//
//////////////////////////////////////////////////////////////////////////

/**
 * wxWidgets Event Table
 */
BEGIN_EVENT_TABLE(WxMaterialInstanceConstantEditor, WxMaterialEditorBase)
	EVT_CLOSE( OnClose )
	EVT_MENU(ID_MATERIALINSTANCE_CONSTANT_EDITOR_SYNCTOGB, OnMenuSyncToGB)
	EVT_MENU(ID_MATERIALINSTANCE_CONSTANT_EDITOR_OPENEDITOR, OnMenuOpenEditor)
	EVT_LIST_ITEM_ACTIVATED(ID_MATERIALINSTANCE_CONSTANT_EDITOR_LIST, OnInheritanceListDoubleClick)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_MATERIALINSTANCE_CONSTANT_EDITOR_LIST, OnInheritanceListRightClick)
	EVT_TOOL(ID_MATERIALINSTANCE_CONSTANT_EDITOR_SHOWALLPARAMETERS, OnShowAllMaterialParameters)
END_EVENT_TABLE()

WxMaterialInstanceConstantEditor::WxMaterialInstanceConstantEditor( wxWindow* Parent, wxWindowID id, UMaterialInterface* InMaterialInterface ) :	
        WxMaterialEditorBase( Parent, id, InMaterialInterface ),   
		FDockingParent(this)
{
	// Set the static mesh editor window title to include the static mesh being edited.
	SetTitle( *FString::Printf( LocalizeSecure(LocalizeUnrealEd("MaterialInstanceEditorCaption_F"), *InMaterialInterface->GetPathName()) ) );

	// Construct a temp holder for our instance parameters.
	UMaterialInstanceConstant* InstanceConstant = Cast<UMaterialInstanceConstant>(InMaterialInterface);
	MaterialEditorInstance = ConstructObject<UMaterialEditorInstanceConstant>(UMaterialEditorInstanceConstant::StaticClass());
	MaterialEditorInstance->SetSourceInstance(InstanceConstant);
	
	// Create toolbar
	ToolBar = new WxMaterialInstanceConstantEditorToolBar( this, -1 );
	SetToolBar( ToolBar );

	// Create property window
	PropertyWindow = new WxPropertyWindowHost;
	PropertyWindow->Create( this, this );
	PropertyWindow->SetFlags(EPropertyWindowFlags::SupportsCustomControls, TRUE);
	PropertyWindow->SetPropOffset(0); // Base offset for drawing arrows is subtracted, so want none to be applied to give room for favorites stars 

	SetSize(1024,768);
	FWindowUtil::LoadPosSize( TEXT("MaterialInstanceEditor"), this, 64,64,800,450 );


	// Add inheritance list.
	InheritanceList = new WxListView(this, ID_MATERIALINSTANCE_CONSTANT_EDITOR_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	RebuildInheritanceList();

	// Add docking windows.
	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Left, *FString::Printf(LocalizeSecure(LocalizeUnrealEd(TEXT("PropertiesCaption_F")), *MaterialInterface->GetPathName())), *LocalizeUnrealEd(TEXT("Properties")));
		AddDockingWindow(InheritanceList, FDockingParent::DH_Left, *FString::Printf(LocalizeSecure(LocalizeUnrealEd(TEXT("MaterialInstanceParent_F")), *MaterialInterface->GetPathName())), *LocalizeUnrealEd(TEXT("MaterialInstanceParent")));

		SetDockHostSize(FDockingParent::DH_Left, 300);

		AddDockingWindow( ( wxWindow* )PreviewWin, FDockingParent::DH_None, NULL );

		// Try to load a existing layout for the docking windows.
		LoadDockingLayout();
	}

	// Add docking menu
	wxMenuBar* MenuBar = new wxMenuBar();
	AppendWindowMenu(MenuBar);
	SetMenuBar(MenuBar);

	// Load editor settings.
	LoadSettings();

	// Set the preview mesh for the material.  This call must occur after the toolbar is initialized.
	if ( !SetPreviewMesh( *InMaterialInterface->PreviewMesh ) )
	{
		// The material preview mesh couldn't be found or isn't loaded.  Default to our primitive type.
		SetPrimitivePreview();
	}

	//move this lower, so the layout would already be established.  Otherwise windows will not clip to the parent window properly
	PropertyWindow->SetObject( MaterialEditorInstance, EPropertyWindowFlags::Sorted );
}

WxMaterialInstanceConstantEditor::~WxMaterialInstanceConstantEditor()
{
	SaveSettings();

	PropertyWindow->SetObject( NULL, EPropertyWindowFlags::NoFlags );
	delete PropertyWindow;
}

void WxMaterialInstanceConstantEditor::OnClose(wxCloseEvent& In)
{
	// flatten the MIC if it was marked dirty while editing the MIC
	GCallbackEvent->Send(CALLBACK_MobileFlattenedTextureUpdate, MaterialEditorInstance->SourceInstance);

	// close the window
	Destroy();
}

void WxMaterialInstanceConstantEditor::Serialize(FArchive& Ar)
{
	WxMaterialEditorBase::Serialize(Ar);

	// Serialize our custom object instance
	Ar<<MaterialEditorInstance;

	// Serialize all parent material instances that are stored in the list.
	Ar<<ParentList;
}

/** Pre edit change notify for properties. */
void WxMaterialInstanceConstantEditor::NotifyPreChange(void* Src, UProperty* PropertyThatChanged)
{
	// If they changed the parent, kill the current object in the property window since we will need to remake it.
	if(PropertyThatChanged->GetName()==TEXT("Parent"))
	{
		// Collapse all of the property arrays, since they will be changing with the new parent.
		PropertyWindow->CollapseItem(TEXT("VectorParameterValues"));
		PropertyWindow->CollapseItem(TEXT("ScalarParameterValues"));
		PropertyWindow->CollapseItem(TEXT("TextureParameterValues"));
		PropertyWindow->CollapseItem(TEXT("FontParameterValues"));
		PropertyWindow->CollapseItem(TEXT("StaticSwitchParameterValues"));
		PropertyWindow->CollapseItem(TEXT("StaticComponentMaskParameterValues"));
	}
}

/** Post edit change notify for properties. */
void WxMaterialInstanceConstantEditor::NotifyPostChange(void* Src, UProperty* PropertyThatChanged)
{
	// Update the preview window when the user changes a property.
	RefreshPreviewViewport();

	// If they changed the parent, regenerate the parent list.
	if(PropertyThatChanged->GetName()==TEXT("Parent"))
	{
		UBOOL bSetEmptyParent = FALSE;

		// Check to make sure they didnt set the parent to themselves.
		if(MaterialEditorInstance->Parent==MaterialInterface)
		{
			bSetEmptyParent = TRUE;
		}

		if (bSetEmptyParent)
		{
			MaterialEditorInstance->Parent = NULL;

			UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MaterialInterface);
			if(MIC)
			{
				MIC->SetParent(NULL);
			}
		}

		RebuildInheritanceList();

		//We've invalidated the tree structure, rebuild it
		PropertyWindow->RequestReconnectToData();
	}

	//rebuild the property window to account for the possibility that the item changed was
	//a static switch
	if(PropertyThatChanged->GetName() == TEXT("StaticSwitchParameterValues") && MaterialEditorInstance->Parent )
	{
		TArray<FGuid> PreviousExpressions(MaterialEditorInstance->VisibleExpressions);
		MaterialEditorInstance->VisibleExpressions.Empty();
		WxMaterialEditor::GetVisibleMaterialParameters(MaterialEditorInstance->Parent->GetMaterial(MSP_SM3), MaterialEditorInstance->SourceInstance, MaterialEditorInstance->VisibleExpressions);

		//check to see if it was the override button that was clicked or the value of the static switch
		//by comparing the values of the previous and current visible expression lists
		UBOOL bHasChanged = PreviousExpressions.Num() != MaterialEditorInstance->VisibleExpressions.Num();

		if(!bHasChanged)
		{
			for(INT Index = 0; Index < PreviousExpressions.Num(); ++Index)
			{
				if(PreviousExpressions(Index) != MaterialEditorInstance->VisibleExpressions(Index))
				{
					bHasChanged = TRUE;
					break;
				}
			}
		}

		if(bHasChanged)
		{
			PropertyWindow->PostRebuild();
		}
	}
}

/** Rebuilds the inheritance list for this material instance. */
void WxMaterialInstanceConstantEditor::RebuildInheritanceList()
{
	InheritanceList->DeleteAllColumns();
	InheritanceList->DeleteAllItems();
	ParentList.Empty();
	
	InheritanceList->Freeze();
	{
		InheritanceList->InsertColumn(0, *LocalizeUnrealEd("Parent"));
		InheritanceList->InsertColumn(1, *LocalizeUnrealEd("Name"));

		// Travel up the parent chain for this material instance until we reach the root material.
		UMaterialInstanceConstant* InstanceConstant = Cast<UMaterialInstanceConstant>(MaterialInterface);

		if(InstanceConstant)
		{
			long CurrentIdx = InheritanceList->InsertItem(0, *InstanceConstant->GetName());
			wxFont CurrentFont = InheritanceList->GetItemFont(CurrentIdx);
			CurrentFont.SetWeight(wxFONTWEIGHT_BOLD);
			InheritanceList->SetItemFont(CurrentIdx, CurrentFont);
			InheritanceList->SetItem(CurrentIdx, 1, *InstanceConstant->GetName());

			ParentList.AddItem(InstanceConstant);

			// Add all parents
			UMaterialInterface* Parent = InstanceConstant->Parent;
			while(Parent && Parent != InstanceConstant)
			{
				long ItemIdx = InheritanceList->InsertItem(0, *(Parent->GetName()));
				InheritanceList->SetItem(ItemIdx, 1, *Parent->GetName());

				ParentList.InsertItem(Parent,0);

				// If the parent is a material then break.
				InstanceConstant = Cast<UMaterialInstanceConstant>(Parent);

				if(InstanceConstant)
				{
					Parent = InstanceConstant->Parent;
				}
				else
				{
					break;
				}
			}

			// Loop through all the items and set their first column.
			INT NumItems = InheritanceList->GetItemCount();
			for(INT ItemIdx=0; ItemIdx<NumItems; ItemIdx++)
			{
				if(ItemIdx==0)
				{
					InheritanceList->SetItem(ItemIdx, 0, *LocalizeUnrealEd("Material"));
				}
				else
				{
					if(ItemIdx < NumItems - 1)
					{
						InheritanceList->SetItem(ItemIdx,0,
							*FString::Printf(TEXT("%s %i"), *LocalizeUnrealEd("Parent"), NumItems-1-ItemIdx));
					}
					else
					{
						InheritanceList->SetItem(ItemIdx, 0, *LocalizeUnrealEd("Current"));
					}
				}
			}
		}

		// Autosize columns
		InheritanceList->SetColumnWidth(0, wxLIST_AUTOSIZE);
		InheritanceList->SetColumnWidth(1, wxLIST_AUTOSIZE);
	}
	InheritanceList->Thaw();
}

/**
 * Draws messages on the canvas.
 */
void WxMaterialInstanceConstantEditor::DrawMessages(FViewport* Viewport,FCanvas* Canvas)
{
	Canvas->PushAbsoluteTransform(FMatrix::Identity);
	if (MaterialEditorInstance->Parent)
	{
		const FMaterialResource* MaterialResource = MaterialEditorInstance->SourceInstance->GetMaterialResource();
		const UMaterial* BaseMaterial = MaterialEditorInstance->SourceInstance->GetMaterial(MSP_BASE);
		INT DrawPositionY = 5;
		if (BaseMaterial && MaterialResource)
		{
			DrawMaterialInfoStrings(Canvas, BaseMaterial, MaterialResource, DrawPositionY);
		}
	}
	Canvas->PopTransform();
}

/** Saves editor settings. */
void WxMaterialInstanceConstantEditor::SaveSettings()
{
	FWindowUtil::SavePosSize( TEXT("MaterialInstanceEditor"), this );

	SaveDockingLayout();

	GConfig->SetBool(TEXT("MaterialInstanceEditor"), TEXT("bShowGrid"), bShowGrid, GEditorUserSettingsIni);
	GConfig->SetBool(TEXT("MaterialInstanceEditor"), TEXT("bDrawGrid"), PreviewVC->IsRealtime(), GEditorUserSettingsIni);
	GConfig->SetInt(TEXT("MaterialInstanceEditor"), TEXT("PrimType"), PreviewPrimType, GEditorUserSettingsIni);
}

/** Loads editor settings. */
void WxMaterialInstanceConstantEditor::LoadSettings()
{
	UBOOL bRealtime=FALSE;

	GConfig->GetBool(TEXT("MaterialInstanceEditor"), TEXT("bShowGrid"), bShowGrid, GEditorUserSettingsIni);
	GConfig->GetBool(TEXT("MaterialInstanceEditor"), TEXT("bDrawGrid"), bRealtime, GEditorUserSettingsIni);

	INT PrimType;
	if(GConfig->GetInt(TEXT("MaterialInstanceEditor"), TEXT("PrimType"), PrimType, GEditorUserSettingsIni))
	{
		PreviewPrimType = (EThumbnailPrimType)PrimType;
	}
	else
	{
		PreviewPrimType = TPT_Sphere;
	}

	if(PreviewVC)
	{
		PreviewVC->SetShowGrid(bShowGrid);
		PreviewVC->SetRealtime(bRealtime);
	}
}

/** Syncs the GB to the selected parent in the inheritance list. */
void WxMaterialInstanceConstantEditor::SyncSelectedParentToGB()
{
	INT SelectedItem = (INT)InheritanceList->GetFirstSelected();
	if(ParentList.IsValidIndex(SelectedItem))
	{
		UMaterialInterface* SelectedMaterialInstance = ParentList(SelectedItem);
		TArray<UObject*> Objects;

		Objects.AddItem(SelectedMaterialInstance);
		GApp->EditorFrame->SyncBrowserToObjects(Objects);
	}
}

/** Opens the editor for the selected parent. */
void WxMaterialInstanceConstantEditor::OpenSelectedParentEditor()
{
	INT SelectedItem = (INT)InheritanceList->GetFirstSelected();
	if(ParentList.IsValidIndex(SelectedItem))
	{
		UMaterialInterface* SelectedMaterialInstance = ParentList(SelectedItem);

		// See if its a material or material instance constant.  Don't do anything if the user chose the current material instance.
		if(MaterialInterface!=SelectedMaterialInstance)
		{
			if(SelectedMaterialInstance->IsA(UMaterial::StaticClass()))
			{
				// Show material editor
				UMaterial* Material = Cast<UMaterial>(SelectedMaterialInstance);
				wxFrame* MaterialEditor = new WxMaterialEditor( (wxWindow*)GApp->EditorFrame,-1,Material );
				MaterialEditor->SetSize(1024,768);
				MaterialEditor->Show();
			}
			else if(SelectedMaterialInstance->IsA(UMaterialInstanceConstant::StaticClass()))
			{
				// Show material instance editor
				UMaterialInstanceConstant* MaterialInstanceConstant = Cast<UMaterialInstanceConstant>(SelectedMaterialInstance);
				wxFrame* MaterialInstanceEditor = new WxMaterialInstanceConstantEditor( (wxWindow*)GApp->EditorFrame,-1, MaterialInstanceConstant );
				MaterialInstanceEditor->SetSize(1024,768);
				MaterialInstanceEditor->Show();
			}
		}
	}
}

/** Event handler for when the user wants to sync the GB to the currently selected parent. */
void WxMaterialInstanceConstantEditor::OnMenuSyncToGB(wxCommandEvent &Event)
{
	SyncSelectedParentToGB();
}

/** Event handler for when the user wants to open the editor for the selected parent material. */
void WxMaterialInstanceConstantEditor::OnMenuOpenEditor(wxCommandEvent &Event)
{
	OpenSelectedParentEditor();
}

/** Double click handler for the inheritance list. */
void WxMaterialInstanceConstantEditor::OnInheritanceListDoubleClick(wxListEvent &ListEvent)
{
	OpenSelectedParentEditor();
}

/** Event handler for when the user right clicks on the inheritance list. */
void WxMaterialInstanceConstantEditor::OnInheritanceListRightClick(wxListEvent &ListEvent)
{
	INT SelectedItem = (INT)InheritanceList->GetFirstSelected();
	if(ParentList.IsValidIndex(SelectedItem))
	{
		UMaterialInterface* SelectedMaterialInstance = ParentList(SelectedItem);

		wxMenu* ContextMenu = new wxMenu();

		if(SelectedMaterialInstance != MaterialInterface)
		{
			FString Label;

			if(SelectedMaterialInstance->IsA(UMaterial::StaticClass()))
			{
				Label = LocalizeUnrealEd("MaterialEditor");
			}
			else
			{
				Label = LocalizeUnrealEd("MaterialInstanceEditor");
			}

			ContextMenu->Append(ID_MATERIALINSTANCE_CONSTANT_EDITOR_OPENEDITOR, *Label);
		}

		ContextMenu->Append(ID_MATERIALINSTANCE_CONSTANT_EDITOR_SYNCTOGB, *LocalizeUnrealEd("SyncGenericBrowser"));

		PopupMenu(ContextMenu);
	}
}

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxMaterialInstanceConstantEditor::GetDockingParentName() const
{
	return TEXT("MaterialInstanceEditor");
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxMaterialInstanceConstantEditor::GetDockingParentVersion() const
{
	return 1;
}

/** Event handler for when the user wants to toggle showing all material parameters. */
void WxMaterialInstanceConstantEditor::OnShowAllMaterialParameters(wxCommandEvent &Event)
{
	PropertyWindow->RequestMainWindowTakeFocus();
	PropertyWindow->PostRebuild();
}

UMaterialInterface* WxMaterialInstanceConstantEditor::GetSyncObject()
{
	if (MaterialEditorInstance)
	{
		return MaterialEditorInstance->SourceInstance;
//		return Cast<UMaterialInterface>(MaterialEditorInstance->SourceInstance);
	}
	return NULL;
}

