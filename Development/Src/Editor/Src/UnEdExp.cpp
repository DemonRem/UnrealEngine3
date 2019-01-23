/*=============================================================================
	UnEdExp.cpp: Editor exporters.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "UnTerrain.h"
#include "EngineSequenceClasses.h"
#include "EngineSoundNodeClasses.h"

/*------------------------------------------------------------------------------
	UTextBufferExporterTXT implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UTextBufferExporterTXT::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UTextBuffer::StaticClass();
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("TXT")) );
	new(FormatDescription)FString(TEXT("Text file"));
	bText = 1;
}
UBOOL UTextBufferExporterTXT::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags)
{
	UTextBuffer* TextBuffer = CastChecked<UTextBuffer>( Object );
	FString Str( TextBuffer->Text );

	TCHAR* Start = const_cast<TCHAR*>(*Str);
	TCHAR* End   = Start + Str.Len();
	while( Start<End && (Start[0]=='\r' || Start[0]=='\n' || Start[0]==' ') )
		Start++;
	while( End>Start && (End [-1]=='\r' || End [-1]=='\n' || End [-1]==' ') )
		End--;
	*End = 0;

	Ar.Log( Start );

	return 1;
}
IMPLEMENT_CLASS(UTextBufferExporterTXT);

/*------------------------------------------------------------------------------
	USoundExporterWAV implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
#if 1
void USoundExporterWAV::InitializeIntrinsicPropertyValues()
{
	SupportedClass = USoundNodeWave::StaticClass();
	bText = 0;
	new( FormatExtension ) FString( TEXT( "WAV" ) );
	new( FormatDescription ) FString( TEXT( "Sound" ) );
}
UBOOL USoundExporterWAV::ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, INT FileIndex, DWORD PortFlags )
{
	USoundNodeWave* Sound = CastChecked<USoundNodeWave>( Object );
	void* RawWaveData = Sound->RawData.Lock( LOCK_READ_ONLY );
	Ar.Serialize( RawWaveData, Sound->RawData.GetBulkDataSize() );
	Sound->RawData.Unlock();
	return TRUE;
}
IMPLEMENT_CLASS(USoundExporterWAV);

#else
/*------------------------------------------------------------------------------
	USoundExporterOGG implementation.
------------------------------------------------------------------------------*/

/**
* Initializes property values for intrinsic classes.  It is called immediately after the class default object
* is initialized against its archetype, but before any objects of this class are created.
*/
void USoundExporterOGG::InitializeIntrinsicPropertyValues()
{
	SupportedClass = USoundNodeWave::StaticClass();
	bText = 0;
	new( FormatExtension ) FString( TEXT( "OGG" ) );
	new( FormatDescription ) FString( TEXT( "Sound" ) );
}
UBOOL USoundExporterOGG::ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, INT FileIndex, DWORD PortFlags )
{
	USoundNodeWave* Sound = CastChecked<USoundNodeWave>( Object );
	void* RawOggData = Sound->CompressedPCData.Lock( LOCK_READ_ONLY );
	Ar.Serialize( RawOggData, Sound->CompressedPCData.GetBulkDataSize() );
	Sound->CompressedPCData.Unlock();
	return TRUE;
}
IMPLEMENT_CLASS(USoundExporterOGG);
#endif

/*------------------------------------------------------------------------------
	USoundSurroundExporterWAV implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void USoundSurroundExporterWAV::InitializeIntrinsicPropertyValues()
{
	SupportedClass = USoundNodeWave::StaticClass();
	bText = 0;
	new( FormatExtension ) FString( TEXT( "WAV" ) );
	new( FormatDescription ) FString( TEXT( "Multichannel Sound" ) );
}

INT USoundSurroundExporterWAV::GetFileCount( void ) const
{
	return( SPEAKER_Count );
}

FString USoundSurroundExporterWAV::GetUniqueFilename( const TCHAR* Filename, INT FileIndex )
{
	static FString SpeakerLocations[SPEAKER_Count] =
	{
		TEXT( "_fl" ),			// SPEAKER_FrontLeft
		TEXT( "_fr" ),			// SPEAKER_FrontRight
		TEXT( "_fc" ),			// SPEAKER_FrontCenter
		TEXT( "_lf" ),			// SPEAKER_LowFrequency
		TEXT( "_sl" ),			// SPEAKER_SideLeft
		TEXT( "_sr" ),			// SPEAKER_SideRight
		TEXT( "_bl" ),			// SPEAKER_BackLeft
		TEXT( "_br" )			// SPEAKER_BackRight
	};

	FFilename WorkName = Filename;
	FString ReturnName = WorkName.GetBaseFilename( FALSE ) + SpeakerLocations[FileIndex] + FString( ".WAV" );

	return( ReturnName );
}

UBOOL USoundSurroundExporterWAV::ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, INT FileIndex, DWORD PortFlags )
{
	USoundNodeWave* Sound = CastChecked<USoundNodeWave>( Object );
	BYTE* RawWaveData = ( BYTE * )Sound->RawData.Lock( LOCK_READ_ONLY );

	if( Sound->ChannelSizes( FileIndex ) )
	{
		Ar.Serialize( RawWaveData + Sound->ChannelOffsets( FileIndex ), Sound->ChannelSizes( FileIndex ) );
	}

	Sound->RawData.Unlock();

	return( Sound->ChannelSizes( FileIndex ) != 0 );
}
IMPLEMENT_CLASS(USoundSurroundExporterWAV);

/*------------------------------------------------------------------------------
	UClassExporterUC implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UClassExporterUC::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UClass::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("UC")) );
	new(FormatDescription)FString(TEXT("UnrealScript"));
}
UBOOL UClassExporterUC::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags)
{
	UClass* Class = CastChecked<UClass>( Object );

	// if the class is instrinsic, it won't have script text
	if ( (Class->ClassFlags&CLASS_Intrinsic) != 0 )
		return FALSE;

	// Export script text.
	check(Class->GetDefaultsCount());
	check(Class->ScriptText);
	UExporter::ExportToOutputDevice( Context, Class->ScriptText, NULL, Ar, TEXT("txt"), TextIndent, PortFlags );

	// Export cpptext.
	if( Class->CppText )
	{
		Ar.Log( TEXT("\r\n\r\ncpptext\r\n{\r\n") );
		Ar.Log( *Class->CppText->Text );
		Ar.Log( TEXT("\r\n}\r\n") );
	}

	// Export default properties that differ from parent's.
	Ar.Log( TEXT("\r\n\r\ndefaultproperties\r\n{\r\n") );
	ExportProperties
	(
		Context,
		Ar,
		Class,
		Class->GetDefaults(),
		TextIndent+3,
		Class->GetSuperClass(),
		Class->GetSuperClass() ? Class->GetSuperClass()->GetDefaults() : NULL,
		Class
	);
	Ar.Log( TEXT("}\r\n") );

	return 1;
}
IMPLEMENT_CLASS(UClassExporterUC);

/*------------------------------------------------------------------------------
	UObjectExporterT3D implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UObjectExporterT3D::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UObject::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("T3D")) );
	new(FormatDescription)FString(TEXT("Unreal object text"));
	new(FormatExtension)FString(TEXT("COPY"));
	new(FormatDescription)FString(TEXT("Unreal object text"));
}
UBOOL UObjectExporterT3D::ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags )
{
	Ar.Logf( TEXT("%sBegin Object Class=%s Name=%s ObjName=%s Archetype=%s'%s'") LINE_TERMINATOR,
		appSpc(TextIndent), *Object->GetClass()->GetName(), *Object->GetName(), *Object->GetName(),
		*Object->GetArchetype()->GetClass()->GetName(), *Object->GetArchetype()->GetPathName() );
		ExportObjectInner( Context, Object, Ar, PortFlags);
	Ar.Logf( TEXT("%sEnd Object") LINE_TERMINATOR, appSpc(TextIndent) );

	return 1;
}
IMPLEMENT_CLASS(UObjectExporterT3D);

/*------------------------------------------------------------------------------
	UPolysExporterT3D implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UPolysExporterT3D::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UPolys::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("T3D")) );
	new(FormatDescription)FString(TEXT("Unreal poly text"));
}
UBOOL UPolysExporterT3D::ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags )
{
	UPolys* Polys = CastChecked<UPolys>( Object );

	FLightingChannelContainer		DefaultLightingChannels;
	DefaultLightingChannels.Bitfield		= 0;
	DefaultLightingChannels.BSP				= TRUE;
	DefaultLightingChannels.bInitialized	= TRUE;

	Ar.Logf( TEXT("%sBegin PolyList\r\n"), appSpc(TextIndent) );
	for( INT i=0; i<Polys->Element.Num(); i++ )
	{
		FPoly* Poly = &Polys->Element(i);
		TCHAR TempStr[MAX_SPRINTF]=TEXT("");

		// Start of polygon plus group/item name if applicable.
		// The default values need to jive FPoly::Init().
		Ar.Logf( TEXT("%s   Begin Polygon"), appSpc(TextIndent) );
		if( Poly->ItemName != NAME_None )
			Ar.Logf( TEXT(" Item=%s"), *Poly->ItemName.ToString() );
		if( Poly->Material )
			Ar.Logf( TEXT(" Texture=%s"), *Poly->Material->GetPathName() );
		if( Poly->PolyFlags != 0 )
			Ar.Logf( TEXT(" Flags=%i"), Poly->PolyFlags );
		if( Poly->iLink != INDEX_NONE )
			Ar.Logf( TEXT(" Link=%i"), Poly->iLink );
		if ( Poly->ShadowMapScale != 32.0f )
			Ar.Logf( TEXT(" ShadowMapScale=%f"), Poly->ShadowMapScale );
		if ( Poly->LightingChannels != DefaultLightingChannels.Bitfield )
			Ar.Logf( TEXT(" LightingChannels=%i"), Poly->LightingChannels );
		Ar.Logf( TEXT("\r\n") );

		// All coordinates.
		Ar.Logf( TEXT("%s      Origin   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Base) );
		Ar.Logf( TEXT("%s      Normal   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Normal) );
		Ar.Logf( TEXT("%s      TextureU %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->TextureU) );
		Ar.Logf( TEXT("%s      TextureV %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->TextureV) );
		for( INT j=0; j<Poly->Vertices.Num(); j++ )
			Ar.Logf( TEXT("%s      Vertex   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Vertices(j)) );
		Ar.Logf( TEXT("%s   End Polygon\r\n"), appSpc(TextIndent) );
	}
	Ar.Logf( TEXT("%sEnd PolyList\r\n"), appSpc(TextIndent) );

	return 1;
}
IMPLEMENT_CLASS(UPolysExporterT3D);

/*------------------------------------------------------------------------------
	UModelExporterT3D implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UModelExporterT3D::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UModel::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("T3D")) );
	new(FormatDescription)FString(TEXT("Unreal model text"));
}
UBOOL UModelExporterT3D::ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags )
{
	UModel* Model = CastChecked<UModel>( Object );

	Ar.Logf( TEXT("%sBegin Brush Name=%s\r\n"), appSpc(TextIndent), *Model->GetName() );
	UExporter::ExportToOutputDevice( Context, Model->Polys, NULL, Ar, Type, TextIndent+3, PortFlags );
	Ar.Logf( TEXT("%sEnd Brush\r\n"), appSpc(TextIndent) );

	return 1;
}
IMPLEMENT_CLASS(UModelExporterT3D);

/*------------------------------------------------------------------------------
	ULevelExporterT3D implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void ULevelExporterT3D::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UWorld::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("T3D")) );
	new(FormatDescription)FString(TEXT("Unreal world text"));
	new(FormatExtension)FString(TEXT("COPY"));
	new(FormatDescription)FString(TEXT("Unreal world text"));
}
UBOOL ULevelExporterT3D::ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags )
{
	UWorld* World = CastChecked<UWorld>( Object );
	APhysicsVolume* DefaultPhysicsVolume = World->GetDefaultPhysicsVolume();

	for( FObjectIterator It; It; ++It )
		It->ClearFlags( RF_TagImp | RF_TagExp );

	// this is the top level in the .t3d file
	Ar.Logf( TEXT("%sBegin Map\r\n"), appSpc(TextIndent) );

	// are we exporting all actors or just selected actors?
	UBOOL bAllActors = appStricmp(Type,TEXT("COPY"))!=0 && !bSelectedOnly;

	TextIndent += 3;

	ULevel* Level;

	// start a new level section
	if (appStricmp(Type, TEXT("COPY")) == 0)
	{
		// for copy and paste, we want to select actors in the current level
		Level = World->CurrentLevel;

		// if we are copy/pasting, then we don't name the level - we paste into the current level
		Ar.Logf(TEXT("%sBegin Level\r\n"), appSpc(TextIndent));

		// mark that we are doing a clipboard copy
		PortFlags |= PPF_Copy;
	}
	else
	{
		// for export, we only want the persistent level
		Level = World->PersistentLevel;

		//@todo seamless if we are exporting only selected, should we export from all levels? or maybe from the current level?

		// if we aren't copy/pasting, then we name the level so that when we import, we get the same level structure
		Ar.Logf(TEXT("%sBegin Level NAME=%s\r\n"), appSpc(TextIndent), *Level->GetName());
	}

	TextIndent += 3;

	// loop through all of the actors just in this level
	for( INT iActor=0; iActor<Level->Actors.Num(); iActor++ )
	{
		AActor* Actor = Level->Actors(iActor);
		// Don't export the default physics volume, as it doesn't have a UModel associated with it
		// and thus will not import properly.
		if ( Actor == DefaultPhysicsVolume )
		{
			continue;
		}
		ATerrain* pkTerrain = Cast<ATerrain>(Actor);
		if (pkTerrain && (bAllActors || pkTerrain->IsSelected()))
		{
			// Terrain exporter...
			// Find the UTerrainExporterT3D exporter?
			UTerrainExporterT3D* pkTerrainExp = ConstructObject<UTerrainExporterT3D>(UTerrainExporterT3D::StaticClass());
			if (pkTerrainExp)
			{
				pkTerrainExp->TextIndent = TextIndent;
				UBOOL bResult = pkTerrainExp->ExportText( Context, pkTerrain, Type, Ar, Warn );
			}
		}
		else
		if( Actor && ( bAllActors || Actor->IsSelected() ) )
		{
			Ar.Logf( TEXT("%sBegin Actor Class=%s Name=%s Archetype=%s'%s'") LINE_TERMINATOR, 
				appSpc(TextIndent), *Actor->GetClass()->GetName(), *Actor->GetName(),
				*Actor->GetArchetype()->GetClass()->GetName(), *Actor->GetArchetype()->GetPathName() );

			ExportObjectInner( Context, Actor, Ar, PortFlags | PPF_ExportsNotFullyQualified );

			Ar.Logf( TEXT("%sEnd Actor\r\n"), appSpc(TextIndent) );
		}
	}

	TextIndent -= 3;

	Ar.Logf(TEXT("%sEnd Level\r\n"), appSpc(TextIndent));

	TextIndent -= 3;

	// Export information about the first selected surface in the map.  Used for copying/pasting
	// information from poly to poly.
	Ar.Logf( TEXT("%sBegin Surface\r\n"), appSpc(TextIndent) );
	TCHAR TempStr[256];
	for( INT i=0; i<GWorld->GetModel()->Surfs.Num(); i++ )
	{
		FBspSurf *Poly = &GWorld->GetModel()->Surfs(i);
		if( Poly->PolyFlags&PF_Selected )
		{
			Ar.Logf( TEXT("%sTEXTURE=%s\r\n"), appSpc(TextIndent+3), *Poly->Material->GetPathName() );
			Ar.Logf( TEXT("%sBASE      %s\r\n"), appSpc(TextIndent+3), SetFVECTOR(TempStr,&(GWorld->GetModel()->Points(Poly->pBase))) );
			Ar.Logf( TEXT("%sTEXTUREU  %s\r\n"), appSpc(TextIndent+3), SetFVECTOR(TempStr,&(GWorld->GetModel()->Vectors(Poly->vTextureU))) );
			Ar.Logf( TEXT("%sTEXTUREV  %s\r\n"), appSpc(TextIndent+3), SetFVECTOR(TempStr,&(GWorld->GetModel()->Vectors(Poly->vTextureV))) );
			Ar.Logf( TEXT("%sNORMAL    %s\r\n"), appSpc(TextIndent+3), SetFVECTOR(TempStr,&(GWorld->GetModel()->Vectors(Poly->vNormal))) );
			Ar.Logf( TEXT("%sPOLYFLAGS=%d\r\n"), appSpc(TextIndent+3), Poly->PolyFlags );
			break;
		}
	}
	Ar.Logf( TEXT("%sEnd Surface\r\n"), appSpc(TextIndent) );

	Ar.Logf( TEXT("%sEnd Map\r\n"), appSpc(TextIndent) );


	return 1;
}
IMPLEMENT_CLASS(ULevelExporterT3D);

/*------------------------------------------------------------------------------
	ULevelExporterSTL implementation.
------------------------------------------------------------------------------*/

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void ULevelExporterSTL::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UWorld::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("STL")) );
	new(FormatDescription)FString(TEXT("Stereolithography"));
}
UBOOL ULevelExporterSTL::ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags )
{
	//@todo seamless - this needs to be world, like the t3d version above
	UWorld* World = CastChecked<UWorld>(Object);
	ULevel* Level = World->PersistentLevel;

	for( FObjectIterator It; It; ++It )
		It->ClearFlags( RF_TagImp | RF_TagExp );

	//
	// GATHER TRIANGLES
	//

	TArray<FVector> Triangles;

	// Specific actors can be exported
	for( INT iActor=0; iActor<Level->Actors.Num(); iActor++ )
	{
		ATerrain* T = Cast<ATerrain>(Level->Actors(iActor));
		if( T && ( !bSelectedOnly || T->IsSelected() ) )
		{
			for( int y = 0 ; y < T->NumVerticesY-1 ; y++ )
			{
				for( int x = 0 ; x < T->NumVerticesX-1 ; x++ )
				{
					FVector P1	= T->GetWorldVertex(x,y);
					FVector P2	= T->GetWorldVertex(x,y+1);
					FVector P3	= T->GetWorldVertex(x+1,y+1);
					FVector P4	= T->GetWorldVertex(x+1,y);

					Triangles.AddItem( P4 );
					Triangles.AddItem( P1 );
					Triangles.AddItem( P2 );

					Triangles.AddItem( P3 );
					Triangles.AddItem( P2 );
					Triangles.AddItem( P4 );
				}
			}
		}

		AStaticMeshActor* Actor = Cast<AStaticMeshActor>(Level->Actors(iActor));
		if( Actor && ( !bSelectedOnly || Actor->IsSelected() ) && Actor->StaticMeshComponent->StaticMesh )
		{
			const FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) Actor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.Lock(LOCK_READ_ONLY);
			for( INT tri = 0 ; tri < Actor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.GetElementCount() ; tri++ )
			{
				for( INT v = 2 ; v > -1 ; v-- )
				{
					FVector vtx = Actor->LocalToWorld().TransformFVector( RawTriangleData[tri].Vertices[v] );
					Triangles.AddItem( vtx );
				}
			}
			Actor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.Unlock();
		}
	}

	// Selected BSP surfaces
	for( INT i=0;i<GWorld->GetModel()->Nodes.Num();i++ )
	{
		FBspNode* Node = &GWorld->GetModel()->Nodes(i);
		if( !bSelectedOnly || GWorld->GetModel()->Surfs(Node->iSurf).PolyFlags&PF_Selected )
		{
			if( Node->NumVertices > 2 )
			{
				FVector vtx1(GWorld->GetModel()->Points(GWorld->GetModel()->Verts(Node->iVertPool+0).pVertex)),
					vtx2(GWorld->GetModel()->Points(GWorld->GetModel()->Verts(Node->iVertPool+1).pVertex)),
					vtx3;

				for( INT v = 2 ; v < Node->NumVertices ; v++ )
				{
					vtx3 = GWorld->GetModel()->Points(GWorld->GetModel()->Verts(Node->iVertPool+v).pVertex);

					Triangles.AddItem( vtx1 );
					Triangles.AddItem( vtx2 );
					Triangles.AddItem( vtx3 );

					vtx2 = vtx3;
				}
			}
		}
	}

	//
	// WRITE THE FILE
	//

	Ar.Logf( TEXT("%ssolid LevelBSP\r\n"), appSpc(TextIndent) );

	for( INT tri = 0 ; tri < Triangles.Num() ; tri += 3 )
	{
		FVector vtx[3];
		vtx[0] = Triangles(tri) * FVector(1,-1,1);
		vtx[1] = Triangles(tri+1) * FVector(1,-1,1);
		vtx[2] = Triangles(tri+2) * FVector(1,-1,1);

		FPlane Normal( vtx[0], vtx[1], vtx[2] );

		Ar.Logf( TEXT("%sfacet normal %1.6f %1.6f %1.6f\r\n"), appSpc(TextIndent+2), Normal.X, Normal.Y, Normal.Z );
		Ar.Logf( TEXT("%souter loop\r\n"), appSpc(TextIndent+4) );

		Ar.Logf( TEXT("%svertex %1.6f %1.6f %1.6f\r\n"), appSpc(TextIndent+6), vtx[0].X, vtx[0].Y, vtx[0].Z );
		Ar.Logf( TEXT("%svertex %1.6f %1.6f %1.6f\r\n"), appSpc(TextIndent+6), vtx[1].X, vtx[1].Y, vtx[1].Z );
		Ar.Logf( TEXT("%svertex %1.6f %1.6f %1.6f\r\n"), appSpc(TextIndent+6), vtx[2].X, vtx[2].Y, vtx[2].Z );

		Ar.Logf( TEXT("%sendloop\r\n"), appSpc(TextIndent+4) );
		Ar.Logf( TEXT("%sendfacet\r\n"), appSpc(TextIndent+2) );
	}

	Ar.Logf( TEXT("%sendsolid LevelBSP\r\n"), appSpc(TextIndent) );

	Triangles.Empty();

	return 1;
}
IMPLEMENT_CLASS(ULevelExporterSTL);

/*------------------------------------------------------------------------------
	ULevelExporterOBJ implementation.
------------------------------------------------------------------------------*/
/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void ULevelExporterOBJ::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UWorld::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("OBJ")) );
	new(FormatDescription)FString(TEXT("Object File"));
}
static void ExportPolys( UPolys* Polys, INT &PolyNum, INT TotalPolys, FOutputDevice& Ar, FFeedbackContext* Warn, UBOOL bSelectedOnly, UModel* Model )
{
    UMaterialInterface *DefaultMaterial = GEngine->DefaultMaterial;

    UMaterialInterface *CurrentMaterial;

    CurrentMaterial = DefaultMaterial;

	// Create a list of the selected polys by finding the selected BSP Surfaces
	TArray<INT> SelectedPolys;
	if (bSelectedOnly)
	{
		// loop through the bsp surfaces
		for (INT SurfIndex = 0; SurfIndex < Model->Surfs.Num(); SurfIndex++)
		{
			FBspSurf& Surf = Model->Surfs(SurfIndex);

			// if the surface is selected, then add the poly it points to to our list of selected polys
			if ((Surf.PolyFlags & PF_Selected) && Surf.Actor)
			{
				SelectedPolys.AddItem(Surf.iBrushPoly);
			}
		}
	}

	for (INT i = 0; i < Polys->Element.Num(); i++)
	{
		Warn->StatusUpdatef( PolyNum++, TotalPolys, *LocalizeUnrealEd(TEXT("ExportingLevelToOBJ")) );

		FPoly *Poly = &Polys->Element(i);

		// if we are exporting only selected, then check to see if we found this poly to be selected
		if (bSelectedOnly && SelectedPolys.FindItemIndex(i) == INDEX_NONE)
		{
			continue;
		}

		int j;

        if (
            (!Poly->Material && (CurrentMaterial != DefaultMaterial)) ||
            (Poly->Material && (Poly->Material != CurrentMaterial))
           )
        {
            FString Material;

            CurrentMaterial = Poly->Material;

            if( CurrentMaterial )
        	    Material = FString::Printf (TEXT("usemtl %s"), *CurrentMaterial->GetName());
            else
        	    Material = FString::Printf (TEXT("usemtl DefaultMaterial"));

            Ar.Logf (TEXT ("%s\n"), *Material );
        }

		for (j = 0; j < Poly->Vertices.Num(); j++)
        {
            // Transform to Lightwave's coordinate system
			Ar.Logf (TEXT("v %f %f %f\n"), Poly->Vertices(j).X, Poly->Vertices(j).Z, Poly->Vertices(j).Y);
        }

		FVector	TextureBase = Poly->Base;

        FVector	TextureX, TextureY;

		TextureX = Poly->TextureU / (FLOAT)CurrentMaterial->GetWidth();
		TextureY = Poly->TextureV / (FLOAT)CurrentMaterial->GetHeight();

		for (j = 0; j < Poly->Vertices.Num(); j++)
        {
            // Invert the y-coordinate (Lightwave has their bitmaps upside-down from us).
    		Ar.Logf (TEXT("vt %f %f\n"),
			    (Poly->Vertices(j) - TextureBase) | TextureX, -((Poly->Vertices(j) - TextureBase) | TextureY));
        }

		Ar.Logf (TEXT("f "));

        // Reverse the winding order so Lightwave generates proper normals:
		for (j = Poly->Vertices.Num() - 1; j >= 0; j--)
			Ar.Logf (TEXT("%i/%i "), (j - Poly->Vertices.Num()), (j - Poly->Vertices.Num()));

		Ar.Logf (TEXT("\n"));
	}
}


UBOOL ULevelExporterOBJ::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags)
{
	UWorld* World = CastChecked<UWorld>(Object);
	ULevel* Level = World->PersistentLevel;

	GEditor->bspBuildFPolys(World->GetModel(), 0, 0 );
	UPolys* Polys = World->GetModel()->Polys;

    UMaterialInterface *DefaultMaterial = GEngine->DefaultMaterial;

    UMaterialInterface *CurrentMaterial;

    INT i, j, TotalPolys;
    INT PolyNum;
    INT iActor;

    // Calculate the total number of polygons to export:

    PolyNum = 0;
    TotalPolys = Polys->Element.Num();

	for( iActor=0; iActor<Level->Actors.Num(); iActor++ )
	{
		AActor* Actor = Level->Actors(iActor);

		// only export selected actors if the flag is set
        if( !Actor || (bSelectedOnly && !Actor->IsSelected()))
		{
            continue;
		}
        
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>( Actor );
        if( !StaticMeshActor || !StaticMeshActor->StaticMeshComponent || !StaticMeshActor->StaticMeshComponent->StaticMesh )
		{
            continue;
		}
         
        UStaticMesh* StaticMesh = StaticMeshActor->StaticMeshComponent->StaticMesh;

        TotalPolys += StaticMesh->LODModels(0).RawTriangles.GetElementCount();
    }

    // Export the BSP

	Ar.Logf (TEXT("# OBJ File Generated by UnrealEd\n"));

	Ar.Logf (TEXT("o PersistentLevel\n"));
    Ar.Logf (TEXT("g BSP\n") );

    ExportPolys( Polys, PolyNum, TotalPolys, Ar, Warn, bSelectedOnly, World->GetModel() );

    // Export the static meshes

    CurrentMaterial = DefaultMaterial;

	for( iActor=0; iActor<Level->Actors.Num(); iActor++ )
	{
		AActor* Actor = Level->Actors(iActor);

		// only export selected actors if the flag is set
        if( !Actor || (bSelectedOnly && !Actor->IsSelected()))
		{
            continue;
		}
        
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>( Actor );
        if( !StaticMeshActor || !StaticMeshActor->StaticMeshComponent || !StaticMeshActor->StaticMeshComponent->StaticMesh )
		{
            continue;
		}

        FMatrix LocalToWorld = Actor->LocalToWorld();
         
    	Ar.Logf (TEXT("g %s\n"), *Actor->GetName() );

        const FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) StaticMeshActor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.Lock(LOCK_READ_ONLY);

	    for (i = 0; i < StaticMeshActor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.GetElementCount(); i++)
	    {
			Warn->StatusUpdatef( PolyNum++, TotalPolys, *LocalizeUnrealEd(TEXT("ExportingLevelToOBJ")) );

            const FStaticMeshTriangle &Triangle = RawTriangleData[i];

            if ( CurrentMaterial != DefaultMaterial )
            {
                FString Material;

                CurrentMaterial = StaticMeshActor->StaticMeshComponent->GetMaterial(Triangle.MaterialIndex); // sjs

                if( CurrentMaterial )
				{
        	        Material = FString::Printf (TEXT("usemtl %s"), *CurrentMaterial->GetName());
				}
                else
				{
        	        Material = FString::Printf (TEXT("usemtl DefaultMaterial"));
				}

                Ar.Logf (TEXT ("%s\n"), *Material);
            }

		    for( j = 0; j < ARRAY_COUNT(Triangle.Vertices); j++ )
            {
                FVector V = Triangle.Vertices[j];

                V = LocalToWorld.TransformFVector( V );

                // Transform to Lightwave's coordinate system
			    Ar.Logf( TEXT("v %f %f %f\n"), V.X, V.Z, V.Y );
            }

		    for( j = 0; j < ARRAY_COUNT(Triangle.Vertices); j++ )
            {
                // Invert the y-coordinate (Lightwave has their bitmaps upside-down from us).
    		    Ar.Logf( TEXT("vt %f %f\n"), Triangle.UVs[j][0].X, -Triangle.UVs[j][0].Y );
            }

		    Ar.Logf (TEXT("f "));

		    for( j = 0; j < ARRAY_COUNT(Triangle.Vertices); j++ )
			{
			    Ar.Logf (TEXT("%i/%i "), (j - ARRAY_COUNT(Triangle.Vertices)), (j - ARRAY_COUNT(Triangle.Vertices)));
			}

		    Ar.Logf (TEXT("\n"));
	    }

		StaticMeshActor->StaticMeshComponent->StaticMesh->LODModels(0).RawTriangles.Unlock();
	}

	GWorld->GetModel()->Polys->Element.Empty();

	Ar.Logf (TEXT("# dElaernU yb detareneG eliF JBO\n"));
	return 1;
}

IMPLEMENT_CLASS(ULevelExporterOBJ);

/*------------------------------------------------------------------------------
	UPolysExporterOBJ implementation.
------------------------------------------------------------------------------*/
/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UPolysExporterOBJ::InitializeIntrinsicPropertyValues()
{
	SupportedClass = UPolys::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("OBJ")) );
	new(FormatDescription)FString(TEXT("Object File"));
}
UBOOL UPolysExporterOBJ::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags)
{
    UPolys* Polys = CastChecked<UPolys> (Object);

    INT PolyNum = 0;
    INT TotalPolys = Polys->Element.Num();

	Ar.Logf (TEXT("# OBJ File Generated by UnrealEd\n"));

    ExportPolys( Polys, PolyNum, TotalPolys, Ar, Warn, false, NULL );

	Ar.Logf (TEXT("# dElaernU yb detareneG eliF JBO\n"));

	return 1;
}

IMPLEMENT_CLASS(UPolysExporterOBJ);

/*------------------------------------------------------------------------------
	USequenceExporterT3D implementation.
------------------------------------------------------------------------------*/
/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void USequenceExporterT3D::InitializeIntrinsicPropertyValues()
{
	SupportedClass = USequence::StaticClass();
	bText = 1;
	PreferredFormatIndex = FormatExtension.AddItem( FString(TEXT("T3D")) );
	new(FormatDescription)FString(TEXT("Unreal sequence text"));
	new(FormatExtension)FString(TEXT("COPY"));
	new(FormatDescription)FString(TEXT("Unreal sequence text"));
}
UBOOL USequenceExporterT3D::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, DWORD PortFlags)
{
	USequence* Sequence = CastChecked<USequence>( Object );

	UBOOL bAsSingle = (appStricmp(Type,TEXT("T3D"))==0);

	// If exporting everything - just pass in the sequence.
	if(bAsSingle)
	{
		Ar.Logf( TEXT("%sBegin Object Class=%s Name=%s Archetype=%s'%s'\r\n"), appSpc(TextIndent), *Sequence->GetClass()->GetName(), *Sequence->GetName(), *Sequence->GetArchetype()->GetClass()->GetName(), *Sequence->GetArchetype()->GetPathName() );
			ExportObjectInner( Context, Sequence, Ar, PortFlags );
		Ar.Logf( TEXT("%sEnd Object\r\n"), appSpc(TextIndent) );
	}
	// If we want only a selection, iterate over to find the SequenceObjects we want.
	else
	{
		for(INT i=0; i<Sequence->SequenceObjects.Num(); i++)
		{
			USequenceObject* SeqObj = Sequence->SequenceObjects(i);
			if( SeqObj && SeqObj->IsSelected() )
			{
				Ar.Logf( TEXT("%sBegin Object Class=%s Name=%s Archetype=%s'%s'\r\n"), appSpc(TextIndent), *SeqObj->GetClass()->GetName(), *SeqObj->GetName(), *SeqObj->GetArchetype()->GetClass()->GetName(), *SeqObj->GetArchetype()->GetPathName() );
					// when we export sequences in this sequnce, we don't want to count on selection, we want all objects to be exported
					// and PPF_Copy will only exported selected objects
					ExportObjectInner( Context, SeqObj, Ar, PortFlags & ~PPF_Copy );
				Ar.Logf( TEXT("%sEnd Object\r\n"), appSpc(TextIndent) );
			}
		}
	}

	return true;
}

IMPLEMENT_CLASS(USequenceExporterT3D);
