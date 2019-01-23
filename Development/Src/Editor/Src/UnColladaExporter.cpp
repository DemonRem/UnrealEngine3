/*=============================================================================
	COLLADA exporter for Unreal Engine 3.
	Based on Feeling Software's FCollada.
	Copyright 2006 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "EngineInterpolationClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineParticleClasses.h"
#include "UnCollada.h"
#include "UnColladaExporter.h"
#include "UnColladaStaticMesh.h"
#include "UnTerrain.h"

namespace UnCollada {

// Custom conversion functors.
class FCDConversionSampleRadiusFunctor : public FCDConversionFunctor
{
public:
	FLOAT FocusDistanceTimes10;
	FCDConversionSampleRadiusFunctor(FLOAT _FocusDistance)
		:	FocusDistanceTimes10(_FocusDistance * 10.0f) {}
	virtual ~FCDConversionSampleRadiusFunctor() {}
	virtual FLOAT operator() (FLOAT Value) { return Value > 1.0f ? FocusDistanceTimes10 / Value : FocusDistanceTimes10; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CExporterActor - A structure to hold the FCollada structures for a given UE3 actor
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CExporterActor::CExporterActor()
	:	Actor(NULL), ColladaNode(NULL), ColladaEntity(NULL), ColladaInstance(NULL)
{
}

CExporterActor::~CExporterActor()
{
	// Release all the properties
	INT PropertyCount = Properties.Num();
	for (INT Idx = 0; Idx < PropertyCount; ++Idx)
	{
		CExporterAnimatableProperty* Property = Properties(Idx);
		SAFE_DELETE(Property->Conversion);
		SAFE_DELETE(Property);
	}
	Properties.Empty();

	// Reset the actor link pointers
	Actor = NULL;
	ColladaNode = NULL;
	ColladaEntity = NULL;
	ColladaInstance = NULL;
}

void CExporterActor::Instantiate(FCDEntity* _ColladaEntity)
{
	ColladaEntity = _ColladaEntity;

	if (ColladaNode != NULL)
	{
		if (ColladaEntity->GetObjectType().Includes(FCDLight::GetClassType())
			|| ColladaEntity->GetObjectType().Includes(FCDCamera::GetClassType()))
		{
			// For cameras and lights: always add a Y-pivot rotation to get the correct coordinate system.
			FCDSceneNode* ColladaPivotNode = ColladaNode->AddChildNode();
			FCDTRotation* ColladaPivotRotationZ = (FCDTRotation*) ColladaPivotNode->AddTransform(FCDTransform::ROTATION);
			FCDTRotation* ColladaPivotRotationX = (FCDTRotation*) ColladaPivotNode->AddTransform(FCDTransform::ROTATION);
			ColladaPivotRotationZ->SetRotation(FMVector3::ZAxis, -90.0f);
			ColladaPivotRotationX->SetRotation(FMVector3::XAxis, 90.0f);
			ColladaPivotNode->SetDaeId(ColladaNode->GetDaeId() + "-pivot");
			ColladaInstance = ColladaPivotNode->AddInstance(ColladaEntity);
		}
		else
		{
			ColladaInstance = ColladaNode->AddInstance(ColladaEntity);
		}
	}
}

CExporterAnimatableProperty* CExporterActor::FindProperty(const TCHAR* Name)
{
	// The UE3 generates property names programmatically (?) and
	// prefixes the property names with the name of the structure(s) needed to
	// access them.
	// We only want to consider the property suffix.
	const TCHAR* Suffix = appStrrchr(Name, '.');
	if (Suffix != NULL)
	{
		Name = Suffix + 1;
	}

	// Look for the property name suffix in the COLLADA linking structures.
	INT PropertyCount = Properties.Num();
	for (INT Idx = 0; Idx < PropertyCount; ++Idx)
	{
		CExporterAnimatableProperty* Property = Properties(Idx);
		if (appStrcmp(*Property->PropertyName, Name) == 0)
		{
			return Property;
		}
	}
	return NULL;
}

CExporterAnimatableProperty* CExporterActor::CreateProperty(const TCHAR* Name, FLOAT* ColladaValues, INT ColladaValueCount, FCDConversionFunctor* Conversion)
{
	// Create the UE3-COLLADA property linking structure
	CExporterAnimatableProperty* Property = new CExporterAnimatableProperty();
	Property->PropertyName = Name;
	Property->ColladaValues.Add(ColladaValueCount);
	for (INT Idx = 0; Idx < ColladaValueCount; ++Idx)
	{
		Property->ColladaValues(Idx) = ColladaValues + Idx;
	}
	Property->Conversion = Conversion;
	Properties.AddItem(Property);

	// Verify that COLLADA has an animated entity for these values. This is the case only for the <extra> values.
	FCDocument* ColladaDocument = ColladaNode->GetDocument();
	FCDAnimated* AnimatedEntity = ColladaDocument->FindAnimatedValue(ColladaValues);
	if (AnimatedEntity == NULL)
	{
		switch (ColladaValueCount)
		{
		case 1: AnimatedEntity = FCDAnimatedFloat::Create(ColladaDocument, ColladaValues); break;
		case 3: AnimatedEntity = FCDAnimatedPoint3::Create(ColladaDocument, (FMVector3*) ColladaValues); break;
		case 4: AnimatedEntity = FCDAnimatedColor::Create(ColladaDocument, (FMVector4*) ColladaValues); break;
		default: break;
		}
	}

	return Property;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CExporter - Main COLLADA exporter singleton
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CExporter::CExporter()
	:	ColladaDocument(NULL)
{
}

CExporter::~CExporter()
{
	CloseDocument();
}

/**
 * Returns the exporter singleton. It will be created on the first request.
 */
CExporter* CExporter::GetInstance()
{
	static CExporter* ExporterInstance = new CExporter();
	return ExporterInstance;
}

/**
 * Creates and readies an empty document for export.
 */
void CExporter::CreateDocument()
{
	// Clears any old document that is still present.
	SAFE_DELETE(ColladaDocument);

	// Create an empty COLLADA document and fill up its asset information.
	ColladaDocument = FCollada::NewTopDocument();
	ExportAsset(ColladaDocument->GetAsset());
}

/**
 * Writes the COLLADA document to disk and releases it.
 */
void CExporter::WriteToFile(const TCHAR* Filename)
{
	// Write out the COLLADA document to disk.
	ColladaDocument->WriteToFile(Filename);

	// Release the document
	CloseDocument();
}

/**
 * Closes the COLLADA document, releasing its memory.
 */
void CExporter::CloseDocument()
{
	// You need to release all the buffered actor structures before releasing the FCollada document.
	INT ActorCount = ColladaActors.Num();
	for (INT Idx = 0; Idx < ActorCount; ++Idx)
	{
		CExporterActor* ColladaActor = ColladaActors(Idx);
		SAFE_DELETE(ColladaActor);
	}
	ColladaActors.Empty();
	SAFE_DELETE(ColladaDocument);
}

/**
 * Exports the basic scene information to the COLLADA document.
 */
void CExporter::ExportLevelMesh(ULevel* Level)
{
	if (ColladaDocument == NULL || Level == NULL) return;

	// Exports the level's scene geometry
	if (Level->Model != NULL && Level->Model->VertexBuffer.Vertices.Num() > 2 && Level->Model->MaterialIndexBuffers.Num() > 0)
	{
		// Create the COLLADA structures to hold the scene geometry
		FCDSceneNode* ColladaSceneRoot = ColladaDocument->GetVisualSceneRoot();
		if (ColladaSceneRoot == NULL)
		{
			ColladaSceneRoot = ColladaDocument->AddVisualScene();
		}
		FCDSceneNode* ColladaWorldGeometryNode = ColladaSceneRoot->AddChildNode();
		ColladaWorldGeometryNode->SetName(TEXT("LevelMesh"));
		ColladaWorldGeometryNode->SetDaeId("LevelMesh-node");
		FCDGeometry* ColladaWorldGeometry = ColladaDocument->GetGeometryLibrary()->AddEntity();
		ColladaWorldGeometry->SetName(TEXT("LevelMesh"));
		ColladaWorldGeometry->SetDaeId("LevelMesh-obj");
		FCDGeometryInstance* ColladaWorldInstance = (FCDGeometryInstance*) ColladaWorldGeometryNode->AddInstance(ColladaWorldGeometry);

		// Export the mesh for the world
		ExportModel(Level->Model, ColladaWorldGeometry, ColladaWorldInstance);
	}

	// Export all the recognized global actors.
	// Right now, this only includes lights.
	INT ActorCount = GWorld->CurrentLevel->Actors.Num();
	for (INT ActorIndex = 0; ActorIndex < ActorCount; ++ActorIndex)
	{
		AActor* Actor = GWorld->CurrentLevel->Actors(ActorIndex);
		if (Actor != NULL)
		{
			if (Actor->IsA(ALight::StaticClass()))
			{
				ExportLight((ALight*) Actor);
			}
			else if (Actor->IsA(AStaticMeshActor::StaticClass()))
			{
				ExportStaticMesh(Actor, ((AStaticMeshActor*) Actor)->StaticMeshComponent);
			}
			else if (Actor->IsA(ADynamicSMActor::StaticClass()))
			{
				ExportStaticMesh(Actor, ((ADynamicSMActor*) Actor)->StaticMeshComponent);
			}
			else if (Actor->IsA(ABrush::StaticClass()))
			{
				// All brushes should be included within the world geometry exported above.
				// ExportBrush((ABrush*) Actor);
			}
			else if (Actor->IsA(ATerrain::StaticClass()))
			{
				// ExportTerrain?((ATerrain*) Actor);
			}
			else if (Actor->IsA(AEmitter::StaticClass()))
			{
				ExportActor(Actor); // Just export the placement of the particle emitter.
			}
		}
	}
}

/**
 * Exports the light-specific information for a UE3 light actor.
 */
void CExporter::ExportLight(ALight* Actor)
{
	if (ColladaDocument == NULL || Actor == NULL || Actor->LightComponent == NULL) return;

	// Export the basic actor information.
	CExporterActor* ColladaActor = ExportActor(Actor);
	ULightComponent* BaseLight = Actor->LightComponent;

	// Create a properly-named COLLADA light structure and instantiate it in the COLLADA scene graph
	FCDLight* ColladaLight = ColladaDocument->GetLightLibrary()->AddEntity();
	ColladaLight->SetName(*Actor->GetName());
	ColladaLight->SetDaeId(TO_STRING(*Actor->GetName()) + "-object");
	ColladaActor->Instantiate(ColladaLight);

	// Export the 'enabled' flag as the visibility for the scene node.
	ColladaActor->ColladaNode->SetVisibility(Actor->bEnabled);

	// Export the basic light information
	ColladaLight->SetIntensity(BaseLight->Brightness);
	ColladaActor->CreateProperty(TEXT("Brightness"), &ColladaLight->GetIntensity());
	ColladaLight->SetColor(ToFMVector3(BaseLight->LightColor)); // not animatable

	// Look for the higher-level light types and determine the lighting method
	if (BaseLight->IsA(UPointLightComponent::StaticClass()))
	{
		UPointLightComponent* PointLight = (UPointLightComponent*) BaseLight;
		if (BaseLight->IsA(USpotLightComponent::StaticClass()))
		{
			USpotLightComponent* SpotLight = (USpotLightComponent*) BaseLight;
			ColladaLight->SetLightType(FCDLight::SPOT);

			// Export the spot light parameters.
			ColladaLight->SetFallOffAngle(SpotLight->InnerConeAngle); // not animatable
			ColladaLight->SetOuterAngle(SpotLight->OuterConeAngle); // not animatable
		}
		else
		{
			ColladaLight->SetLightType(FCDLight::POINT);
		}

		// Export the point light parameters.
		ColladaLight->SetDropoff(PointLight->FalloffExponent);
		ColladaActor->CreateProperty(TEXT("FalloffExponent"), &ColladaLight->GetDropoff());
		ColladaLight->SetLinearAttenuationFactor(1.0f / PointLight->Radius);
		ColladaActor->CreateProperty(TEXT("Radius"), &ColladaLight->GetLinearAttenuationFactor());
	}
	else if (BaseLight->IsA(UDirectionalLightComponent::StaticClass()))
	{
		// The directional light has no interesting properties.
		ColladaLight->SetLightType(FCDLight::DIRECTIONAL);
		ColladaLight->SetOvershoot(true);
	}
}

void CExporter::ExportCamera(ACameraActor* Actor)
{
	if (ColladaDocument == NULL || Actor == NULL) return;

	// Export the basic actor information.
	CExporterActor* ColladaActor = ExportActor(Actor);

	// Create a properly-named COLLADA light structure and instantiate it in the COLLADA scene graph
	FCDCamera* ColladaCamera = ColladaDocument->GetCameraLibrary()->AddEntity();
	ColladaCamera->SetName(*Actor->GetName());
	ColladaCamera->SetDaeId(TO_STRING(*Actor->GetName()) + "-object");
	ColladaActor->Instantiate(ColladaCamera);

	// Export the view area information
	ColladaCamera->SetPerspective();
	ColladaCamera->SetFovX(Actor->FOVAngle);
	ColladaActor->CreateProperty(TEXT("FOVAngle"), &ColladaCamera->GetFovX());
	ColladaCamera->SetAspectRatio(Actor->AspectRatio);
	ColladaActor->CreateProperty(TEXT("AspectRatio"), &ColladaCamera->GetAspectRatio());

	// Push the near/far clip planes away, as the UE3 engine uses larger values than the default.
	ColladaCamera->SetNearZ(10.0f);
	ColladaCamera->SetFarZ(10000.0f);

	// Export the post-processing information: only one of depth-of-field or motion blur can be supported in 3dsMax.
	// And Maya only supports some simple depth-of-field effect.
	FPostProcessSettings* PostProcess = &Actor->CamOverridePostProcess;
	if (PostProcess->bEnableDOF)
	{
		// Export the depth-of-field information
		FCDETechnique* CustomTechnique = ColladaCamera->GetExtra()->AddTechnique(DAE_FCOLLADA_PROFILE);
		FCDENode* MaxDepthOfFieldNode = CustomTechnique->AddChildNode(DAEFC_CAMERA_DEPTH_OF_FIELD_ELEMENT);

		// 'focal depth' <- 'focus distance'.
		if (PostProcess->DOF_FocusType == FOCUS_Distance)
		{
			FCDENode* FocalDepthNode = MaxDepthOfFieldNode->AddParameter(DAEFC_CAMERA_DOF_FOCALDEPTH_PARAMETER, PostProcess->DOF_FocusDistance);
			ColladaActor->CreateProperty(TEXT("DOF_FocusDistance"), &FocalDepthNode->GetAnimated()->GetDummy(), 1, NULL);
		}
		else if (PostProcess->DOF_FocusType == FOCUS_Position)
		{
			MaxDepthOfFieldNode->AddParameter(DAEFC_CAMERA_DOF_FOCALDEPTH_PARAMETER, (Actor->Location - PostProcess->DOF_FocusPosition).Size());
		}

		// 'sample radius' <- 10 * 'focus distance' / 'focus radius'.
		// Since 'focus distance' is animatable, this complicates thing: we'll simply scale using the local value.
		FCDConversionSampleRadiusFunctor* SampleRadiusConversion = new FCDConversionSampleRadiusFunctor(PostProcess->DOF_FocusDistance);
		FCDENode* SampleRadiusNode = MaxDepthOfFieldNode->AddParameter(DAEFC_CAMERA_DOF_SAMPLERADIUS_PARAMETER, (*SampleRadiusConversion)(PostProcess->DOF_FocusInnerRadius));
		ColladaActor->CreateProperty(TEXT("DOF_FocusInnerRadius"), &SampleRadiusNode->GetAnimated()->GetDummy(), 1, SampleRadiusConversion);

		// 'tile size' <- 'blur kernel size'.
		FCDENode* TileSizeNode = MaxDepthOfFieldNode->AddParameter(DAEFC_CAMERA_DOF_TILESIZE_PARAMETER, PostProcess->DOF_BlurKernelSize);
		ColladaActor->CreateProperty(TEXT("DOF_BlurKernelSize"), &TileSizeNode->GetAnimated()->GetDummy(), 1, NULL);

		// 'use target distance'
		// Always set to false, since we'll be overwriting the target distance with the 'focus distance'.
		MaxDepthOfFieldNode->AddParameter(DAEFC_CAMERA_DOF_USETARGETDIST_PARAMETER, false);
	}
	else if (PostProcess->bEnableMotionBlur)
	{
		// Export the motion-blur information
		FCDETechnique* CustomTechnique = ColladaCamera->GetExtra()->AddTechnique(DAEMAX_MAX_PROFILE);
		FCDENode* MotionBlurNode = CustomTechnique->AddChildNode(DAEMAX_CAMERA_MOTIONBLUR_ELEMENT);

		// 'duration' <- 'blur amount'
		FCDENode* BlurAmountNode = MotionBlurNode->AddParameter(DAEMAX_CAMERA_MB_DURATION_PARAMETER, PostProcess->MotionBlur_Amount);
		ColladaActor->CreateProperty(TEXT("MotionBlur_Amount"), &BlurAmountNode->GetAnimated()->GetDummy(), 1, NULL);
	}
}

/**
 * Exports the mesh and the actor information for a UE3 brush actor.
 */
void CExporter::ExportBrush(ABrush* Actor)
{
	if (ColladaDocument == NULL || Actor == NULL || Actor->BrushComponent == NULL) return;

	// Retrieve the information structures, verifying the integrity of the data.
	UModel* Model = Actor->BrushComponent->Brush;
	if (Model == NULL || Model->VertexBuffer.Vertices.Num() < 3 || Model->MaterialIndexBuffers.Num() == 0) return;

	// Create the COLLADA actor, the COLLADA geometry and instantiate it.
	CExporterActor* ColladaActor = ExportActor(Actor);
	FCDGeometry* ColladaGeometry = ColladaDocument->GetGeometryLibrary()->AddEntity();
	ColladaGeometry->SetName(*Actor->GetName());
	ColladaGeometry->SetDaeId(TO_STRING(ColladaGeometry->GetDaeId()) + "-obj");
	ColladaActor->Instantiate(ColladaGeometry);
	FCDGeometryInstance* ColladaInstance = (FCDGeometryInstance*) ColladaActor->ColladaInstance;

	// Export the mesh information
	ExportModel(Model, ColladaGeometry, ColladaInstance);
}

void CExporter::ExportModel(UModel* Model, FCDGeometry* ColladaGeometry, FCDGeometryInstance* ColladaInstance)
{
	INT VertexCount = Model->VertexBuffer.Vertices.Num();
	INT MaterialCount = Model->MaterialIndexBuffers.Num();

	// Create the mesh and three data sources for the vertex positions, normals and texture coordinates.
	FCDGeometryMesh* ColladaMesh = ColladaGeometry->CreateMesh();
	FCDGeometrySource* PositionSource = ColladaMesh->AddVertexSource(FUDaeGeometryInput::POSITION);
	FCDGeometrySource* NormalSource = ColladaMesh->AddSource(FUDaeGeometryInput::NORMAL);
	FCDGeometrySource* TexcoordSource = ColladaMesh->AddSource(FUDaeGeometryInput::TEXCOORD);
	PositionSource->SetDaeId(ColladaGeometry->GetDaeId() + "-positions");
	NormalSource->SetDaeId(ColladaGeometry->GetDaeId() + "-normals");
	TexcoordSource->SetDaeId(ColladaGeometry->GetDaeId() + "-texcoords");
	PositionSource->SetSourceStride(3);
	NormalSource->SetSourceStride(3);
	TexcoordSource->SetSourceStride(2);
	PositionSource->GetSourceData().reserve(3 * VertexCount);
	NormalSource->GetSourceData().reserve(3 * VertexCount);
	TexcoordSource->GetSourceData().reserve(3 * VertexCount);
	for (INT VertexIdx = 0; VertexIdx < VertexCount; ++VertexIdx)
	{
		FModelVertex& Vertex = Model->VertexBuffer.Vertices(VertexIdx);
		FVector Normal = (FVector) Vertex.TangentZ;
		PositionSource->GetSourceData().push_back(Vertex.Position.X);
		PositionSource->GetSourceData().push_back(-Vertex.Position.Y);
		PositionSource->GetSourceData().push_back(Vertex.Position.Z);
		NormalSource->GetSourceData().push_back(Normal.X);
		NormalSource->GetSourceData().push_back(-Normal.Y);
		NormalSource->GetSourceData().push_back(Normal.Z);
		TexcoordSource->GetSourceData().push_back(Vertex.TexCoord.X);
		TexcoordSource->GetSourceData().push_back(-Vertex.TexCoord.Y);
	}

	// Create the materials and the per-material tesselation structures.
	TDynamicMap<UMaterialInterface*,FRawIndexBuffer32>::TIterator MaterialIterator(Model->MaterialIndexBuffers);
	for (; (UBOOL) MaterialIterator; ++MaterialIterator)
	{
		UMaterialInterface* MaterialInterface = MaterialIterator.Key();
		FRawIndexBuffer32& IndexBuffer = MaterialIterator.Value();
		INT IndexCount = IndexBuffer.Indices.Num();
		if (IndexCount < 3) continue;

		// Create the COLLADA polygons set.
		FCDGeometryPolygons* ColladaPolygons = ColladaMesh->AddPolygons();

		// Add normal and texcoord source inputs to the per-vertex input.
		// Since the position source is a per-vertex source, it is automatically added
		// to the per-vertex input that is automatically created for the polygon set.
		ColladaPolygons->AddInput(NormalSource, 0);
		ColladaPolygons->AddInput(TexcoordSource, 0);

		// Retrieve and fill in the index buffer.
		UInt32List* Indices = ColladaPolygons->FindIndices(PositionSource);
		for (INT IndexIdx = 0; IndexIdx < IndexCount; ++IndexIdx)
		{
			Indices->push_back(IndexBuffer.Indices(IndexIdx));
		}

		// The UModel class always contains triangle lists
		UInt32List& VertexCounts = ColladaPolygons->GetFaceVertexCounts();
		VertexCounts.resize(IndexCount / 3, 3);

		// Are NULL materials okay?
		if (MaterialInterface != NULL && MaterialInterface->GetMaterial() != NULL)
		{
			FCDMaterial* ColladaMaterial = ExportMaterial(MaterialInterface->GetMaterial());
			ColladaPolygons->SetMaterialSemantic(ColladaMaterial->GetName());
			ColladaInstance->AddMaterialInstance(ColladaMaterial, ColladaPolygons);
		}
		else
		{
			// Set no material: Maya/3dsMax will assign some default material.
		}
	}
}

void CExporter::ExportStaticMesh(AActor* Actor, UStaticMeshComponent* StaticMeshComponent)
{
	if (ColladaDocument == NULL || Actor == NULL || StaticMeshComponent == NULL) return;

	// Retrieve the static mesh rendering information at the correct LOD level.
	UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;
	if (StaticMesh == NULL || StaticMesh->LODModels.Num() == 0) return;
	INT LODIndex = StaticMeshComponent->ForcedLodModel;
	if (LODIndex >= StaticMesh->LODModels.Num())
	{
		LODIndex = StaticMesh->LODModels.Num() - 1;
	}
	FStaticMeshRenderData& RenderMesh = StaticMesh->LODModels(LODIndex);

	// Export the render mesh as a COLLADA geometry.
	// This handles the duplication of static meshes.
	CStaticMesh StaticMeshExporter(this);
	FCDGeometry* ColladaGeometry = StaticMeshExporter.ExportStaticMesh(RenderMesh, *Actor->GetName());
	if (ColladaGeometry == NULL) return;

	// Create the COLLADA actor for this UE actor and instantiate it.
	CExporterActor* ColladaActor = ExportActor(Actor);
	ColladaActor->Instantiate(ColladaGeometry);
	FCDGeometryInstance* ColladaGeometryInstance = (FCDGeometryInstance*) ColladaActor->ColladaInstance;
	FCDGeometryMesh* ColladaMesh = ColladaGeometry->GetMesh();

	// Generate the materials and links for this COLLADA geometry.
	size_t ColladaPolygonsCount = ColladaMesh->GetPolygonsCount();
	for (size_t PolygonsIndex = 0; PolygonsIndex < ColladaPolygonsCount; ++PolygonsIndex)
	{
		FCDGeometryPolygons* ColladaPolygons = ColladaMesh->GetPolygons(PolygonsIndex);

		// Retrieve the static mesh instance's material instance for this polygons set.
		UMaterialInterface* MaterialInterface = StaticMeshComponent->GetMaterial((INT) PolygonsIndex);
		if (MaterialInterface == NULL) continue;
		FCDMaterial* ColladaMaterial = ExportMaterial(MaterialInterface->GetMaterial());

		// Attach the material and the polygons set.
		UNUSED(FCDMaterialInstance* ColladaMaterialInstance = )
			ColladaGeometryInstance->AddMaterialInstance(ColladaMaterial, ColladaPolygons);
	}
}

/**
* Exports the profile_COMMON information for a UE3 material.
*/
FCDMaterial* CExporter::ExportMaterial(UMaterial* Material)
{
	if (ColladaDocument == NULL || Material == NULL) return NULL;

	// Verify that this material has not already been exported:
	// the FCDMaterial objects hold a pointer to the equivalent UMaterial object.
	size_t MaterialCount = ColladaDocument->GetMaterialLibrary()->GetMaterialCount();
	for (size_t Idx = 0; Idx < MaterialCount; ++Idx)
	{
		FCDMaterial* ColladaMaterial = ColladaDocument->GetMaterialLibrary()->GetMaterial(Idx);
		if (ColladaMaterial->GetUserHandle() == Material)
		{
			return ColladaMaterial;
		}
	}

	// Create the COLLADA material and effect structures
	FCDMaterial* ColladaMaterial = ColladaDocument->GetMaterialLibrary()->AddMaterial();
	ColladaMaterial->SetName(*Material->GetName());
	ColladaMaterial->SetDaeId(TO_STRING(ColladaMaterial->GetName()));
	FCDEffect* ColladaEffect = ColladaDocument->GetEffectLibrary()->AddEffect();
	ColladaEffect->SetName(*Material->GetName());
	ColladaEffect->SetDaeId(TO_STRING(ColladaEffect->GetDaeId() + "-fx"));
	ColladaMaterial->SetEffect(ColladaEffect);
	FCDEffectStandard* ColladaStdEffect = (FCDEffectStandard*) ColladaEffect->AddProfile(FUDaeProfileType::COMMON);

	// Set the lighting model
	if (Material->LightingModel == MLM_Phong || Material->LightingModel == MLM_Custom || Material->LightingModel == MLM_SHPRT)
	{
		ColladaStdEffect->SetLightingType(FCDEffectStandard::PHONG);
	}
	else if (Material->LightingModel == MLM_NonDirectional)
	{
		ColladaStdEffect->SetLightingType(FCDEffectStandard::LAMBERT);
	}
	else if (Material->LightingModel == MLM_Unlit)
	{
		ColladaStdEffect->SetLightingType(FCDEffectStandard::CONSTANT);
	}

	// Fill in the profile_COMMON effect with the UE3 material information.
	// TODO: Look for textures/constants in the Material expressions...
	ColladaStdEffect->SetDiffuseColor(ToFMVector4(Material->DiffuseColor.Constant));
	ColladaStdEffect->SetEmissionColor(ToFMVector4(Material->EmissiveColor.Constant));
	ColladaStdEffect->SetSpecularColor(ToFMVector4(Material->SpecularColor.Constant));
	ColladaStdEffect->SetShininess(Material->SpecularPower.Constant);
	ColladaStdEffect->SetTranslucencyFactor(Material->Opacity.Constant);
	return ColladaMaterial;
}

/**
 * Exports the given Matinee sequence information into a COLLADA document.
 */
void CExporter::ExportMatinee(USeqAct_Interp* MatineeSequence)
{
	if (MatineeSequence == NULL || ColladaDocument == NULL) return;

	// If the Matinee editor is not open, we need to initialize the sequence.
	UBOOL InitializeMatinee = MatineeSequence->InterpData == NULL;
	if (InitializeMatinee)
	{
		MatineeSequence->InitInterp();
	}

	// Iterate over the Matinee data groups and export the known tracks
	INT GroupCount = MatineeSequence->GroupInst.Num();
	for (INT GroupIndex = 0; GroupIndex < GroupCount; ++GroupIndex)
	{
		UInterpGroupInst* Group = MatineeSequence->GroupInst(GroupIndex);
		AActor* Actor = Group->GetGroupActor();
		if (Group->Group == NULL || Actor == NULL) continue;
		CExporterActor* ColladaActor = ExportActor(Actor);

		// Look for the class-type of the actor.
		if (Actor->IsA(ACameraActor::StaticClass()))
		{
			ExportCamera((ACameraActor*) Actor);
		}

		// Look for the tracks that we currently support
		INT TrackCount = min(Group->TrackInst.Num(), Group->Group->InterpTracks.Num());
		for (INT TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
		{
			// TODO: Figure out the right way to identify the track and what they affect.
			UInterpTrackInst* TrackInst = Group->TrackInst(TrackIndex);
			UInterpTrack* Track = Group->Group->InterpTracks(TrackIndex);
			if (TrackInst->IsA(UInterpTrackInstMove::StaticClass()) && Track->IsA(UInterpTrackMove::StaticClass()))
			{
				UInterpTrackInstMove* MoveTrackInst = (UInterpTrackInstMove*) TrackInst; // Is there useful info in this one?
				UInterpTrackMove* MoveTrack = (UInterpTrackMove*) Track;
				ExportMatineeTrackMove(ColladaActor, MoveTrack);
			}
			else if (TrackInst->IsA(UInterpTrackInstFloatProp::StaticClass()) && Track->IsA(UInterpTrackFloatProp::StaticClass()))
			{
				UInterpTrackInstFloatProp* PropertyTrackInst = (UInterpTrackInstFloatProp*) TrackInst; // Is there useful info in this one?
				UInterpTrackFloatProp* PropertyTrack = (UInterpTrackFloatProp*) Track;
				ExportMatineeTrackFloatProp(ColladaActor, PropertyTrack);
			}
		}
	}

	if (InitializeMatinee)
	{
		MatineeSequence->TermInterp();
	}
}

/**
 * Writes out the asset information about the UE3 level and user in the COLLADA document.
 */
void CExporter::ExportAsset(FCDAsset* ColladaAsset)
{
	if (ColladaDocument == NULL || ColladaAsset == NULL) return;

	// Add a new contributor to the asset and fill it up with the tool/user information
	FCDAssetContributor* Contributor = ColladaAsset->AddContributor();

	// Set the author's information
	TCHAR Username[128];
	appGetEnvironmentVariable(TEXT("USER"), Username, 128);
	if (Username[0] == 0) appGetEnvironmentVariable(TEXT("USERNAME"), Username, 128);
	Contributor->SetAuthor(Username);
	Contributor->SetAuthoringTool(TEXT("UE3 Editor")); // TODO: Add some versioning information here.

	// Set the up axis information which is crucial to get the DCC tools to accept the data.
	ColladaAsset->SetUpAxis(FMVector3::ZAxis);
}

/**
 * Exports a scene node with the placement indicated by a given UE3 actor.
 * This scene node will always have four transformations: one translation vector and three Euler rotations.
 * The transformations will appear in the order: {translation, rot Z, rot Y, rot X} as per the COLLADA specifications.
 */
CExporterActor* CExporter::ExportActor(AActor* Actor)
{
	// Verify that this actor isn't already exported, create a structure for it
	// and buffer it.
	CExporterActor* ColladaActor = FindActor(Actor);
	if (ColladaActor == NULL)
	{
		ColladaActor = new CExporterActor();
		ColladaActor->Actor = Actor;
		ColladaActors.AddItem(ColladaActor);
	}

	// Create a visual scene node in the COLLADA document for this actor.
	if (ColladaActor->ColladaNode == NULL)
	{
		// Retrieve or create the visual scene root.
		FCDSceneNode* ColladaVisualScene = ColladaDocument->GetVisualSceneRoot();
		if (ColladaVisualScene == NULL)
		{
			ColladaVisualScene = ColladaDocument->AddVisualScene();
		}

		// Create a properly named visual scene node.
		FCDSceneNode* ColladaNode = ColladaVisualScene->AddChildNode();
		ColladaNode->SetName(*Actor->GetName());
		ColladaNode->SetDaeId(TO_STRING(*Actor->GetName()));
		ColladaActor->ColladaNode = ColladaNode;

		// Create the four expected transform
		FCDTTranslation* ColladaPosition = (FCDTTranslation*) ColladaNode->AddTransform(FCDTransform::TRANSLATION);
		FCDTRotation* ColladaRotationZ = (FCDTRotation*) ColladaNode->AddTransform(FCDTransform::ROTATION);
		FCDTRotation* ColladaRotationY = (FCDTRotation*) ColladaNode->AddTransform(FCDTransform::ROTATION);
		FCDTRotation* ColladaRotationX = (FCDTRotation*) ColladaNode->AddTransform(FCDTransform::ROTATION);

		// Set the default position of the actor on the transforms
		// The UE3 transformation is different from COLLADA's Z-up: invert the Y-axis for translations and the Y/Z angle values in rotations.
		FVector InitialPos = Actor->Location;
		FVector InitialRot = Actor->Rotation.Euler();
		ColladaPosition->SetTranslation(ToFMVector3(InitialPos));
		ColladaRotationX->SetRotation(FMVector3::XAxis, InitialRot.X);
		ColladaRotationY->SetRotation(FMVector3::YAxis, -InitialRot.Y);
		ColladaRotationZ->SetRotation(FMVector3::ZAxis, -InitialRot.Z);
	}

	return ColladaActor;
}

/**
 * Exports the Matinee movement track into the COLLADA animation library.
 */
void CExporter::ExportMatineeTrackMove(CExporterActor* ColladaActor, UInterpTrackMove* MoveTrack)
{
	if (ColladaActor == NULL || MoveTrack == NULL) return;
	if (ColladaActor->Actor == NULL || ColladaActor->ColladaNode == NULL) return;
	FCDSceneNode* ColladaNode = ColladaActor->ColladaNode;

	// Retrieve the necessary transforms in the COLLADA scene node to express the UE placement.
    FCDTTranslation* ColladaPosition = (FCDTTranslation*) ColladaNode->GetTransform(0);
	FCDTRotation* ColladaRotationZ = (FCDTRotation*) ColladaNode->GetTransform(1);
	FCDTRotation* ColladaRotationY = (FCDTRotation*) ColladaNode->GetTransform(2);
	FCDTRotation* ColladaRotationX = (FCDTRotation*) ColladaNode->GetTransform(3);

	// Check the 'MoveFrame' flag for the expected curve data frame.
	UBOOL IsRelative = (MoveTrack->MoveFrame == IMF_RelativeToInitial);
	FCDExtra* ColladaExtra = ColladaNode->GetExtra();
	FCDETechnique* ColladaTechnique = ColladaExtra->AddTechnique(DAEUE3_PROFILE_NAME);
	ColladaTechnique->AddParameter(DAEUE3_ABSOLUTE_FRAME_PARAMETER, IsRelative != 0);

	// Retrieve and set the initial position for the relative data frame.
	FVector InitialPos, InitialRot;
	if (IsRelative)
	{
		InitialPos = ColladaActor->Actor->Location;
		InitialRot = ColladaActor->Actor->Rotation.Euler();
	}
	else
	{
		InitialPos = InitialRot = FVector(0.0f, 0.0f, 0.0f);
	}

	// For the Y and Z angular rotations, we need to invert the relative animation frames,
	// While keeping the standard angles constant.

	if (MoveTrack != NULL)
	{
		FCDConversionScaleOffsetFunctor Conversion;

		// Name the animated tracks
		ColladaPosition->SetSubId("translation");
		ColladaRotationX->SetSubId("rotationX");
		ColladaRotationY->SetSubId("rotationY");
		ColladaRotationZ->SetSubId("rotationZ");

		// Export the movement track in the animation curves of these COLLADA transforms.
		// The UE3 transformation is different from COLLADA's Z-up.
		fstring PosTrackName = ColladaNode->GetName() + FC("-position");
		FCDAnimated* ColladaAnimatedPosition = FCDAnimatedPoint3::Create(ColladaDocument, &ColladaPosition->GetTranslation());
		ExportAnimatedVector(PosTrackName.c_str(), ColladaAnimatedPosition, 0, &MoveTrack->PosTrack, 0, &Conversion.Set(1.0f, InitialPos.X));
		ExportAnimatedVector(PosTrackName.c_str(), ColladaAnimatedPosition, 1, &MoveTrack->PosTrack, 1, &Conversion.Set(-1.0f, InitialPos.Y));
		ExportAnimatedVector(PosTrackName.c_str(), ColladaAnimatedPosition, 2, &MoveTrack->PosTrack, 2, &Conversion.Set(1.0f, InitialPos.Z));

		fstring RotXTrackName = ColladaNode->GetName() + FC("-rotX");
		FCDAnimated* ColladaAnimatedRotationX = FCDAnimatedAngle::Create(ColladaDocument, &ColladaRotationX->GetAngle());
		ExportAnimatedVector(RotXTrackName.c_str(), ColladaAnimatedRotationX, 0, &MoveTrack->EulerTrack, 0, &Conversion.Set(1.0f, InitialRot.X));

		fstring RotYTrackName = ColladaNode->GetName() + FC("-rotY");
		FCDAnimated* ColladaAnimatedRotationY = FCDAnimatedAngle::Create(ColladaDocument, &ColladaRotationY->GetAngle());
		ExportAnimatedVector(RotYTrackName.c_str(), ColladaAnimatedRotationY, 0, &MoveTrack->EulerTrack, 1, &Conversion.Set(-1.0f, InitialRot.Y));

		fstring RotZTrackName = ColladaNode->GetName() + FC("-rotZ");
		FCDAnimated* ColladaAnimatedRotationZ = FCDAnimatedAngle::Create(ColladaDocument, &ColladaRotationZ->GetAngle());
		ExportAnimatedVector(RotZTrackName.c_str(), ColladaAnimatedRotationZ, 0, &MoveTrack->EulerTrack, 2, &Conversion.Set(-1.0f, InitialRot.Z));
	}
}

/**
 * Exports the Matinee float property track into the COLLADA animation library.
 */
void CExporter::ExportMatineeTrackFloatProp(CExporterActor* ColladaActor, UInterpTrackFloatProp* PropTrack)
{
	if (ColladaActor == NULL || PropTrack == NULL) return;
	if (ColladaActor->Actor == NULL || ColladaActor->ColladaEntity == NULL) return;

	// Retrieve the property linking structure
	CExporterAnimatableProperty* Property = ColladaActor->FindProperty(PropTrack->PropertyName.GetName());
	if (Property != NULL)
	{
		fstring TrackName = ColladaActor->ColladaNode->GetName() + TEXT("-") + *Property->PropertyName;
		ExportAnimatedFloat(TrackName.c_str(), ColladaDocument->FindAnimatedValue(Property->ColladaValues(0)), &PropTrack->FloatTrack, Property->Conversion);
	}
}

/**
 * Exports the Matinee vector property track into the COLLADA animation library.
 */
void CExporter::ExportMatineeTrackVectorProp(CExporterActor* ColladaActor, UInterpTrackVectorProp* PropTrack)
{
	if (ColladaActor == NULL || PropTrack == NULL) return;
	if (ColladaActor->Actor == NULL || ColladaActor->ColladaEntity == NULL) return;

	// TODO: Find a vector property that is useful to export.
}

/**
 * Exports a given interpolation curve into the COLLADA animation curve.
 */
void CExporter::ExportAnimatedVector(const TCHAR* Name, FCDAnimated* ColladaAnimated, INT ColladaAnimatedIndex, FInterpCurveVector* Curve, INT CurveIndex, FCDConversionFunctor* Conversion)
{
	if (ColladaDocument == NULL) return;
	if (ColladaAnimated == NULL || ColladaAnimatedIndex >= (INT) ColladaAnimated->GetValueCount()) return;
	if (Curve == NULL || CurveIndex >= 3) return;

	// Create the animation entity for this curve and the animation curve.
	fstring BaseName = fstring(Name) + TO_FSTRING(ColladaAnimated->GetQualifier(ColladaAnimatedIndex));
	FCDAnimation* ColladaAnimation = ColladaDocument->GetAnimationLibrary()->AddEntity();
	ColladaAnimation->SetDaeId(TO_STRING(BaseName));
	FCDAnimationChannel* ColladaChannel = ColladaAnimation->AddChannel();
	FCDAnimationCurve* ColladaCurve = ColladaChannel->AddCurve();
	ColladaAnimated->AddCurve(ColladaAnimatedIndex, ColladaCurve);

	// Write out the key times from the UE3 curve to the COLLADA curve.
	INT KeyCount = Curve->Points.Num();
	FloatList& KeyTimes = ColladaCurve->GetKeys();
	KeyTimes.reserve(KeyCount);
	for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
	{
		FInterpCurvePoint<FVector>& Key = Curve->Points(KeyIndex);

		// The Unreal engine allows you to place more than one key at one time value:
		// displace the extra keys. This assumes that Unreal's keys are always ordered.
		FLOAT KeyTime = Key.InVal;
		if (!KeyTimes.empty() && KeyTime < KeyTimes.back() + FLT_TOLERANCE)
		{
			KeyTime = KeyTimes.back() + 0.01f; // Add 1 millisecond to the timing of this key.
		}
		KeyTimes.push_back(KeyTime);
	}

	// Write out the key values from the UE3 curve to the COLLADA curve.
	for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
	{
		FInterpCurvePoint<FVector>& Key = Curve->Points(KeyIndex);
		FLOAT KeyTime = KeyTimes[KeyIndex];

		FLOAT OutValue = (CurveIndex == 0) ? Key.OutVal.X : (CurveIndex == 1) ? Key.OutVal.Y : Key.OutVal.Z;
		ColladaCurve->GetKeyValues().push_back(OutValue);
		ColladaCurve->GetInterpolations().push_back(FUDaeInterpolation::BEZIER);

		// Calculate the in/out control points and write them out to the COLLADA curve.
		FLOAT InTangentValue = (CurveIndex == 0) ? Key.ArriveTangent.X : (CurveIndex == 1) ? Key.ArriveTangent.Y : Key.ArriveTangent.Z;
		FLOAT OutTangentValue = (CurveIndex == 0) ? Key.LeaveTangent.X : (CurveIndex == 1) ? Key.LeaveTangent.Y : Key.LeaveTangent.Z;

		// Calculate the time values for the control points.
		FLOAT InTangentX = (KeyIndex > 0) ? (2.0f * KeyTime + KeyTimes[KeyIndex - 1]) / 3.0f : (KeyTime - 0.333f);
		FLOAT OutTangentX = (KeyIndex < KeyCount - 1) ? (KeyTimes[KeyIndex + 1] + 2.0f * KeyTime) / 3.0f : (KeyTime + 0.333f);
		if (IsEquivalent(OutTangentX, KeyTime))
		{
			OutTangentX = KeyTime + 0.00333f; // 1/3rd of a millisecond.
		}

		// Convert the tangent values from Hermite to Bezier control points.
		FLOAT InTangentY = OutValue - InTangentValue / 3.0f;
		FLOAT OutTangentY = OutValue + OutTangentValue / 3.0f;
		ColladaCurve->GetInTangents().push_back(FMVector2(InTangentX, InTangentY));
		ColladaCurve->GetOutTangents().push_back(FMVector2(OutTangentX, OutTangentY));
	}

	// Convert the values. This is useful for inversions and negative scales.
	ColladaCurve->ConvertValues(Conversion, Conversion);
}

void CExporter::ExportAnimatedFloat(const TCHAR* Name, FCDAnimated* ColladaAnimated, FInterpCurveFloat* Curve, FCDConversionFunctor* Conversion)
{
	if (ColladaAnimated == NULL || Curve == NULL || ColladaDocument == NULL) return;

	// Create the animation entity for this curve and the animation curve.
	FCDAnimation* ColladaAnimation = ColladaDocument->GetAnimationLibrary()->AddEntity();
	ColladaAnimation->SetDaeId(TO_STRING((const fchar*) Name));
	FCDAnimationChannel* ColladaChannel = ColladaAnimation->AddChannel();
	FCDAnimationCurve* ColladaCurve = ColladaChannel->AddCurve();
	ColladaAnimated->AddCurve(0, ColladaCurve);

	// Write out the key times from the UE3 curve to the COLLADA curve.
	INT KeyCount = Curve->Points.Num();
	FloatList& KeyTimes = ColladaCurve->GetKeys();
	KeyTimes.reserve(KeyCount);
	for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
	{
		FInterpCurvePoint<FLOAT>& Key = Curve->Points(KeyIndex);

		// The Unreal engine allows you to place more than one key at one time value:
		// displace the extra keys. This assumes that Unreal's keys are always ordered.
		FLOAT KeyTime = Key.InVal;
		if (!KeyTimes.empty() && KeyTime < KeyTimes.back() + FLT_TOLERANCE)
		{
			KeyTime = KeyTimes.back() + 0.01f; // Add 1 millisecond to the timing of this key.
		}
		KeyTimes.push_back(KeyTime);
	}

	// Write out the key values from the UE3 curve to the COLLADA curve.
	for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
	{
		FInterpCurvePoint<FLOAT>& Key = Curve->Points(KeyIndex);
		FLOAT KeyTime = KeyTimes[KeyIndex];
		ColladaCurve->GetKeyValues().push_back(Key.OutVal);

		// Write out the correct interpolation type.
		FUDaeInterpolation::Interpolation KeyType;
		switch (Key.InterpMode)
		{
		case CIM_Constant: 
			KeyType = FUDaeInterpolation::STEP;
			break;
		case CIM_Linear: 
			KeyType = FUDaeInterpolation::LINEAR;
			break;
		default: 
			KeyType = FUDaeInterpolation::BEZIER;
		}
		ColladaCurve->GetInterpolations().push_back(KeyType);

		// Calculate the time values for the control points.
		FLOAT InTangentX = (KeyIndex > 0) ? (2.0f * KeyTime + KeyTimes[KeyIndex - 1]) / 3.0f : (KeyTime - 0.333f);
		FLOAT OutTangentX = (KeyIndex < KeyCount - 1) ? (KeyTimes[KeyIndex + 1] + 2.0f * KeyTime) / 3.0f : (KeyTime + 0.333f);

		// Convert the tangent values from Hermite to Bezier control points.
		FLOAT InTangentY = Key.OutVal - Key.ArriveTangent / 3.0f;
		FLOAT OutTangentY = Key.OutVal + Key.LeaveTangent / 3.0f;
		ColladaCurve->GetInTangents().push_back(FMVector2(InTangentX, InTangentY));
		ColladaCurve->GetOutTangents().push_back(FMVector2(OutTangentX, OutTangentY));
	}

	// Convert the values. This is useful for inversions and negative scales.
	ColladaCurve->ConvertValues(Conversion, Conversion);
}

/**
 * Finds the given UE3 actor in the already-exported list of structures
 */
CExporterActor* CExporter::FindActor(AActor* Actor)
{
	INT Count = ColladaActors.Num();
	for (INT Idx = 0; Idx < Count; ++Idx)
	{
		CExporterActor* ColladaActor = ColladaActors(Idx);
		if (ColladaActor->Actor == Actor)
		{
			return ColladaActor;
		}
	}
	return NULL;
}

};

