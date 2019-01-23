/**********************************************************************

Filename    :   GFxUIDataStores.cpp
Content     :   UGFxMoviePlayer class implementation for GFx

Copyright   :   Copyright 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxUI.h"

#include "EngineSequenceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "UnUIMarkupResolver.h"
#include "GFxUIUIPrivateClasses.h"
#include "GFxUIEngine.h"

#if WITH_GFx

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif
#include "GMemory.h"
#include "GFxPlayer.h"
#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

IMPLEMENT_CLASS(UGFxDataStoreSubscriber);


void UGFxMoviePlayer::RefreshDataStoreBindings()
{
    if (DataStoreBindings.Num() == 0)
        return;
    if (DataStoreSubscriber == NULL)
    {
        DataStoreSubscriber = ConstructObject<UGFxDataStoreSubscriber>(UGFxDataStoreSubscriber::StaticClass());
        DataStoreSubscriber->Movie = this;
    }

    for (INT BindingIndex = 0; BindingIndex < DataStoreBindings.Num(); BindingIndex++)
    {
        FGFxDataStoreBinding& Binding = DataStoreBindings(BindingIndex);
//        Binding.DataSource.MarkupString = Binding.Binding;

        delete Binding.ModelRef;
        Binding.ModelRef = NULL;
        delete Binding.ControlRef;
        Binding.ControlRef = NULL;
        Binding.DataSource.UnregisterSubscriberCallback();
        Binding.DataSource.Subscriber = DataStoreSubscriber;

        FUIStringParser Parser;
        Parser.ScanString(Binding.DataSource.MarkupString);

        const TIndirectArray<FTextChunk>* Chunks = Parser.GetTextChunks();
        if ( Chunks->Num() == 1 )
        {
            const FTextChunk* NextChunk = &((*Chunks)(0));
            if ( NextChunk->IsMarkup() )
            {
                FString DataStoreTag, DataStoreValue;
                if ( NextChunk->GetDataStoreMarkup(DataStoreTag, DataStoreValue) )
                {
                    UDataStoreClient* DSClient = UUIInteraction::GetDataStoreClient();
                    if ( DSClient != NULL )
                    {
                        Binding.DataSource.ResolvedDataStore = DSClient->FindDataStore(FName(*DataStoreTag), eventGetLP());

                        if ( Binding.DataSource.ResolvedDataStore != NULL )
                        {
                            // save the resolved tags for later use
                            Binding.DataSource.DataStoreName = *DataStoreTag;
                            Binding.DataSource.DataStoreField = *DataStoreValue;

                            Binding.ListDataProvider
                                = Binding.DataSource.ResolvedDataStore->ResolveListElementProvider(DataStoreValue);

                            if (Binding.ListDataProvider && Binding.CellTags.Num() == 0)
                            {
                                TScriptInterface<IUIListElementCellProvider> CellSchemaProvider
                                    = Binding.ListDataProvider->GetElementCellSchemaProvider(Binding.DataSource.DataStoreField);

                                if (CellSchemaProvider)
                                {
                                    TMap<FName,FString> CellTags;
                                    CellSchemaProvider->GetElementCellTags(Binding.DataSource.DataStoreField, CellTags);
                                    Binding.CellTags.Empty();
                                    CellTags.GenerateKeyArray(Binding.CellTags);
                                }
                            }

                            Binding.FullCellTags.Empty();
                            for (int i = 0; i < Binding.CellTags.Num(); i++)
                                Binding.FullCellTags.AddItem(FName(*(DataStoreValue + TEXT(".") + Binding.CellTags(i).ToString())));

                            // and register this subscriber with the newly resolved data store
                            Binding.DataSource.RegisterSubscriberCallback();

                            DataStoreSubscriber->RefreshSubscriberValue(BindingIndex);

                            FTCHARToUTF8 mid (*Binding.ModelId);
                            Binding.ModelIdUtf8.Empty();
                            Binding.ModelIdUtf8.Add(mid.Length()+1);
                            memcpy((char*)&Binding.ModelIdUtf8(0), mid, mid.Length()+1);

                            FTCHARToUTF8 cid (*Binding.ControlId);
                            Binding.ControlIdUtf8.Empty();
                            Binding.ControlIdUtf8.Add(cid.Length()+1);
                            memcpy((char*)&Binding.ControlIdUtf8(0), cid, cid.Length()+1);
                        }
                    }
                }
            }
        }
    }
}

void UGFxMoviePlayer::PublishDataStoreValues()
{
    if (DataStoreSubscriber)
        DataStoreSubscriber->PublishValues();
}

void UGFxMoviePlayer::SetPriority(BYTE NewPriority)
{
	Priority = NewPriority;
	if (pMovie)
	{
		FGFxEngine* Engine = FGFxEngine::GetEngine();
		Engine->InsertMovie(pMovie,SDPG_Foreground);
	}
}

static void ConvertToString(const GFxValue& val, FString& Str)
{
    if (val.GetType() == GFxValue::VT_String)
        Str = val.GetString();
    else if (val.GetType() == GFxValue::VT_StringW)
        Str = val.GetStringW();
    else if (val.GetType() == GFxValue::VT_Boolean)
        Str = val.GetBool() ? TEXT("1") : TEXT("0");
    else if (val.GetType() == GFxValue::VT_Number)
        Str = FString::Printf( TEXT("%lf"), val.GetNumber());
    else if (val.GetType() == GFxValue::VT_Undefined)
        Str = TEXT("undefined");
    else
        Str = FString();
}

// GameDataProvider arguments [callId, Model or View Id, ...]

void UGFxMoviePlayer::ProcessDataStoreCall(const char* methodName, const GFxValue* args, int argCount)
{
    INT BindingIndex = -1;
    for (INT i = 0; i < DataStoreBindings.Num(); i++)
        if (DataStoreBindings(i).ModelIdUtf8.Num())
            if (!strcmp((char*)&DataStoreBindings(i).ModelIdUtf8(0), args[1].GetString()) ||
                !strcmp((char*)&DataStoreBindings(i).ControlIdUtf8(0), args[1].GetString()))
                BindingIndex = i;

    if (BindingIndex < 0)
        return;

    FGFxDataStoreBinding& Binding = DataStoreBindings(BindingIndex);

    if (!strcmp(methodName, "__requestItemRange") && Binding.ListDataProvider && argCount >= 4)
    {
        SInt Start = (SInt)args[2].GetNumber();
        SInt End  = (SInt)args[3].GetNumber();

        GFxValue Response[2];
        Response[0] = args[0];

        FUIDataStoreBinding& DataSource = Binding.DataSource;
        FUIProviderFieldValue ResolvedValue(EC_EventParm);

        DataSource.GetBindingValue(ResolvedValue); // selected items - currently unused

        {
            TArray<INT> NewIndices;
            if (Binding.ListDataProvider->GetListElements(DataSource.DataStoreField, NewIndices))
            {
                pMovie->pView->CreateArray(&Response[1]);

                for (int Index = Start; Index <= End && Index < NewIndices.Num(); Index++)
                {
                    TScriptInterface<IUIListElementCellProvider> CellDataProvider
                        = Binding.ListDataProvider->GetElementCellValueProvider(Binding.DataSource.DataStoreField, NewIndices(Index));
                    GFxValue Row;
                    pMovie->pView->CreateObject(&Row);

                    for (int Cell = 0; Cell < Binding.CellTags.Num(); Cell++)
                    {
                        FUIProviderFieldValue CellValue(EC_EventParm);
                        GFxValue Value;
                        if (CellDataProvider->GetCellFieldValue(DataSource.DataStoreField, Binding.CellTags(Cell),
                            /*DataSource.DataSourceIndex*/BindingIndex, CellValue, NewIndices(Index)))
                        {
                            switch (CellValue.PropertyType)
                            {
                            case DATATYPE_Property: Value.SetStringW(*CellValue.StringValue); break;
                            }
                        }
                        Row.SetMember(FTCHARToUTF8(*Binding.CellTags(Cell).ToString()), Value);
                    }

                    Response[1].PushBack(Row);
                }
            }
        }

        pMovie->pView->Invoke("respond", NULL, Response, 2);
    }
    else if (!strcmp(methodName, "__requestItemAt"))
    {
    }
    else if (!strcmp(methodName, "__registerModel") && argCount >= 3 && args[2].IsObject() && Binding.ListDataProvider)
    {
        delete Binding.ModelRef;
        Binding.ModelRef = new GFxValue(args[2]);

        INT Count = Binding.ListDataProvider->GetElementCount(Binding.DataSource.DataStoreField);

        GFxValue length ((Double)Count);
        Binding.ModelRef->Invoke("invalidate", NULL, &length, 1);
    }
    else if (!strcmp(methodName, "__registerControl") && argCount >= 4 && args[2].IsObject())
    {
        delete Binding.ControlRef;
        Binding.ControlRef = new GFxValue(args[2]);

        if (Binding.DataSource.ResolvedDataStore && !Binding.ListDataProvider)
        {
            FUIProviderFieldValue Value(EC_EventParm);
            Value.PropertyTag = Binding.DataSource.DataStoreField;
            Value.PropertyType = DATATYPE_Property;
            if (Binding.DataSource.GetBindingValue(Value))
            {
                if (!strcmp(args[3].GetString(), "Slider"))
                {
                    GFxValue gval (*Value.StringValue);
                    Binding.ControlRef->SetMember("value", gval);
                }
                else if (!strcmp(args[3].GetString(), "TextInput"))
                {
                    GFxValue gval (*Value.StringValue);
                    Binding.ControlRef->SetMember("text", gval);
                }
            }
        }
    }
    else if (!strcmp(methodName, "__handleEvent") && Binding.ControlRef && argCount >= 2 && args[2].IsObject())
    {
        GFxValue gval;
        args[2].GetMember("type", &gval);
        if (gval.IsString())
        {
            FUIProviderFieldValue Value(EC_EventParm);
            Value.PropertyTag = Binding.DataSource.DataStoreField;
            Value.PropertyType = DATATYPE_Property;

            if (!strcmp (gval.GetString(), "change"))
            {
                Binding.ControlRef->GetMember("value", &gval);
                ConvertToString(gval, Value.StringValue);
                Binding.DataSource.SetBindingValue(Value);
            }
            else if (!strcmp (gval.GetString(), "textChange"))
            {
                Binding.ControlRef->GetMember("text", &gval);
                ConvertToString(gval, Value.StringValue);
                Binding.DataSource.SetBindingValue(Value);
            }
        }
    }
}

void UGFxDataStoreSubscriber::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
    check(0);
}

FString UGFxDataStoreSubscriber::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
    return Movie->DataStoreBindings(BindingIndex).DataSource.MarkupString;
}

UBOOL UGFxDataStoreSubscriber::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
    FGFxDataStoreBinding& Binding = Movie->DataStoreBindings(BindingIndex);

    if ( Binding.DataSource )
    {
        FUIProviderFieldValue ResolvedValue(EC_EventParm);

        if (Binding.DataSource.GetBindingValue(ResolvedValue) || Binding.ListDataProvider)
        {
            Binding.DataSource.RequiredFieldType = ResolvedValue.PropertyType;

            if (Binding.ListDataProvider && Binding.ModelRef)
            {
                INT Count = Binding.ListDataProvider->GetElementCount(Binding.DataSource.DataStoreField);

                GFxValue length ((Double)Count);
                Binding.ModelRef->Invoke("invalidate", NULL, &length, 1);
            }
            else if (Binding.ControlRef)
            {
                GFxValue val (*ResolvedValue.StringValue);
                Binding.ControlRef->SetMember("value", val);
            }
            else if (Binding.VarPath.Len())
                Movie->SetVariableString(Binding.VarPath, ResolvedValue.StringValue);
        }
        else if (Binding.ControlRef)
        {
            GFxValue val (*Binding.DataSource.MarkupString);
            Binding.ControlRef->SetMember("value", val);
        }
        else if (Binding.VarPath.Len())
            Movie->SetVariableString(Binding.VarPath, Binding.DataSource.MarkupString);

        return TRUE;
    }
    return FALSE;
}

void UGFxDataStoreSubscriber::NotifyDataStoreValueUpdated( UUIDataStore* SourceDataStore, UBOOL bValuesInvalidated, FName PropertyTag, UUIDataProvider* SourceProvider, INT ArrayIndex )
{
    for (INT BindingIndex = 0; BindingIndex < Movie->DataStoreBindings.Num(); BindingIndex++)
    {
        FGFxDataStoreBinding& Binding = Movie->DataStoreBindings(BindingIndex);

        if (SourceDataStore == Binding.DataSource.ResolvedDataStore)
        {
            if (PropertyTag == NAME_None || PropertyTag == Binding.DataSource.DataStoreField
                && (bValuesInvalidated || !Binding.ListDataProvider))
                RefreshSubscriberValue(BindingIndex);
            else if (Binding.ListDataProvider && Binding.ModelRef)
            {
                FString props = *PropertyTag.ToString();
                const TCHAR *propn = *props;

                for (int Cell = 0; Cell < Binding.CellTags.Num(); Cell++)
                    if (Binding.DataSource.DataStoreField.ToString() + TEXT(".") + Binding.CellTags(Cell).ToString() == PropertyTag.ToString())
                    {
                        //Binding.ModelRef->Invoke("invalidateRange", NULL, &length, 1);

                        RefreshSubscriberValue(BindingIndex);
                        break;
                    }
            }
        }
    }
}

void UGFxDataStoreSubscriber::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
    for (INT BindingIndex = 0; BindingIndex < Movie->DataStoreBindings.Num(); BindingIndex++)
    {
        if (Movie->DataStoreBindings(BindingIndex).DataSource)
            out_BoundDataStores.AddUniqueItem(*Movie->DataStoreBindings(BindingIndex).DataSource);
    }
}

void UGFxDataStoreSubscriber::ClearBoundDataStores()
{
    for (INT BindingIndex = 0; BindingIndex < Movie->DataStoreBindings.Num(); BindingIndex++)
    {
        FGFxDataStoreBinding& Binding = Movie->DataStoreBindings(BindingIndex);
        Binding.DataSource.ClearDataBinding();
        delete Binding.ModelRef;
        Binding.ModelRef = NULL;
        delete Binding.ControlRef;
        Binding.ControlRef = NULL;
    }

    TArray<UUIDataStore*> DataStores;
    GetBoundDataStores(DataStores);

    for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
    {
        UUIDataStore* DataStore = DataStores(DataStoreIndex);
        DataStore->eventSubscriberDetached(this);
    }
}

UBOOL UGFxDataStoreSubscriber::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
    FGFxDataStoreBinding& Binding = Movie->DataStoreBindings(BindingIndex);

    if (Binding.bEditable && Binding.DataSource)
    {
        out_BoundDataStores.AddUniqueItem(*Binding.DataSource);

        FUIProviderScriptFieldValue Value(EC_EventParm);
        Value.PropertyTag = Binding.DataSource.DataStoreField;
/*
        if (Binding.DataSource.RequiredFieldType == DATATYPE_Collection)
        {
            Value.PropertyType = DATATYPE_Collection;
            Movie->GetVariableIntArray(Binding.VarPath, 0, Value.ArrayValue);
        }
        */
        if (Binding.ControlRef)
        {
            Value.PropertyType = DATATYPE_Property;
            GFxValue val;
            Binding.ControlRef->GetMember("value", &val);
            Value.StringValue = val.GetStringW();
        }
        else
        {
            Value.PropertyType = DATATYPE_Property;
            Value.StringValue = Movie->GetVariableString(Binding.VarPath);
        }

        return Binding.DataSource.SetBindingValue(Value);
    }

    return FALSE;
}

void UGFxDataStoreSubscriber::PublishValues()
{
    TArray<UUIDataStore*> DataStores;

    for (INT BindingIndex = 0; BindingIndex < Movie->DataStoreBindings.Num(); BindingIndex++)
    {
        SaveSubscriberValue(DataStores, BindingIndex);
    }

    for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
    {
        UUIDataStore* DataStore = DataStores(DataStoreIndex);
        DataStore->OnCommit();
    }
}

#else // WITH_GFx = 0

IMPLEMENT_CLASS(UGFxDataStoreSubscriber);

void UGFxMoviePlayer::RefreshDataStoreBindings()
{
}

void UGFxMoviePlayer::PublishDataStoreValues()
{
}

void UGFxMoviePlayer::ProcessDataStoreCall(const char* methodName, const GFxValue* args, int argCount)
{
}

void UGFxDataStoreSubscriber::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	check(0);
}

FString UGFxDataStoreSubscriber::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	return FString();
}

UBOOL UGFxDataStoreSubscriber::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	return FALSE;
}

void UGFxDataStoreSubscriber::NotifyDataStoreValueUpdated( UUIDataStore* SourceDataStore, UBOOL bValuesInvalidated, FName PropertyTag, UUIDataProvider* SourceProvider, INT ArrayIndex )
{
}

void UGFxDataStoreSubscriber::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{

}

void UGFxDataStoreSubscriber::ClearBoundDataStores()
{

}

UBOOL UGFxDataStoreSubscriber::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	return FALSE;
}

void UGFxDataStoreSubscriber::PublishValues()
{

}

void UGFxMoviePlayer::SetPriority(BYTE NewPriority)
{
}

#endif // WITH_GFx