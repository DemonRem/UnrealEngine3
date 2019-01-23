/**
 * Contains the shared data that is used by all SkeletalMeshComponents (instances).
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SkeletalMesh extends Object
	native
	noexport
	hidecategories(Object);

var		const native			BoxSphereBounds			Bounds;

/** List of materials applied to this mesh. */
var()	const native			array<MaterialInterface>	Materials;
/** Origin in original coordinate system */
var()	const native			vector					Origin;
/** Amount to rotate when importing (mostly for yawing) */
var()	const native			rotator					RotOrigin;

var		const native			array<int>				RefSkeleton;	// FMeshBone
var		const native			int						SkeletalDepth;
var		const native			map{FName,INT}			NameIndexMap;

var		const native private	IndirectArray_Mirror	LODModels;		// FStaticLODModel
var		const native			array<matrix>			RefBasesInvMatrix;

struct native BoneMirrorInfo
{
	/** The bone to mirror. */
	var()	int		SourceIndex;
	/** Axis the bone is mirrored across. */
	var()	EAxis	BoneFlipAxis;
};

/** List of bones that should be mirrored. */
var()	editfixedsize	array<BoneMirrorInfo>	SkelMirrorTable;
var()	EAxis									SkelMirrorAxis;
var()	EAxis									SkelMirrorFlipAxis;

var		array<SkeletalMeshSocket>		Sockets;

/** Struct containing information for a particular LOD level, such as materials and info for when to use it. */
struct native SkeletalMeshLODInfo
{
	/**	Indicates when to use this LOD. A smaller number means use this LOD when further away. */
	var()	float						DisplayFactor;
	/**	Used to avoid 'flickering' when on LOD boundary. Only taken into account when moving from complex->simple. */
	var()	float						LODHysteresis;
	/** Mapping table from this LOD's materials to the SkeletalMesh materials array. */
	var()	editfixedsize array<INT>	LODMaterialMap;
	/** Per-section control over whether to enable shadow casting. */
	var()	editfixedsize array<bool>	bEnableShadowCasting;
};

/** Struct containing information for each LOD level, such as materials to use, whether to cast shadows, and when use the LOD. */
var()	editfixedsize array<SkeletalMeshLODInfo>	LODInfo;

/** For each bone specified here, all triangles rigidly weighted to that bone are entered into a kDOP, allowing per-poly collision checks. */
var()	array<name>	PerPolyCollisionBones;

/** 
 *	KDOP tree's used for storing rigid triangle information for a subset of bones. 
 *	Length of this array matches PerPolyCollisionBones
 */
var		private const native array<int> PerPolyBoneKDOPs;

/** If true, use PhysicsAsset for line collision checks. If false, use per-poly bone collision (if present). */
var()	bool		bUseSimpleLineCollision;

/** If true, use PhysicsAsset for extent (swept box) collision checks. If false, use per-poly bone collision (if present). */
var()	bool		bUseSimpleBoxCollision;

/** All meshes default to GPU skinning. Set to True to enable CPU skinning */
var()	const bool	bForceCPUSkinning;

/** The FaceFX asset the skeletal mesh uses for FaceFX operations. */
var() FaceFXAsset FaceFXAsset;

/** Asset used for previewing bounds in AnimSetViewer. Makes setting up LOD distance factors more reliable. */
var()	editoronly PhysicsAsset		BoundsPreviewAsset;

/** 
 * Bone Name to start drawing bones from. 
 * See SkeletalMeshComponent.bDisplayBones. 
 */
var() Name StartDisplayBoneName;

/** LOD bias to use for PC. */
var() int LODBiasPC;
/** LOD bias to use for PS3. */
var() int LODBiasPS3;
/** LOD bias to use for Xbox 360. */
var() int LODBiasXbox360;

/** Cache of ClothMesh objects at different scales. */
var	const native transient array<pointer>	ClothMesh;

/** Scale of each of the ClothMesh objects in cache. This array is same size as ClothMesh. */
var const native transient array<float>		ClothMeshScale;

/** 
 *	Mapping between each vertex in the simulation mesh and the graphics mesh. 
 *	This is ordered so that 'free' vertices are first, and then after NumFreeClothVerts they are 'fixed' to the skinned mesh.
 */
var const array<int>		ClothToGraphicsVertMap;

/**
 * Mapping from index of rendered mesh to index of simulated mesh.
 * This mapping applies before ClothToGraphicsVertMap which can then operate normally
 * The reason for this mapping is to weld several vertices with the same position but different texture coordinates into one
 * simulated vertex which makes it possible to run closed meshes for cloth.
 */
var const array<int>		ClothWeldingMap;

/**
 * This is the highest value stored in ClothWeldingMap
 */
var const int				ClothWeldingDomain;

/**
 * This will hold the indices to the reduced number of cloth vertices used for cooking the NxClothMesh.
 */
var const array<int>		ClothWeldedIndices;

/**
 * Forces the Welding Code to be turned off even if the mesh has doubled vertices
 */
var(Cloth)	const bool		bForceNoWelding;

/** Point in the simulation cloth vertex array where the free verts finish and we start having 'fixed' verts. */
var const int				NumFreeClothVerts;

/** Index buffer for simulation cloth. */
var const array<int>		ClothIndexBuffer;

/** Vertices with any weight to these bones are considered 'cloth'. */
var(Cloth)	const array<name>		ClothBones;


/** Enable constraints that attempt to minimize curvature or folding of the cloth. */
var(Cloth)	const bool				bEnableClothBendConstraints;

/** Enable damping forces on the cloth. */
var(Cloth)	const bool				bEnableClothDamping;

/** Enable center of mass damping of cloth internal velocities.  */
var(Cloth)	const bool				bUseClothCOMDamping;

/** Controls strength of springs that attempts to keep particles in the cloth together. */
var(Cloth)	const float				ClothStretchStiffness;

/** 
 *	Controls strength of springs that stop the cloth from bending. 
 *	bEnableClothBendConstraints must be true to take affect. 
 */
var(Cloth)	const float				ClothBendStiffness;

/** 
 *	This is multiplied by the size of triangles sharing a point to calculate the points mass.
 *	This cannot be modified after the cloth has been created.
 */
var(Cloth)	const float				ClothDensity;

/** How thick the cloth is considered when doing collision detection. */
var(Cloth)	const float				ClothThickness;

/** 
 *	Controls how much damping force is applied to cloth particles.
 *	bEnableClothDamping must be true to take affect.
 */
var(Cloth)	const float				ClothDamping;

/** Increasing the number of solver iterations improves how accurately the cloth is simulated, but will also slow down simulation. */
var(Cloth)	const int				ClothIterations;

/** Controls movement of cloth when in contact with other bodies. */
var(Cloth)	const float				ClothFriction;

/** 
 * Controls the size of the grid cells a cloth is divided into when performing broadphase collision. 
 * The cell size is relative to the AABB of the cloth.
 */
var(Cloth)	const float				ClothRelativeGridSpacing;

/** Adjusts the internal "air" pressure of the cloth. Only has affect when bEnableClothPressure. */
var(Cloth)	const float				ClothPressure;

/** Response coefficient for cloth/rb collision */
var(Cloth)	const float				ClothCollisionResponseCoefficient;

/** How much an attachment to a rigid body influences the cloth */
var(Cloth)	const float				ClothAttachmentResponseCoefficient;

/** How much extension an attachment can undergo before it tears/breaks */
var(Cloth)	const float				ClothAttachmentTearFactor;

/**
 * Maximum linear velocity at which cloth can go to sleep.
 * If negative, the global default will be used.
 */
var(Cloth)	const float				ClothSleepLinearVelocity;



/** Enable orthogonal bending resistance to minimize curvature or folding of the cloth. 
 *  This technique uses angular springs instead of distance springs as used in 
 *  'bEnableClothBendConstraints'. This mode is slower but independent of stretching resistance.
 */
var(Cloth)	const bool				bEnableClothOrthoBendConstraints;

/** Enables cloth self collision. */
var(Cloth)	const bool				bEnableClothSelfCollision;

/** Enables pressure support. Simulates inflated objects like balloons. */
var(Cloth)	const bool				bEnableClothPressure;

/** Enables two way collision with rigid-bodies. */
var(Cloth)	const bool				bEnableClothTwoWayCollision;

/** Cloth bone type, used when attaching to the physics asset. */
enum ClothBoneType
{
	CLOTHBONE_Fixed,						//0
	CLOTHBONE_BreakableAttachment,			//1
	/* EXPERIMENTAL: CLOTHBONE_TearLine */
};

/** Used to specify a set of special cloth bones which are attached to the physics asset */
struct native ClothSpecialBoneInfo
{
	/** The bone name to attach to a cloth vertex */
	var() name BoneName;
	
	/** The type of attachment */
	var() ClothBoneType BoneType;
	
	/** Array used to cache cloth indices which will be attached to this bone, created in BuildClothMapping(),
	 * Note: These are welded indices.
	 */
	var const array<int> AttachedVertexIndices;
};

/** 
 * Vertices with any weight to these bones are considered cloth with special behavoir, currently
 * they are attached to the physics asset with fixed or breakable attachments.
 */
var(Cloth)	const array<ClothSpecialBoneInfo>		ClothSpecialBones; 

/** 
 * Enable cloth line/extent/point checks. 
 * Note: line checks are performed with a raycast against the cloth, but point and swept extent checks are performed against the cloth AABB 
 */
var(Cloth)	const bool				bEnableClothLineChecks;

/**
 *  Whether cloth simulation should be wrapped inside a Rigid Body and only be used upon impact
 */
var(Cloth)  const bool		bClothMetal;

/** Threshold for when deformation is allowed */
var(Cloth)	const float				ClothMetalImpulseThreshold;
/** Amount by which colliding objects are brought closer to the cloth */
var(Cloth)	const float				ClothMetalPenetrationDepth;
/** Maximum deviation of cloth particles from initial position */
var(Cloth)	const float				ClothMetalMaxDeformationDistance;

/** 
 *  Used to enable cloth tearing. Note, extra vertices/indices must be reserved using ClothTearReserve 
 *  Also cloth tearing is not available when welding is enabled.
 */
var(Cloth)	const bool				bEnableClothTearing;

/** Stretch factor beyond which a cloth edge/vertex will tear. Should be greater than 1. */
var(Cloth)	const float				ClothTearFactor;

/** Number of vertices/indices to set aside to accomodate new triangles created as a result of tearing */
var(Cloth)	const int				ClothTearReserve;

/** Map which maps from a set of 3 triangle indices packet in a 64bit to the location in the index buffer,
 *  Used to update indices for torn triangles.
 *  Note: This structure is lazy initialized when a torn cloth mesh is created. (But could be precomputed
 *  in BuildClothMapping() if serialization is handled correctly).
 */
var			const native Map_Mirror ClothTornTriMap {TMap<QWORD,INT>};

var const native transient int ReleaseResourcesFence;

defaultproperties
{
	SkelMirrorAxis=AXIS_X
	SkelMirrorFlipAxis=AXIS_Z

	// Cloth params
	ClothThickness = 0.5f;
	ClothDensity = 1.0f;
	ClothBendStiffness = 1.0f;
	ClothStretchStiffness = 1.0f;
    ClothDamping = 0.5f;
	ClothFriction = 0.5f;
    ClothIterations = 5;

	bUseSimpleLineCollision=true
	bUseSimpleBoxCollision=true

	bEnableClothOrthoBendConstraints = FALSE
	bEnableClothSelfCollision = FALSE
	bEnableClothPressure = FALSE
	bEnableClothTwoWayCollision = FALSE
	bForceNoWelding = FALSE
	
	ClothRelativeGridSpacing = 1.0
	ClothPressure = 1.0
	ClothCollisionResponseCoefficient = 0.2
	ClothAttachmentResponseCoefficient = 0.2
	ClothAttachmentTearFactor = 1.5
	ClothSleepLinearVelocity =-1.0

	ClothMetalImpulseThreshold=10.0
	ClothMetalPenetrationDepth=0.0
	ClothMetalMaxDeformationDistance=0.0

	bEnableClothTearing = FALSE
	ClothTearFactor = 3.5
	ClothTearReserve = 128
}
