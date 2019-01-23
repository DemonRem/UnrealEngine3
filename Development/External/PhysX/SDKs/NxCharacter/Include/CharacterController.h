#ifndef NX_CHARACTER_CONTROLLER
#define NX_CHARACTER_CONTROLLER
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

//#define NEW_CALLBACKS_DESIGN
//#define USE_CONTACT_NORMAL_FOR_SLOPE_TEST
//#define GROUP_TRIANGLE_DATA

/* Exclude from documentation */
/** \cond */

enum TriangleCollisionFlag
	{
	// Must be the 3 first ones to be indexed by (flags & (1<<edge_index))
	TCF_ACTIVE_EDGE01	= (1<<0),
	TCF_ACTIVE_EDGE12	= (1<<1),
	TCF_ACTIVE_EDGE20	= (1<<2),
	TCF_DOUBLE_SIDED	= (1<<3),
//	TCF_WALKABLE		= (1<<4),
	};

#ifdef GROUP_TRIANGLE_DATA

	struct TriangleData
		{
			Triangle			mTri;
			Triangle			mEdges;
			udword				mEdgeFlags;
			udword				mOriginalIndex;
		};
	const udword TriangleDataSize = sizeof(TriangleData)/sizeof(udword);

		// Customized container for triangle data
	class TriangleCollisionData : private Container
		{
			public:
			// Constructor / Destructor
										TriangleCollisionData()		{}
										~TriangleCollisionData()	{}

			inline_		void			ForceSize(udword i)			{ Container::ForceSize(i);								}
			inline_		void			Reset()						{ Container::Reset();									}
			inline_		const udword*	GetBase()			const	{ return GetEntries();									}
			inline_		udword			GetCurrentIndex()	const	{ return GetNbEntries();								}
			inline_		udword			GetNbTriangles()	const	{ return GetNbEntries()/TriangleDataSize;				}
			inline_		TriangleData*	GetTriangleData()	const	{ return (TriangleData*)GetEntries();					}
			inline_		TriangleData*	ReserveData(udword nb)		{ return (TriangleData*)Reserve(nb*TriangleDataSize);	}
		};

/*
class TriangleMeshShape;

struct triangleData
	{
		NxVec3				verts[3];	// triangle vertices in world space
		TriangleMeshShape*	owner;		// mesh owner
		NxU32				edgeFlags;	// misc edge flags
		NxU32				index;		// triangle index in original database
	};
const NxU32 triangleDataSize = sizeof(triangleData)/sizeof(NxU32);

	// Customized container for triangle data
class triangleCollisionData : private Container
	{
		public:
		// Constructor / Destructor
									triangleCollisionData()		{}
									~triangleCollisionData()	{}

		NX_INLINE	void			reset()						{ Reset();												}
		NX_INLINE	udword			getNbTriangles()	const	{ return GetNbEntries()/triangleDataSize;				}
		NX_INLINE	triangleData*	getTriangleData()	const	{ return (triangleData*)GetEntries();					}
		NX_INLINE	triangleData*	reserve(NxU32 nb)			{ return (triangleData*)Reserve(nb*triangleDataSize);	}
	};
*/
#endif

	enum TouchedGeomType
	{
		TOUCHED_USER_BOX,
		TOUCHED_USER_CAPSULE,
		TOUCHED_MESH,
		TOUCHED_BOX,
		TOUCHED_SPHERE,
		TOUCHED_CAPSULE,

		TOUCHED_LAST,

		TOUCHED_FORCE_DWORD	= 0x7fffffff
	};

	class SweepTest;

// PT: apparently stupid .Net aligns some of them on 8-bytes boundaries for no good reason. This is bad.
#pragma pack(4)

	struct TouchedGeom
	{
		TouchedGeomType	mType;
		void*			mUserData;	// NxController or NxShape pointer
		BigPoint		mOffset;	// Local origin, typically the center of the world bounds around the character. We translate both
									// touched shapes & the character so that they are nearby this point, then add the offset back to
									// computed "world" impacts.
	};

	struct TouchedUserBox : public TouchedGeom
	{
		PruningAABB		mBox;
	};
	ICE_COMPILE_TIME_ASSERT(sizeof(TouchedUserBox)==sizeof(TouchedGeom)+sizeof(PruningAABB));

	struct TouchedUserCapsule : public TouchedGeom
	{
		PruningLSS		mCapsule;
	};
	ICE_COMPILE_TIME_ASSERT(sizeof(TouchedUserCapsule)==sizeof(TouchedGeom)+sizeof(PruningLSS));

	struct TouchedMesh : public TouchedGeom
	{
		udword			mNbTris;
#ifdef GROUP_TRIANGLE_DATA
		udword			mIndexTriangleData;
#else
		udword			mIndexWorldTriangles;
		udword			mIndexWorldEdgeNormals;
		udword			mIndexEdgeFlags;
#endif
	};

	struct TouchedBox : public TouchedGeom
	{
		OBB				mBox;
	};
	ICE_COMPILE_TIME_ASSERT(sizeof(TouchedBox)==sizeof(TouchedGeom)+sizeof(OBB));

	struct TouchedSphere : public TouchedGeom
	{
		Sphere			mSphere;
	};
	ICE_COMPILE_TIME_ASSERT(sizeof(TouchedSphere)==sizeof(TouchedGeom)+sizeof(Sphere));

	struct TouchedCapsule : public TouchedGeom
	{
		LSS				mCapsule;
	};
	ICE_COMPILE_TIME_ASSERT(sizeof(TouchedCapsule)==sizeof(TouchedGeom)+sizeof(LSS));

#pragma pack()

	enum SweptContactType
	{
		SWEPT_CTC_SHAPE,		// We touched another shape
		SWEPT_CTC_CONTROLLER,	// We touched another controller
	};

	struct SweptContact
	{
		BigPoint			mWorldPos;		// Contact position in world space
		Point				mWorldNormal;	// Contact normal in world space
		float				mDistance;		// Contact distance
		udword				mIndex;			// Feature identifier (e.g. triangle index for meshes)
		TouchedGeom*		mGeom;
	};

	enum SweptVolumeType
	{
		SWEPT_BOX,
		SWEPT_CAPSULE,

		SWEPT_LAST
	};

	class SweptVolume
	{
		public:
			SweptVolume();
			~SweptVolume();

		virtual void ComputeTemporalBox(const SweepTest&, PruningAABB& box, const Point& direction)							const	= 0;
		virtual void ComputeTemporalBox(const SweepTest&, PruningAABB& box, const BigPoint& center, const Point& direction)	const	= 0;

		inline_	SweptVolumeType	GetType()	const	{ return mType;	}

		BigPoint		mCenter;
		float			mHalfHeight;	// UBI
		protected:
		SweptVolumeType	mType;
	};

	class SweptBox : public SweptVolume
	{
		public:
			SweptBox();
			~SweptBox();

		virtual void ComputeTemporalBox(const SweepTest&, PruningAABB& box, const Point& direction)							const;
		virtual void ComputeTemporalBox(const SweepTest&, PruningAABB& box, const BigPoint& center, const Point& direction)	const;

		Point	mExtents;
	};

	class SweptCapsule : public SweptVolume
	{
		public:
						SweptCapsule();
						~SweptCapsule();

		virtual void	ComputeTemporalBox(const SweepTest&, PruningAABB& box, const Point& direction)							const;
		virtual void	ComputeTemporalBox(const SweepTest&, PruningAABB& box, const BigPoint& center, const Point& direction)	const;

		inline_	void	GetCapsule(const BigPoint& center, PruningLSS& capsule, udword axis) const
						{
							capsule.mRadius		= mRadius;
							capsule.mP0			= center;
							capsule.mP1			= center;
							capsule.mP0[axis]	+= mHeight*0.5f;
							capsule.mP1[axis]	-= mHeight*0.5f;
						}
		inline_	void	GetLocalCapsule(LSS& capsule, udword axis) const
						{
							capsule.mRadius		= mRadius;
							capsule.mP0.Zero();
							capsule.mP1.Zero();
							capsule.mP0[axis]	+= mHeight*0.5f;
							capsule.mP1[axis]	-= mHeight*0.5f;
						}

		float	mRadius;
		float	mHeight;
	};

#ifdef NEW_CALLBACKS_DESIGN
	class CCT_HitReport;
#endif
	class NxGroupsMask;

	class SweepTest
	{
		public:
							SweepTest();
							~SweepTest();

				void		MoveCharacter(
							void* user_data,
#ifdef NEW_CALLBACKS_DESIGN
							CCT_HitReport* report,
#else
							void* user_data2,
#endif
							SweptVolume& volume,
							const Point& direction,
							udword nb_boxes, const PruningAABB* boxes, const void** box_user_data,
							udword nb_capsules, const PruningLSS* capsules, const void** capsule_user_data,
							udword groups, float min_dist,
							udword& collision_flags,
							const NxGroupsMask* groupsMask,
							bool apply_ubi_fix
							);

				bool		DoSweepTest(
								void* user_data,
#ifdef NEW_CALLBACKS_DESIGN
							CCT_HitReport* report,
#else
								void* user_data2,
#endif
								udword nb_boxes, const PruningAABB* boxes, const void** box_user_data,
								udword nb_capsules, const PruningLSS* capsules, const void** capsule_user_data,
								SweptVolume& swept_volume,
								const Point& direction, float bump, float friction, udword max_iter,
								udword* nb_collisions, udword group_flags, float min_dist, const NxGroupsMask* groupsMask, bool down_pass=false);

				void		FindTouchedCCTs(
									udword nb_boxes, const PruningAABB* boxes, const void** box_user_data,
									udword nb_capsules, const PruningLSS* capsules, const void** capsule_user_data,
									const PruningAABB& world_box
									);

				void		VoidTestCache() { mCachedTBV.SetEmpty(); }

//		private:
#ifdef GROUP_TRIANGLE_DATA
	TriangleCollisionData	mTriangleData;
#else
				TriList		mWorldTriangles;
				TriList		mWorldEdgeNormals;
				Container	mEdgeFlags;
#endif
				Container	mGeoms;
				PruningAABB	mCachedTBV;
				udword		mNbCachedStatic;
#ifdef GROUP_TRIANGLE_DATA
				udword		mNbCachedTris;
#else
				udword		mNbCachedT;
				udword		mNbCachedEN;
				udword		mNbCachedF;
#endif
		public:
#ifdef USE_CONTACT_NORMAL_FOR_SLOPE_TEST
				Point		mCN;
#else
				Triangle	mTouched;
#endif
				float		mSlopeLimit;
				float		mSkinWidth;
				float		mStepOffset;
				float		mVolumeGrowth;	// Must be >1.0f and not too big
				float		mContactPointHeight;	// UBI
				udword		mUpDirection;
				udword		mMaxIter;
				bool		mHitNonWalkable;
				bool		mWalkExperiment;
				bool		mHandleSlope;
				bool		mValidTri;
				bool		mValidateCallback;
				bool		mNormalizeResponse;
		private:
				void		UpdateTouchedGeoms(void* user_data,
												udword nb_boxes, const PruningAABB* boxes, const void** box_user_data,
												udword nb_capsules, const PruningLSS* capsules, const void** capsule_user_data,
												udword group_flags, const PruningAABB& world_box, bool first_iter, const NxGroupsMask* groupsMask);
	};

bool FindTouchedGeometry(
	void* user_data,
	const PruningAABB& world_aabb,

#ifdef GROUP_TRIANGLE_DATA
	TriangleCollisionData& triangles_data,
#else
	TriList& world_triangles,
	TriList& world_edge_normals,
	Container& edge_flags,
#endif
	Container& geoms,
	udword group_flags,
	bool static_shapes, bool dynamic_shapes, const NxGroupsMask* groupsMask);

#ifdef NEW_CALLBACKS_DESIGN
	class CCT_HitReport
	{
		public:
						CCT_HitReport()		{}
		virtual			~CCT_HitReport()	{}

		virtual	void	ShapeHitCallback(const SweptContact& contact, const Point& dir, float length)	= 0;
		virtual	void	UserHitCallback(const SweptContact& contact, const Point& dir, float length)	= 0;
	};
#else
	void ShapeHitCallback(void* user_data2, const SweptContact& contact, const Point& dir, float length);
	void UserHitCallback(void* user_data2, const SweptContact& contact, const Point& dir, float length);
#endif

	// On the NX side:

	// User-defined structure passed to callbacks
	class NxScene;
	class NxActor;
	class NxShape;
	struct CallbackData
	{
		NxScene*	scene;
		NxShape*	shape;
		NxActor*	actor;
	};

/** \endcond */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
