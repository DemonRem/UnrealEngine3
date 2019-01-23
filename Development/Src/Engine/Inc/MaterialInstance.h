/**
 *	
 *	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


#ifndef _MATERIAL_INSTANCE_H_
#define _MATERIAL_INSTANCE_H_



/**
 * Protects the members of a UMaterialInstanceConstant from re-entrance.
 */
struct FMICReentranceGuard
{
	UMaterialInstance*	Material;

	FMICReentranceGuard(UMaterialInstance* InMaterial):
	Material(InMaterial)
	{
		check(!Material->ReentrantFlag);
		Material->ReentrantFlag = 1;
	}

	~FMICReentranceGuard()
	{
		Material->ReentrantFlag = 0;
	}
};

/** Struct to manage the Render data needed for TimeVaring Data**/
struct FTimeVaryingDataType
{
	FLOAT StartTime; 
	FLOAT ScalarValue;
	FInterpCurveFloat TheCurve;

	FTimeVaryingDataType(): StartTime(0.0f), ScalarValue(0.0f)
	{
		// we need to appMemzero this as we are using a TArrayNoInit in the FInterpCurveFloat so we can use copy contructor nicely
		appMemzero( &this->TheCurve.Points, sizeof(this->TheCurve.Points) );
	}
};


/**
* The resource used to render a UMaterialInstance.
*/
class FMaterialInstanceResource: public FMaterialRenderProxy, public FDeferredCleanupInterface
{
public:

	struct DataType
	{
		UMaterialInstance* ParentInstance;
		UMaterialInterface* Parent;
		TMap<FName,FLinearColor> VectorParameterMap;
		TMap<FName,FLOAT> ScalarParameterMap;
		TMap<FName,const UTexture*> TextureParameterMap;
		// add here or make a new class? 
		TMap<FName,FTimeVaryingDataType> ScalarOverTimeParameterMap;
	};


	/** Initialization constructor. */
	FMaterialInstanceResource(UMaterialInstance* InOwner,UBOOL bInSelected):
	Owner(InOwner),
		bSelected(bInSelected),
		GameThreadParent(NULL)
	{}

	// FDeferredCleanupInterface
	virtual void FinishCleanup()
	{
		delete this;
	}

	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial() const
	{
		check(IsInRenderingThread());
		const FMaterial * InstanceMaterial = Data.ParentInstance->GetMaterialResource();
		//if the instance contains a static permutation resource, use that
		if (InstanceMaterial && InstanceMaterial->GetShaderMap())
		{
			return InstanceMaterial;
		}
		else if (Data.ParentInstance->bHasStaticPermutationResource)
		{
			//there was an error, use the default material's resource
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected)->GetMaterial();
		}
		else
		{
			//use the parent's material resource
			return Data.Parent->GetRenderProxy(bSelected)->GetMaterial();
		}

	}

	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		check(IsInRenderingThread());
		const FLinearColor* Value = Data.VectorParameterMap.Find(ParameterName);
		if(Value)
		{
			*OutValue = *Value;
			return TRUE;
		}
		else if(Data.Parent)
		{
			return Data.Parent->GetRenderProxy(bSelected)->GetVectorValue(ParameterName, OutValue, Context);
		}
		else
		{
			return FALSE;
		}
	}

	virtual UBOOL GetScalarValue(const FName& ParameterName,FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		check(IsInRenderingThread());
		const FLOAT* Value = Data.ScalarParameterMap.Find(ParameterName);
		if(Value)
		{
			*OutValue = *Value;
			return TRUE;
		}
		else if(Data.Parent)
		{
			return Data.Parent->GetRenderProxy(bSelected)->GetScalarValue(ParameterName, OutValue, Context);
		}
		else
		{
			return FALSE;
		}
	}

	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue) const
	{
		check(IsInRenderingThread());
		const UTexture* Value = Data.TextureParameterMap.FindRef(ParameterName);
		if(Value)
		{
			*OutValue = Value->Resource;
			return TRUE;
		}
		else if(Data.Parent)
		{
			return Data.Parent->GetRenderProxy(bSelected)->GetTextureValue(ParameterName,OutValue);
		}
		else
		{
			return FALSE;
		}
	}

	// Accessors.
	void SetData(const DataType& InData)
	{
		Data = InData;
	}

	void SetGameThreadParent(UMaterialInterface* NewGameThreadParent)
	{
		if(GameThreadParent != NewGameThreadParent)
		{
			// Assign the new parent.
			GameThreadParent = NewGameThreadParent;
		}
	}

protected:

	/** The UMaterialInstance which owns this resource. */
	UMaterialInstance* Owner;

	/** Whether this resource represents the selected version of the material instance. */
	UBOOL bSelected;

	/** The rendering thread material instance properties. */
	DataType Data;

	/** The game thread accessible parent of the material instance. */
	UMaterialInterface* GameThreadParent;
};


class FMaterialInstanceConstantResource : public FMaterialInstanceResource
{
public:
	/** Initialization constructor. */
	FMaterialInstanceConstantResource(UMaterialInstance* InOwner,UBOOL bInSelected):
       FMaterialInstanceResource( InOwner, bInSelected )
	   {
	   }
};



class FMaterialInstanceTimeVaryingResource : public FMaterialInstanceResource
{
public:
	/** Initialization constructor. */
	FMaterialInstanceTimeVaryingResource(UMaterialInstance* InOwner,UBOOL bInSelected):
       FMaterialInstanceResource( InOwner, bInSelected )
	   {
	   }

	   /** 
 	    * For MITVs you can utilize both single Scalar values and InterpCurve values.
		*
		* If there is any data in the InterpCurve, then the MITV will utilize that. Else it will utilize the Scalar value
		* of the same name.
		**/
	virtual UBOOL GetScalarValue(const FName& ParameterName,FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		check(IsInRenderingThread());

		const FTimeVaryingDataType* Value = Data.ScalarOverTimeParameterMap.Find(ParameterName);

		//debugf( TEXT( "Looking for: %s  SizeOfMap: %d"), *ParameterName.ToString(), Data.ScalarOverTimeParameterMap.Num() );

		if(Value)
		{
			if( Value->TheCurve.Points.Num() > 0 )
			{
				const FLOAT StartTime = Value->StartTime;
				const FLOAT EvalTime = Context.CurrentTime - StartTime;
				const FLOAT EvalValue = Value->TheCurve.Eval(EvalTime, 0.0f);

				//debugf( TEXT( "  CurrentTime %f StarTime %f, EvalTime %f EvalValue %f" ), Context.CurrentTime, StartTime, EvalTime, EvalValue );

				*OutValue = EvalValue;
			}
			else
			{
				*OutValue = Value->ScalarValue;
			}
			
			return TRUE;
		}
		else if(Data.Parent)
		{
			return Data.Parent->GetRenderProxy(bSelected)->GetScalarValue(ParameterName, OutValue, Context);
		}
		else
		{
			return FALSE;
		}
	}
};



namespace
{
	/**
	* This function takes a array of parameter structs and attempts to establish a reference to the expression object each parameter represents.
	* If a reference exists, the function checks to see if the parameter has been renamed.
	*
	* @param Parameters		Array of parameters to operate on.
	* @param ParentMaterial	Parent material to search in for expressions.
	*
	* @return Returns whether or not any of the parameters was changed.
	*/
	template <class ParameterType, class ExpressionType>
	UBOOL UpdateParameterSet(TArray<ParameterType> &Parameters, UMaterial* ParentMaterial)
	{
		UBOOL bChanged = FALSE;

		// Loop through all of the parameters and try to either establish a reference to the 
		// expression the parameter represents, or check to see if the parameter's name has changed.
		for(INT ParameterIdx=0; ParameterIdx<Parameters.Num(); ParameterIdx++)
		{
			UBOOL bTryToFindByName = TRUE;

			ParameterType &Parameter = Parameters(ParameterIdx);

			if(Parameter.ExpressionGUID.IsValid())
			{
				ExpressionType* Expression = ParentMaterial->FindExpressionByGUID<ExpressionType>(Parameter.ExpressionGUID);

				// Check to see if the parameter name was changed.
				if(Expression)
				{
					bTryToFindByName = FALSE;

					if(Parameter.ParameterName != Expression->ParameterName)
					{
						Parameter.ParameterName = Expression->ParameterName;
						bChanged = TRUE;
					}
				}
			}

			// No reference to the material expression exists, so try to find one in the material expression's array if we are in the editor.
			if(bTryToFindByName && GIsEditor && !GIsGame)
			{
				for(INT ExpressionIndex = 0;ExpressionIndex < ParentMaterial->Expressions.Num();ExpressionIndex++)
				{
					ExpressionType* ParameterExpression = Cast<ExpressionType>(ParentMaterial->Expressions(ExpressionIndex));

					if(ParameterExpression && ParameterExpression->ParameterName == Parameter.ParameterName)
					{
						Parameter.ExpressionGUID = ParameterExpression->ExpressionGUID;
						bChanged = TRUE;
						break;
					}
				}
			}
		}

		return bChanged;
	}
}






#endif // _MATERIAL_INSTANCE_H_


