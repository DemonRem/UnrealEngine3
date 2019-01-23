#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#pragma warning(disable:4996) // Disabling stupid .NET deprecated warning.


#include <NxPhysics.h>
#include "NXU_SchemaStream.h"
#include "NXU_schema.h"
#include "NXU_File.h"
#include "NXU_string.h"
#include "NXU_Asc2Bin.h"

namespace NXU
{

#define DEBUG_WARNING 0
#define OLD_FORMAT 1   // true if enabling backwards compatibility to earlier release of NxuStream2

bool gSaveDefaults=true;

#if OLD_FORMAT
static bool isFlag(const char *f)
{
  bool ret = false;

  if ( strcmp(f,"flags") == 0 ||
       strcmp(f,"mFlags") == 0 ||
       strcmp(f,"mFlag") == 0 ||
       strcmp(f,"wheelFlags") == 0 ||
       strcmp(f,"shapeFlags") == 0 ||
       strcmp(f,"meshFlags") == 0 ||
       strcmp(f,"jointFlags") == 0 )
  {
    ret = true;
  }

  return ret;
}

static bool isJoint(SCHEMA_CLASS c)
{
  bool ret = false;


  if (  c == SC_NxD6JointDesc ||
        c == SC_NxCylindricalJointDesc ||
        c == SC_NxDistanceJointDesc ||
        c == SC_NxFixedJointDesc ||
        c == SC_NxPointInPlaneJointDesc ||
        c == SC_NxPointOnLineJointDesc ||
        c == SC_NxPrismaticJointDesc ||
        c == SC_NxRevoluteJointDesc ||
        c == SC_NxSphericalJointDesc ||
        c == SC_NxPulleyJointDesc )
  {
    ret = true;
  }

  return ret;
}
#endif

static TiXmlNode	*getElement(const	char *name,	const	char *id,TiXmlNode *node_begin)
{
	assert(name);

	if (name ==	0)
	{
		return 0;
	}

	TiXmlNode	*ret = 0;

	TiXmlNode	*node	=	node_begin->FirstChild();

	while	(node	&& !ret)
	{
		if (node->Type() ==	TiXmlNode::ELEMENT)
		{
			if (_stricmp(node->Value(), name) ==	0)
			{
				if (id)
				{
					TiXmlElement *element	=	node->ToElement();
					TiXmlAttribute *atr	=	element->FirstAttribute();
					if (atr)
					{
						const	char *aname	=	atr->Name();
						const	char *avalue = atr->Value();
						if (_stricmp(avalue,	id)	== 0)
						{
							ret	=	node;
							break;
						}
					}

				}
				else
				{
					ret	=	node;
					break;
				}
			}
		}

		if (node->NoChildren())
		{
			assert(node);
			while	(node->NextSibling() ==	NULL &&	node !=	node_begin )
			{
				node = node->Parent();
			}
			if (node ==	node_begin)
			{
				break;
			}
			assert(node);
			node = node->NextSibling();
		}
		else
		{
			assert(node);
			node = node->FirstChild();
		}

	}

	return ret;

}

static const	char *getElementText(const char	*name,bool &foundNode,TiXmlNode *begin_node)
{
	const	char *ret	=	0;

	TiXmlNode	*node	=	getElement(name,0,begin_node);
	if (node)
	{
    foundNode = true;
		TiXmlNode	*child = node->FirstChild();
		if (child	&& child->Type() ==	TiXmlNode::TEXT)
		{
			ret	=	child->Value();
		}
	}
  else
  {
    foundNode = false;
  }

	return ret;
}

class SchemaClass
{
public:

  SchemaClass(SCHEMA_CLASS ctype,TiXmlNode *node,int depth,SchemaClass *parent,int index)
  {
  	mClassType = ctype;
  	mNode      = node;
  	mDepth     = depth;
  	mCurrent   = node;
    mClassName = EnumToString(mClassType);
    mParent    = parent;
    mIndex     = index;
    mPrevious  = 0;
    mNext      = 0;
  }

  void setPrevious(SchemaClass *prev)
  {
    mPrevious = prev;
  }

  void setNext(SchemaClass *next)
  {
    mNext = next;
  }

	const char * locateAttribute(const char *txt)
	{
		const char *ret = 0;
		TiXmlElement *e = mNode->ToElement();
		if ( e )
		{
  		TiXmlAttribute *atr	=	e->FirstAttribute();
  		while	(atr)
  		{
  			const	char *aname	=	atr->Name();
  			const	char *avalue = atr->Value();
  			if ( strcmp(txt,aname) == 0 )
  			{
  				ret = avalue;
  				break;
  			}
  			atr	=	atr->Next();
  		}
		}
		return ret;
	}

#if OLD_FORMAT
  // Used to maintain backwards compatibility with the 2.6.0 release of NxuStream.
  // Just remapping a few fields based on some minor name cleanup.
  const char * getRemapText(const char *txt,bool &ignore)
  {
  	const char *ret = txt;
  	ignore = false;

		if ( isJoint(mClassType) )
		{
			if ( strcmp(txt,"onActor2") == 0 )
			{
				ret = "mOnActor2";
			}
			else if ( strcmp(txt,"planeLimitPoint") == 0 )
			{
				ret = "mPlaneLimitPoint";
			}
		}

		switch ( mClassType )
		{
			case SC_NxShapeFlag:
        if ( strcmp(txt,"NX_SF_DISABLE_SCENE_QUERIES") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxBodyFlag:
				if ( strcmp(txt,"NX_BF_ENERGY_SLEEP_TEST") == 0 )
				{
					ignore = true;
				}
				else if ( strcmp(txt,"NX_BF_VISUALIZATION") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxBodyDesc:
				if ( strcmp(txt,"sleepDamping") == 0 )
				{
					ignore = true;
				}
				else if ( strcmp(txt,"sleepEnergyThreshold") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxSceneDesc:
				if ( strcmp(txt,"hasFilter") == 0 )
				{
					ignore = true;
				}
				else if ( strcmp(txt,"maxBounds") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxActorDesc:
				if ( strcmp(txt,"compartment") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxPairFlagDesc:
				if ( strcmp(txt,"actor0") == 0 )
				{
					ret = "mActor0";
				}
				else if ( strcmp(txt,"actor1") == 0 )
				{
					ret = "mActor1";
				}
				else if ( strcmp(txt,"shapeIndex0") == 0 )
				{
					ret = "mShapeIndex0";
				}
				else if ( strcmp(txt,"shapeIndex1") == 0 )
				{
					ret = "mShapeIndex1";
				}
				else if ( strcmp(txt,"isActorPair") == 0 )
				{
					ignore = true;
				}
				break;
			case SC_NxShapeDesc:
				if ( strcmp(txt,"CCDSkeleton") == 0 )
				{
					ret = "mCCDSkeleton";
				}
				break;
			case SC_NxMotorDesc:
				if ( strcmp(txt,"maxForce") == 0 )
				{
					ret = "motorMaxForce";
				}
				break;
			case SC_NxMaterialDesc:
				if ( strcmp(txt,"materialIndex") == 0 )
				{
					ret = "index";
				}
				break;
			case SC_NxTriangleMeshDesc:
			case SC_NxConvexMeshDesc:
				if ( strcmp(txt,"cookedData") == 0  )
				{
					ret = "mCookData";
				}
				else if ( strcmp(txt,"cookedDataSize") == 0 )
				{
					ret = "mCookedDataSize";
				}
				break;
		}

    return ret;
  }
#endif
	const char * locate(const char *txt,bool oldFormat)
	{
		bool ignore = false;
#if OLD_FORMAT
		if ( oldFormat )
			txt = getRemapText(txt,ignore);
#endif
		const char *ret = locateAttribute(txt);
		if ( !ret )
		{
      bool foundNode;
			ret = getElementText(txt,foundNode,mNode);
			if ( !foundNode)
			{
				if ( !ignore )
				{
#if DEBUG_WARNING
  				reportWarning("Failed to locate item node(%-30s) '%s'",mNode->Value(), txt );
#endif
				}
			}
		}
		return ret;
	}


	int          mDepth;
  TiXmlNode 	*mNode;
  TiXmlNode	  *mCurrent;            // current scan location.
  SCHEMA_CLASS mClassType;  // the class type.
  const char * mClassName;
  SchemaClass *mParent;
  int          mIndex;
  SchemaClass *mNext;          // next one of this type.
  SchemaClass *mPrevious;      // previous one of this type.
};

typedef NxArray< SchemaClass * > SchemaClassVector;

class SchemaStack
{
public:
	int	mCurrent;
	SchemaClass *mSearch;
};

class SchemaXML
{
public:
  SchemaXML(const char *fname,void *mem,int len,SchemaStream *ss)
  {
  	mStream = ss;
	  mDocument = new TiXmlDocument;
	  mSearch = 0;
	  mOldFormat = false;
	  mIsValid = false;
		mFph = fopen("schemalog.txt", "wb");

    for (int i=0; i<SC_LAST; i++)
    {
      mSearchLocations[i] = 0;
    }

	  if (mDocument->LoadFile(fname,mem,len))
	  {
		  mNode	=	mDocument;
		  preParse(mNode);
	  }
	  else
	  {
	    reportError("Error	parsing	file '%s'	as XML", fname);
			reportError("XML	parse	error(%d)	on Row:	%d Column: %d",	mDocument->ErrorId(), mDocument->ErrorRow(), mDocument->ErrorCol());
			reportError("XML	error	description: \"%s\"",	mDocument->ErrorDesc());
  		delete mDocument;
	  	mDocument = 0;
	  	mNode = 0;
    }
    mStackPtr = 0;

  }

  ~SchemaXML(void)
  {
    if ( mFph )
      fclose(mFph);
  	delete mDocument;
  	for (unsigned int i=0; i<mClasses.size(); i++)
  	{
  		SchemaClass *sc = mClasses[i];
  		delete sc;
  	}
  }

	bool isClass(const char *str,SCHEMA_CLASS &t,SchemaClass *last,TiXmlNode *node)
	{
		bool ret = false;

		if ( strcmp(str,"NXUSTREAM") == 0 )
		{
#if OLD_FORMAT
			t = SC_NXUSTREAM2;
			mOldFormat = true;
			ret = true;
#endif
		}
		else if ( strcmp(str,"NXUSTREAM2") == 0 )
		{
			t = SC_NXUSTREAM2;
			mOldFormat = false;
			ret = true;
		}
		else
		{
#if OLD_FORMAT
      if ( mOldFormat ) // translate old format tags to the new syntax
      {
      	if ( strcmp(str,"NxPhysicsSDK") == 0 )
      	{
      		t = SC_NxuPhysicsCollection;
					ret = true;
      	}
        else if ( strcmp(str,"NxSceneInstance") == 0 )
        {
          t = SC_NxSceneInstanceDesc;
          ret = true;
        }
      	else if ( strcmp(str,"NxJointDesc") == 0 )
      	{
      		// if it is a joint desc, we need to find out what type!
      		bool foundNode;
					const char *type = getElementText("type",foundNode,node);
					assert(type);
					if ( type )
					{
            if ( strcmp(type,"NX_JOINT_D6") == 0 )
            {
              t = SC_NxD6JointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_PRISMATIC") == 0 )
            {
              t = SC_NxPrismaticJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_REVOLUTE") == 0 )
            {
              t = SC_NxRevoluteJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_CYLINDRICAL") == 0 )
            {
              t = SC_NxCylindricalJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_SPHERICAL") == 0 )
            {
              t = SC_NxSphericalJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_POINT_ON_LINE") == 0 )
            {
              t = SC_NxPointOnLineJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_POINT_IN_PLANE") == 0 )
            {
              t = SC_NxPointInPlaneJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_DISTANCE") == 0 )
            {
              t = SC_NxDistanceJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_PULLEY") == 0 )
            {
              t = SC_NxPulleyJointDesc;
              ret = true;
            }
            else if ( strcmp(type,"NX_JOINT_FIXED") == 0 )
            {
              t = SC_NxFixedJointDesc;
              ret = true;
            }
					}
      	}
      	else if ( strncmp(str,"NX_",3) == 0 && last && last->mClassType == SC_NxuPhysicsCollection )
      	{
      		t = SC_NxParameterDesc;
					ret = true;
      	}
				else if ( strcmp(str,"driveType") == 0 )
				{
					t = SC_NxD6JointDriveType;
					ret = true;
				}
        else if ( strcmp(str,"type") == 0 && last && last->mClassType == SC_NxFluidEmitterDesc )
        {
          t = SC_NxEmitterType;
					ret = true;
        }
        else if ( strcmp(str,"shape") == 0 && last && last->mClassType == SC_NxFluidEmitterDesc  )
        {
          t = SC_NxEmitterShape;
					ret = true;
        }
				else if ( strcmp(str,"groupsMask") == 0 )
				{
					t = SC_NxGroupsMask;
					ret = true;
				}
        else if ( strcmp(str,"collisionMethod") == 0 )
        {
          t = SC_NxFluidCollisionMethod;
          ret = true;
        }
        else if ( isFlag(str) && last )
        {

					SCHEMA_CLASS sc = last->mClassType;

					if ( sc == SC_NxJointFlag || sc == SC_NxD6JointFlag )
					{
						sc = last->mParent->mClassType;
					}

          ret = true;

          if ( strcmp(str,"jointFlags") == 0 )
          {
            sc = SC_NxJointDesc;
          }

          switch ( sc )
          {
            case SC_NxWheelShapeDesc:
							t = SC_NxWheelShapeFlags;
              break;
            case SC_NxClothMeshDesc:
							t = SC_NxMeshFlags;
              break;
            case SC_NxActorGroupPair:
							t = SC_NxContactPairFlag;
              break;
            case SC_NxCapsuleShapeDesc:
							t = SC_NxCapsuleShapeFlag;
              break;
            case SC_NxConvexShapeDesc:
							t = SC_NxMeshShapeFlag;
              break;
            case SC_NxTriangleMeshShapeDesc:
							t = SC_NxMeshShapeFlag;
              break;
            case SC_NxHeightFieldShapeDesc:
							t = SC_NxMeshShapeFlag;
              break;
            case SC_NxJointDesc:
							t = SC_NxJointFlag;
              break;
            case SC_NxD6JointDesc:
							t = SC_NxD6JointFlag;
              break;
            case SC_NxDistanceJointDesc:
              t = SC_NxDistanceJointFlag;
              break;
            case SC_NxPulleyJointDesc:
							t = SC_NxPulleyJointFlag;
              break;
            case SC_NxPairFlagDesc:
							t = SC_NxContactPairFlag;
              break;
            case SC_NxFluidEmitterDesc:
							t = SC_NxFluidEmitterFlag;
              break;
            case SC_NxFluidDesc:
							t = SC_NxFluidFlag;
              break;
            case SC_NxClothDesc:
							t = SC_NxClothFlag;
              break;
						case SC_NxCCDSkeletonDesc:
							t = SC_NxMeshFlags;
							break;
						case SC_NxConvexMeshDesc:
							t = SC_NxConvexFlags;
							break;
						case SC_NxTriangleMeshDesc:
							t = SC_NxMeshFlags;
							break;
            case SC_NxHeightFieldDesc:
              t = SC_NxMeshFlags;
              break;
            case SC_NxSceneDesc:
              t = SC_NxSceneFlags;
              break;
           case SC_NxMaterialDesc:
              t = SC_NxMaterialFlag;
              break;
            case SC_NxBodyDesc:
              t = SC_NxBodyFlag;
              break;
            case SC_NxActorDesc:
              t = SC_NxActorFlag;
              break;
						case SC_NxShapeDesc:
              if ( strcmp(str,"shapeFlags") == 0 )
                t = SC_NxShapeFlag;
              else
  							t = SC_NxCapsuleShapeFlag;
							break;
						case SC_NxSphericalJointDesc:
							t = SC_NxSphericalJointFlag;
							break;
						case SC_NxRevoluteJointDesc:
							t = SC_NxRevoluteJointFlag;
							break;
						default:
              ret = false;
						  assert(0);
						  break;
          }
        }
        else if ( strcmp(str,"jointFlags") == 0 )
        {
					t = SC_NxJointFlag;
					ret = true;
        }
        else if ( strcmp(str,"shapeFlags") == 0 )
        {
          t = SC_NxShapeFlag;
          ret = true;
        }
      }
#endif
			if ( !ret )
			{
				for (int i=0; i<SC_LAST; i++)
				{
					const char *c = EnumToString( (SCHEMA_CLASS) i );
					c+=3; // skip the enumeration prefix 'SC_'
					if ( strcmp(c,str) == 0 )
					{
						t = (SCHEMA_CLASS) i;
#if OLD_FORMAT
						if ( mOldFormat && isJoint(t) )
							ret = false; // skip joints if using old format
						else
#endif
  						ret = true;
						break;
					}
				}
			}
		}


    if ( ret )
    {
      SCHEMA_CLASS sc = SC_LAST;
      if ( last ) sc = last->mClassType;
#if DEBUG_WARNING
      reportWarning("ISCLASS: %-30s Node:%-30s  Last: %-30s Type: %s", EnumToString( (SCHEMA_CLASS) t), node->Value(), EnumToString( sc ), str );
#endif
    }
    else
    {
      SCHEMA_CLASS sc = SC_LAST;
      if ( last ) sc = last->mClassType;
#if DEBUG_WARNING
      reportWarning("NOCLASS: %-30s Node:%-30s  Last: %-30s Type: %s", "", node->Value(), EnumToString( sc ), str );
#endif
    }

		return ret;
	}

  SchemaClass * findParent(int depth)
  {
    SchemaClass *ret = 0;
    int match = depth-1; //
    if ( match >= 0 )
    {
      int count = mClasses.size();
      for (int i=count-1; i>=0; i--)
      {
        SchemaClass *c = mClasses[i];
        if ( c->mDepth  <= match )
        {
          ret = c;
          break;
        }
      }
    }
    return ret;
  }

  void preParse(TiXmlNode *begin)
  {

  	TiXmlNode	*node	=	begin;
  	int	depth	=	0;

    SchemaClass *nodes[SC_LAST];

    for (int i=0; i<SC_LAST; i++) nodes[i] = 0;

  	while	(node)
  	{
  		SCHEMA_CLASS t;

  		if ( node->Type() == TiXmlNode::ELEMENT )
		  {

        SchemaClass *last = findParent(depth);
        const char *parent = "";
        if ( last )
          parent = last->mClassName;

        if (isClass(node->Value(), t, last, node) )
        {
    			if ( t == SC_NXUSTREAM2 )
    				mIsValid = true;

          int index = mClasses.size();

    			SchemaClass *sc = new SchemaClass(t,node,depth,last,index);

          if ( mSearchLocations[t] == 0 )
          {
            mSearchLocations[t] = sc;
          }

          SchemaClass *prev = nodes[t];

          if ( prev )
          {
            prev->setNext(sc);
          }

          sc->setPrevious(prev);

					nodes[t] = sc;

          if ( mFph )
            fprintf(mFph,"%d) source(%s) is of type (%s) Depth(%d) Parent(%s) \r\n", mClasses.size(), node->Value(), sc->mClassName, depth, parent );

   				mClasses.push_back(sc);
        }
        else
        {
          if ( mFph )
          {
            fprintf(mFph,"     ## %s does not map to a known major class type depth(%d) parent(%s).\r\n", node->Value(), depth, parent );
          }
        }

			}

  		if (node->NoChildren())
  		{
  			while	(node->NextSibling() ==	NULL &&	node !=	mDocument)
  			{
  				depth--;
  				node = node->Parent();
  			}
  			if (node ==	mDocument)
  			{
  				break;
  			}
  			assert(node);
  			node = node->NextSibling();
  		}
  		else
  		{
  			depth++;
  			node = node->FirstChild();
  		}
  	}
  }

	bool beginHeader(NxI32 headerId,const char *parent)
	{
		bool ret = false;

    SCHEMA_CLASS c = (SCHEMA_CLASS) headerId;

#if OLD_FORMAT
    if ( mOldFormat )
    {
		  if ( isShape(c) )
        c = SC_NxShapeDesc;
    }
#endif

    bool nextOk = true;

    SchemaClass *sc = mSearchLocations[c];
#if OLD_FORMAT
    if ( mOldFormat && headerId == SC_NxShapeDesc )
    {
      assert( mSearch->mClassType == SC_NxShapeDesc );
      sc = mSearch; // just use the current one.
      nextOk = false;
    }
		else if ( mOldFormat && headerId == SC_NxSimpleTriangleMesh )
		{
			assert( mSearch->mClassType == SC_NxTriangleMeshDesc || mSearch->mClassType == SC_NxClothMeshDesc );
			sc = mSearch;
			nextOk = false;
		}
		else if ( mOldFormat && c == SC_NxJointDesc ) // just use the existing one
		{
			sc = mSearch;
			nextOk = true;
		}
#endif
    if ( sc )
    {
			ret = true;

   		bool ok = true;

   		if ( parent )
   		{
   			const char *id = sc->locateAttribute("id");
				if ( id )
				{
					const char *v = getElement(parent);
					const char *idp = getElement(id);
#if OLD_FORMAT
					if ( mOldFormat )
					{
						if (_stricmp(id,"filterConstant0") == 0 )
							idp = "groupMask0";
						else if ( _stricmp(id,"filterConstant1") == 0 )
							idp = "groupMask1";
						else if ( _stricmp(id,"data") == 0 && c == SC_NxParticleData )
							idp = "initialParticleData";
					}
#endif
   				if ( _stricmp(idp,v) != 0 )
   				{
   					ret = false;
   				}
   			}
				else
				{
          ret = true;
//					reportWarning("unable to find 'id' for '%s' of '%s'\r\n", parent, sc->mNode->Value() );
				}
   		}

      if ( ret )
      {
        if ( mStackPtr < 64 )
        {
          mStack[mStackPtr] = mSearch;
          mStackPtr++;
        }

        mSearch = sc;

        if ( nextOk )
          mSearchLocations[c] = sc->mNext;
      }

    }

    return ret;
	}

  bool endHeader(void)
  {
  	bool ret = false;


    if ( mStackPtr )
    {
      mStackPtr--;
      mSearch = mStack[mStackPtr];
      ret = true;
    }

  	return ret;
  }

#if OLD_FORMAT
  bool isShape(SCHEMA_CLASS c)
  {
    bool ret = false;


    if ( c == SC_NxWheelShapeFlags ||
         c == SC_NxBoxShapeDesc ||
         c == SC_NxPlaneShapeDesc ||
         c == SC_NxSphereShapeDesc ||
         c == SC_NxCapsuleShapeDesc ||
         c == SC_NxMeshShapeFlag ||
         c == SC_NxConvexShapeDesc ||
         c == SC_NxTriangleMeshShapeDesc ||
         c == SC_NxWheelShapeDesc ||
         c == SC_NxHeightFieldShapeDesc )
    {
      ret = true;
    }

    return ret;
  }
#endif

  NxI32 peekHeader(NxI32 or0,
                   NxI32 or1,
                   NxI32 or2,
                   NxI32 or3,
                   NxI32 or4,
                   NxI32 or5,
                   NxI32 or6,
                   NxI32 or7,
                   NxI32 or8,
                   NxI32 or9,
                   NxI32 or10)
  {
    NxI32 peekId = -1;

    int scan[11];

    scan[0] = or0;
    scan[1] = or1;
    scan[2] = or2;
    scan[3] = or3;
    scan[4] = or4;
    scan[5] = or5;
    scan[6] = or6;
    scan[7] = or7;
    scan[8] = or8;
    scan[9] = or9;
    scan[10] = or10;
#if OLD_FORMAT
    if ( mOldFormat )
    {
    	if ( or1 != -1 ) // if scanning for multiple types...
    	{
        SCHEMA_CLASS s = (SCHEMA_CLASS) scan[0];

        if ( isShape(s) )
        {
          peekId = -2;
          SchemaClass *shape = mSearchLocations[SC_NxShapeDesc];
          if ( shape )
          {
						SchemaClass *parent = shape->mParent;

						if ( parent == mSearch )
						{
							const char *type = shape->locate("type",mOldFormat);

							if ( type )
							{
								if ( strcmp(type,"NX_SHAPE_PLANE") == 0 )
								{
									peekId = SC_NxPlaneShapeDesc;
								}
								else if ( strcmp(type,"NX_SHAPE_BOX") == 0 )
								{
									peekId = SC_NxBoxShapeDesc;
								}
                else if ( strcmp(type,"NX_SHAPE_SPHERE") == 0 )
                {
                  peekId = SC_NxSphereShapeDesc;
                }
                else if ( strcmp(type,"NX_SHAPE_CAPSULE") == 0 )
                {
                  peekId = SC_NxCapsuleShapeDesc;
                }
                else if ( strcmp(type,"NX_SHAPE_WHEEL") == 0 )
                {
                  peekId = SC_NxWheelShapeDesc;
                }
                else if ( strcmp(type,"NX_SHAPE_CONVEX") == 0 )
                {
                  peekId = SC_NxConvexShapeDesc;
                }
                else if ( strcmp(type,"NX_SHAPE_MESH") == 0 )
                {
                  peekId = SC_NxTriangleMeshShapeDesc;
                }
                else if ( strcmp(type,"NX_SHAPE_HEIGHTFIELD") == 0 )
                {
                  peekId = SC_NxHeightFieldShapeDesc;
                }
							}
						}
					}
        }
      }
    }
#endif
    if ( peekId == -1 )
    {

      int nearest = 999999;

      for (int i=0; i<11; i++)
      {
        NxI32 header = scan[i];

        if ( header < 0)
          break;

        if ( header >= 0 && header < SC_LAST )
        {
  				SCHEMA_CLASS ctype = (SCHEMA_CLASS) header;


          SchemaClass *sc = mSearchLocations[header];

    			if ( sc )
    			{
    				SchemaClass *parent = sc->mParent;
#if OLD_FORMAT
						if ( mOldFormat && (ctype == SC_NxSceneDesc || ctype == SC_NxScene)  )
							parent = mSearch;
						else if ( mOldFormat && ctype == SC_NxConvexMeshDesc )
							parent = mSearch;
						else if ( mOldFormat && ctype == SC_NxTriangleMeshDesc )
							parent = mSearch;
						else if ( mOldFormat && ctype == SC_NxClothMeshDesc )
							parent = mSearch;
						else if ( mOldFormat && ctype == SC_NxSceneInstanceDesc )
							parent = mSearch;
#endif
  					if ( parent == mSearch )
  					{
  						if ( sc->mIndex < nearest )
  						{
  							nearest = sc->mIndex;
  							peekId = header;
    					}
  					}
    			}
        }
      }
    }

		return peekId;
  }

#if OLD_FORMAT
  // we have to account for cases where the old system treated booleans as enums and convert them..
  bool isEnumFlag(void)
  {
    bool ret = false;
		if ( mOldFormat )
    {
      switch ( mSearch->mClassType )
      {
        case SC_NxD6JointDriveType:
        case SC_NxEmitterType:
        case SC_NxEmitterShape:
          ret = true;
          break;
      }
    }
    return ret;
  }
#endif

  void load(NX_BOOL &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
#if OLD_FORMAT
      if ( isEnumFlag() )
			{
				TiXmlNode *child = mSearch->mNode->FirstChild();
				if ( child && child->Type() == TiXmlNode::TEXT )
				{
					const char *str = child->Value();
					if ( strcmp(name,str) == 0 )
					{
						v = true;
					}
					else
					{
						v = false;
					}
				}
			}
			else
#endif
			{
#if OLD_FORMAT
				if ( mOldFormat && mSearch->mClassType == SC_NxSceneDesc && strcmp(name,"hasMaxBounds") == 0 )
				{
					name = "boundsPlanes";
				}
#endif

  			const char *txt = mSearch->locate(name,mOldFormat);

  			if ( txt )
  			{
					if ( _stricmp(txt,"true") == 0 || _stricmp(txt,"1") == 0 )
						v = true;
					else
						v = false;
  			}
  		}
		}
  }

  void load(NxU16 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			v = (NxU16) atoi(txt);
  		}
  	}
  }

  void load(NxU32 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
#if OLD_FORMAT
			if ( mOldFormat && mSearch->mClassType == SC_NxuPhysicsCollection )
			{
				if ( strcmp(name,"sdkVersion") == 0 )
				{
					name = "SDK_VERSION";
				}
				else if ( strcmp(name,"nxuVersion") == 0 )
				{
					name = "NXU_VERSION";
				}
			}
#endif
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
#if OLD_FORMAT
				if ( mOldFormat && (_stricmp(name,"shapeIndex0") == 0 || _stricmp(name,"shapeIndex1") == 0) )
				{
					int iv = atoi(txt);
					if ( iv < 0 ) iv = 0;
					v = (NxU32) iv;
				}
				else
#endif
				{
  			  v = (NxU32) atoi(txt);
				}
  		}
  	}
  }

  void load(NxF32 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
			const char *txt = 0;
			if ( mSearch->mClassType == SC_NxParameterDesc && strcmp(name,"value") == 0 && mOldFormat )
			{
				TiXmlNode *child = mSearch->mNode->FirstChild();
				if ( child && child->Type() == TiXmlNode::TEXT )
				{
				  txt = child->Value();
				}
			}
			else
			{
  		  txt = mSearch->locate(name,mOldFormat);
			}
  		if ( txt )
  		{
  			Asc2Bin(txt,1,"f",&v);
  		}
  	}
  }

  void load(const char *&v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = 0;

			bool isActorLookup = false;
			bool isConvexLookup = false;
			bool isMeshLookup = false;
      bool isClothLookup = false;

#if OLD_FORMAT
			if ( mOldFormat )
			{

				if ( mSearch->mClassType == SC_NxParameterDesc && strcmp(name,"param") == 0 )
				{
					txt = mSearch->mNode->Value();
				}
				else if ( strcmp(name,"userProperties") == 0 )
				{
					name = "NxuUserProperties";
				}
				else if ( strcmp(name,"meshData") == 0 && mSearch->mClassType == SC_NxShapeDesc )
				{
					if ( mStream->getCurrentShape() && mStream->getCurrentShape()->mType == SC_NxConvexShapeDesc )
					{
  					name = "mConvexMeshDesc";
	  				isConvexLookup = true;
	  			}
	  			else
	  			{
	  				name = "mTriangleMeshDesc";
	  				isMeshLookup = true;
	  			}
				}
				else if ( mSearch->mClassType == SC_NxPairFlagDesc && (strcmp(name,"actor0") == 0 || strcmp(name,"actor1") == 0 ) )
				{
					isActorLookup = true;
				}
				else if ( isJoint(mSearch->mClassType) && strcmp(name,"actor0") == 0 )
				{
					name = "mRefAttachActorDesc";
					isActorLookup = true;
				}
				else if ( isJoint(mSearch->mClassType) && strcmp(name,"actor1") == 0 )
				{
					name = "mAttachActorDesc";
					isActorLookup = true;
				}
        else if ( mSearch->mClassType == SC_NxClothDesc && strcmp(name,"clothMesh") == 0 )
        {
          name = "ClothMeshIndex";
          isClothLookup = true;
        }
			}
#endif

			if ( !txt )
			{
			  txt = mSearch->locate(name,mOldFormat);
			}

  		if ( txt )
  		{
				if ( isActorLookup || isConvexLookup || isMeshLookup || isClothLookup )
				{
					int no = atoi(txt);
					if ( isActorLookup )
					  v = actorLookup(no);
					else if ( isConvexLookup )
						v = convexLookup(no);
					else if ( isMeshLookup )
						v = meshLookup(no);
          else if ( isClothLookup )
            v = clothLookup(no);
				}
				else
				{
  			  v = getGlobalString(txt);
				}
  		}
  	}
  }

	const char *convexLookup(int index)
	{
		const char *ret = 0;
		if ( mStream->getCurrentCollection() )
		{
			int count = mStream->getCurrentCollection()->mConvexMeshes.size();
			if ( index >= 0 && index < count )
			{
				NxConvexMeshDesc *cm = mStream->getCurrentCollection()->mConvexMeshes[index];
				ret = cm->mId;
			}
		}
		return ret;
	}

	const char *meshLookup(int index)
	{
		const char *ret = 0;
		if ( mStream->getCurrentCollection() )
		{
			int count = mStream->getCurrentCollection()->mTriangleMeshes.size();
			if ( index >= 0 && index < count )
			{
				NxTriangleMeshDesc *cm = mStream->getCurrentCollection()->mTriangleMeshes[index];
				ret = cm->mId;
			}
		}
		return ret;
	}

	const char *clothLookup(int index)
	{
		const char *ret = 0;
		if ( mStream->getCurrentCollection() )
		{
			int count = mStream->getCurrentCollection()->mClothMeshes.size();
			if ( index >= 0 && index < count )
			{
				NxClothMeshDesc *cm = mStream->getCurrentCollection()->mClothMeshes[index];
				ret = cm->mId;
			}
		}
		return ret;
	}

	const char * actorLookup(int index)
	{
    const char *ret = 0;
    if ( mStream->getCurrentScene() )
    {
      int count = mStream->getCurrentScene()->mActors.size();
      if ( index >= 0 && index < count )
      {
        ret = mStream->getCurrentScene()->mActors[index]->mId;
      }
			else
			{
				if ( index != -1 )
					assert(0);
			}
    }
		return ret;
	}

  void load(NxArray< NxF32 > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"f");
  			if ( mem )
  			{
  				float *source = (float *)mem;
  				for (int i=0; i<count; i++)
  				{
  					v.push_back( source[i]);
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxArray< NxU32 > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"d");
  			if ( mem )
  			{
  				NxU32 *source = (NxU32 *)mem;
  				for (int i=0; i<count; i++)
  				{
  					v.push_back( source[i]);
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxArray< NxU16 > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"h");
  			if ( mem )
  			{
  				NxU16 *source = (NxU16 *)mem;
  				for (int i=0; i<count; i++)
  				{
  					v.push_back( source[i]);
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxArray< NxU8 > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"x1");
  			if ( mem )
  			{
  				NxU8 *source = (NxU8 *)mem;
  				for (int i=0; i<count; i++)
  				{
  					v.push_back( source[i]);
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxVec3 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			Asc2Bin(txt,1,"fff",&v.x);
  		}
  	}
  }

  void load(NxQuat &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			NxF32 quat[4];
  			void *mem = Asc2Bin(txt,1,"ffff",quat);
  			if ( mem )
  			{
  				v.setXYZW(quat);
  			}
  		}
  	}
  }

  void load(NxMat34 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			float matrix[12];
  			void *mem = Asc2Bin(txt,1,"fff fff fff fff",matrix);
  			if ( mem )
  			{

 				  v.M.setRowMajor(matrix);

  				v.t.x = matrix[9];
  				v.t.y = matrix[10];
  				v.t.z = matrix[11];
  			}
  		}
  	}
  }

  void load(NxArray< NxVec3 > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"fff");
  			if ( mem )
  			{
  				float *source = (float *)mem;
  				for (int i=0; i<count; i++)
  				{
  					NxVec3 nv(source);
  					v.push_back(nv);
  					source+=3;
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxArray< NxTri > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"ddd");
  			if ( mem )
  			{
  				NxU32 *source = (NxU32 *)mem;
  				for (int i=0; i<count; i++)
  				{
  					NxTri t(source[0],source[1],source[2]);
  					v.push_back(t);
  					source+=3;
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxArray< NxTetra > &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			int count=0;
  			void *mem = Asc2Bin(txt,count,"dddd");
  			if ( mem )
  			{
  				NxU32 *source = (NxU32 *)mem;
  				for (int i=0; i<count; i++)
  				{
  					NxTetra t(source[0],source[1],source[2],source[3]);
  					v.push_back(t);
  					source+=4;
  				}
  				delete mem;
  			}
  		}
  	}
  }

  void load(NxBounds3 &v,const char *name,bool attribute)
  {
  	if ( mSearch )
  	{
  		const char *txt = mSearch->locate(name,mOldFormat);
  		if ( txt )
  		{
  			float bounds[6];
  			void *mem = Asc2Bin(txt,2,"fff",bounds);
  			if ( mem )
  			{
  				v.min.set(bounds);
  				v.max.set(&bounds[3]);
  			}
  		}
  	}
  }


	bool               mOldFormat;
	TiXmlDocument   	*mDocument;
	TiXmlNode         *mNode;
  SchemaClass       *mSearch;
  bool               mIsValid;
  SchemaClass       *mSearchLocations[SC_LAST];
	SchemaClassVector  mClasses;
	FILE *mFph;
  SchemaClass       *mStack[64];
  int                mStackPtr;
  SchemaStream      *mStream; // parent stream.
};

static bool isUpper(const char c)
{
  bool ret = false;
  if ( c >= 'A' && c <= 'Z' ) ret = true;
  return ret;
}

const char *getElement(const char *name)
{
  const char *ret = name;

  if ( name && name[0] == 'm' && isUpper(name[1]) )
  {
  	static char	data[64	*32];
  	static int index = 0;

  	char *store	=	&data[index	*64];
  	index++;

  	if (index	== 32)
  	{
  		index	=	0;
  	}

    if ( isUpper(name[2]) ) // don't lower case the first letter if the next letter is also upper case!
    	store[0] = name[1];
    else
      store[0] = name[1]+32; // convert it to lower case.

    const char *source = &name[2];
    char *dest = &store[1];
    while ( *source )
    {
      *dest++ = *source++;
    }
    *dest = 0;
    ret = store;
  }

  return ret;
}

static const char	*fstring(NxF32 v,	bool binary	=	false)
{
	static char	data[64	*16];
	static int index = 0;

	char *ret	=	&data[index	*64];
	index++;
	if (index	== 16)
	{
		index	=	0;
	}

	if (v	== FLT_MAX)
	{
		strcpy(ret,	"FLT_MAX");
	}
	else if	(v ==	FLT_MIN)
	{
		strcpy(ret,	"FLT_MIN");
	}
	else if	(v ==	1)
	{
		strcpy(ret,	"1");
	}
	else if	(v ==	0)
	{
		strcpy(ret,	"0");
	}
	else if	(v ==	 - 1)
	{
		strcpy(ret,	"-1");
	}
	else
	{
		if (binary)
		{
			unsigned int *iv = (unsigned int*) &v;
			sprintf(ret, "%.4f$%x",	v,	*iv);
		}
		else
		{
			sprintf(ret, "%.9f", v);
			const	char *dot	=	strstr(ret,	".");
			if (dot)
			{
				int	len	=	(int)strlen(ret);
				char *foo	=	&ret[len - 1];
				while	(*foo	== '0')
				{
					foo--;
				}
				if (*foo ==	'.')
				{
					*foo = 0;
				}
				else
				{
					foo[1] = 0;
				}
			}
		}
	}

	return ret;
}

class SchemaBlock
{
public:

  SchemaBlock(const char *header,SchemaStream *f,int depth)
  {
  	mName = header;
  	mFph = f;
  	mDepth = depth;
  	mFlushed = false;
  	indent(true,mDepth);
  	add(true,"<%s",header);
  }

  ~SchemaBlock(void)
  {
  	if ( mFph )
  	{
  		if ( mFlushed )
  		{
				assert( mHeader.size() == 0 );

      	if ( mBody.size() )
      	{
    		  mFph->write(&mBody[0], mBody.size() );
    		}

			  for (int i=0; i<mDepth; i++)
			  {
				  mFph->printf("  ");
			  }
     		mFph->printf("</%s>\r\n", mName );
  		}
  		else
  		{
      	mFph->write(&mHeader[0], mHeader.size() );

      	if ( mBody.size() )
      	{
    		  mFph->printf(">\r\n");
    		  mFph->write(&mBody[0], mBody.size());
				  for (int i=0; i<mDepth; i++)
				  {
					  mFph->printf("  ");
				  }
      		mFph->printf("</%s>\r\n", mName );
      	}
      	else
    	  {
    		  mFph->printf("/>\r\n");
      	}
      }
    }
  }

  void flush(void)
  {
  	if ( mHeader.size() )
  	{
    	mFph->write(&mHeader[0], mHeader.size());
    	mFph->printf(">\r\n");
    	mHeader.clear();
  	}
  	if ( mBody.size() )
  	{
   		mFph->write(&mBody[0], mBody.size() );
   		mBody.clear();
  	}
  	mFlushed = true; // if it was flushed already...
  }

  void store(NX_BOOL v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	const char *tf = "false";
  	if ( v ) tf = "true";

  	if ( attribute )
  		add(true," %s=\"%s\"", name, tf );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%s</%s>\r\n", name, tf, name );
  	}
  }

  void store(NxU32 v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	if ( attribute )
  		add(true," %s=\"%u\"", name, v );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%u</%s>\r\n", name, v, name );
  	}
  }

  void store(NxU16 v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	if ( attribute )
  		add(true," %s=\"%u\"", name, v );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%u</%s>\r\n", name, v, name );
  	}
  }

  void store(NxF32 v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	if ( attribute )
  		add(true," %s=\"%s\"", name, fstring(v) );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%s</%s>\r\n", name, fstring(v), name );
  	}
  }

  void store(const char *_v,const char *name,bool attribute)
  {
		const char *v = _v;
		if ( _v == 0 ) v = "";

  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	if ( attribute )
  		add(true," %s=\"%s\"", name, v );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%s</%s>\r\n", name, v, name );
  	}
  }

  void store(NxArray< NxF32 > &v,const char *name,bool attribute)
  {
  	assert(attribute == false);
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%s ", fstring( v[i] ) );
  		if ( ((i+1)&15) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxArray< NxU32 > &v,const char *name,bool attribute)
  {
  	assert(attribute == false);
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%u ", v[i] );
  		if ( ((i+1)&15) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxArray< NxU8 > &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%02X", v[i] );
  		if ( ((i+1)&63) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxArray< NxU16 > &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		NxU32 uv = v[i];
  		add(false,"%u ", uv );
  		if ( ((i+1)&15) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxVec3 &v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	if ( attribute )
  		add(true," %s=\"%s %s %s\"", name, fstring(v.x), fstring(v.y), fstring(v.z) );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%s %s %s</%s>\r\n", name, fstring(v.x), fstring(v.y), fstring(v.z), name );
  	}
  }

  void store(NxQuat &v,const char *name,bool attribute)
  {
  	if ( mFlushed ) assert( attribute == false ); // can't write new attributes after the header has been flushed!
  	float q[4];
  	v.getXYZW(q);

  	if ( attribute )
  		add(true," %s=\"%s %s %s %s\"", name, fstring(q[0]), fstring(q[1]), fstring(q[2]), fstring(q[3]) );
  	else
  	{
  		indent(false,mDepth+1);
  		add(false,"<%s>%s %s %s %s</%s>\r\n", name, fstring(q[0]), fstring(q[1]), fstring(q[2]), fstring(q[3]), name );
  	}
  }

  void store(NxMat34 &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	float f[9];
  	v.M.getRowMajor(f);
 		indent(false,mDepth+1);
  	add(false,"<%s>%s %s %s  %s %s %s  %s %s %s  %s %s %s</%s>\r\n",
  	  name,
  	  fstring(f[0]),fstring(f[1]),fstring(f[2]),
  	  fstring(f[3]),fstring(f[4]),fstring(f[5]),
  	  fstring(f[6]),fstring(f[7]),fstring(f[8]),
  	  fstring(v.t.x),fstring(v.t.y),fstring(v.t.z),
  	  name );
  }

  void store(NxBounds3 &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
 		indent(false,mDepth+1);
    add(false,"<%s>%s %s %s  %s %s %s</%s>\r\n",
      name,
      fstring(v.min.x),fstring(v.min.y),fstring(v.min.z),
      fstring(v.max.x),fstring(v.max.y),fstring(v.max.z),
      name);
  }


  void store(NxArray< NxVec3 > &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%s %s %s  ", fstring( v[i].x ), fstring( v[i].y ), fstring( v[i].z ) );
  		if ( ((i+1)&3) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxArray< NxTri > &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%u %u %u  ", v[i].a, v[i].b, v[i].c );
  		if ( ((i+1)&3) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

  void store(NxArray< NxTetra > &v,const char *name,bool attribute)
  {
  	assert( attribute == false );
  	indent(false,mDepth+1);
  	add(false,"<%s>", name );
  	bool lastReturn = false;
  	int returnCount = 0;
  	for (NxU32 i=0; i<v.size(); i++)
  	{
  		add(false,"%u %u %u %u  ", v[i].a, v[i].b, v[i].c, v[i].d );
  		if ( ((i+1)&3) == 0 )
  		{
  			add(false,"\r\n");
  			indent(false,mDepth+2);
  			lastReturn = true;
  			returnCount++;
  		}
  		else
  			lastReturn = false;
  	}
  	if ( !lastReturn && returnCount != 0)
  	{
  		add(false,"\r\n");
    	indent(false,mDepth+1);
    }
  	add(false,"</%s>\r\n", name );
  }

	void add(bool header,const char *fmt,...)
	{
		char wbuff[2048];
		vsprintf(wbuff, fmt, (char *)(&fmt+1));
		char *scan = wbuff;
		while ( *scan )
		{
			if ( header )
  			mHeader.push_back(*scan);
  		else
  			mBody.push_back(*scan);
			scan++;
		}
	}

	void indent(bool header,int depth)
	{
		for (int i=0; i<depth; i++)
		{
			add(header,"  ");
		}
	}

	SchemaStream   *mFph;
  const char     *mName;
  NxArray< char > mHeader;
  NxArray< char > mBody;
  int             mDepth;
	bool            mFlushed;
};

SchemaStream::SchemaStream(const char *fname,
                           bool isBinary,
                           const char *spec,    // "wb" "rb" or "wmem"
                           void *mem,
                           int len)
{
  mCurrentCollection = 0;
  mCurrentScene = 0;
  mCurrentActor = 0;
  mCurrentJoint = 0;
  mCurrentShape = 0;
  mCurrentCloth = 0;
  mCurrentFluid = 0;

	mBinary = isBinary;
 	mCurrentBlock = 0;
 	mStackPtr = 0;
  mFph = 0;
  mSchemaXML = 0;
  mReadMode = false;
  mIsValid = false;
  mHeaderStackPtr = 0;


  if ( _stricmp(spec,"rb") == 0  )
  {
    mReadMode = true;
    if ( isBinary )
    {
    	mFph = nxu_fopen(fname,spec,mem,len);
      if ( mFph )
      {
        char stream[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        NxU32 sdk = 0;
        NxU32 uv = 0;
    		read(stream,10);
    		read(&sdk, sizeof(NxU32));
    		read(&uv,  sizeof(NxU32));
        bool ok = true;
        if ( strcmp(stream,"NXUSTREAM") != 0 )
        {
          ok = false;
          reportError("File %s is not a valid binary NxuStream file, missing header", fname );
        }
        if ( ok && sdk != NX_SDK_VERSION_NUMBER )
        {
          ok = false;
          reportError("This binary file %s was saved out with a different version of the SDK. Saved with SDK%d but running SDK%d", fname, sdk, NX_SDK_VERSION_NUMBER );
        }
        if ( ok && uv != NXUSTREAM_VERSION )
        {
          ok = false;
          reportError("Even though the file %s was saved with the same SDK version, the version format has changed from %d to %d",fname,uv,NXUSTREAM_VERSION);
        }
				
        if ( ok )
        {
          mIsValid = true;
					if ( !beginHeader(SC_NXUSTREAM2) )
					{
						invalidate("Did not encoutner the expected header");
					}
        }
				else
        {
          invalidate("Invalid NxuStream file header");
        }
      }
      else
      {
        reportError("Failed to open binary file '%s' for read access.", fname );
      }

    }
    else
    {
    	mSchemaXML = new SchemaXML(fname,mem,len,this);
    	if ( !mSchemaXML->mDocument )
    	{
    		delete mSchemaXML;
    		mSchemaXML = 0;
    	}
    	else if ( mSchemaXML->mIsValid )
    	{
			  mIsValid = true;
    	}
    	else
    	{
    		reportError("Valid XML file but does not appear to contain any NXUSTREAM data.");
    		delete mSchemaXML;
    		mSchemaXML = 0;
    	}
    }
  }
  else
  {
  	mFph = nxu_fopen(fname,spec,mem,len);
    if ( mFph )
    {
    	if ( mBinary )
    	{
        mIsValid = true;
    		NxU32 sdk = NX_SDK_VERSION_NUMBER;
    		NxU32 uv  = NXUSTREAM_VERSION;
    		write("NXUSTREAM",10);
    		write(&sdk, sizeof(NxU32));
    		write(&uv,  sizeof(NxU32) );
    	}
    	else
    	{
        mIsValid = true;
        printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
      }
      beginHeader(SC_NXUSTREAM2);
    }
    else
    {
      invalidate("Failed to open file '%s' for write access", fname );
    }
  }
}

void SchemaStream::endFlush(void)
{
  if ( mReadMode )
  {
  }
  else
  {
  	endHeader();

    if ( mBinary )
    {
      beginHeader(SC_LAST); // pad the EOF with a null header.
      endHeader();
    }

  	for (NxU32 i=0; i<mBlocks.size(); i++)
  	{
  		SchemaBlock *sb = mBlocks[i];
  		delete sb;
  	}
  	mBlocks.clear();
  	if ( mFph )
  	{
  		if ( mBinary )
  		{
  			closeLast();
  		}
  	}
  }
}

SchemaStream::~SchemaStream(void)
{
	endFlush();

	if ( mFph )
	{
  	nxu_fclose(mFph);
	}
	delete mSchemaXML;
}


void SchemaStream::closeLast(void)
{
	if ( mStackPtr )
	{
		mStackPtr--;
		NxU32 start = mStack[mStackPtr];
		NxU32 end   = (NxU32)ftell();
		NxU32 len   = (NxU32)(end-start);
		nxu_fseek(mFph,start+sizeof(NxU32)+sizeof(NxI32),SEEK_SET);
		nxu_fwrite(&len,sizeof(NxU32),1,mFph);
		nxu_fseek(mFph,end,SEEK_SET);
	}
}

void SchemaStream::pushLast(NxU32 loc)
{
	assert( mStackPtr < 32 );
	if ( mStackPtr < 32 )
	{
		mStack[mStackPtr] = loc;
		mStackPtr++;
	}
}

bool SchemaStream::beginHeader(NxI32 headerId,const char *parent)
{
  bool ok = true;

  if ( isValid() )
  {
    if ( mReadMode )
    {
      if ( mBinary )
      {
        NxU32 loc = (NxU32)ftell();

        read(&mCurrentHeader.mId1,sizeof(NxU32));
        read(&mCurrentHeader.mType,sizeof(NxI32));
        read(&mCurrentHeader.mLen,sizeof(NxU32));
        read(&mCurrentHeader.mId2,sizeof(NxU32));

        if ( isValid() && mCurrentHeader.isValid() )
        {
          if ( mHeaderStackPtr < 32 )
          {
            assert( mCurrentHeader.mType == headerId );
            if ( mCurrentHeader.mType == headerId )
            {
              mHeaderStack[mHeaderStackPtr] = mCurrentHeader;
              mHeaderStackPtr++;
            }
            else
            {
              ok = false; // not the header we expected
              nxu_fseek(mFph,loc,SEEK_SET); // go back to the original location
              assert(mHeaderStackPtr);
              if ( mHeaderStackPtr )
                mCurrentHeader = mHeaderStack[mHeaderStackPtr];
            }
          }
          else
          {
            invalidate("Header stack overflow.");
          }
        }
        else
        {
          invalidate("Encountered invalid header ID, file is corrupted");
        }
      }
      else if ( mSchemaXML )
      {
     		ok = mSchemaXML->beginHeader(headerId,parent);
      }
    }
    else
    {
    	if ( mBinary )
    	{
    		NxU32 loc = (NxU32)ftell();
    		pushLast(loc);
        SchemaHeader h;
        h.mType =headerId;
        write(&h.mId1,sizeof(NxU32));
        write(&h.mType,sizeof(NxI32));
        write(&h.mLen,sizeof(NxU32));
        write(&h.mId2,sizeof(NxU32));
    	}
    	else
    	{
      	if ( mCurrentBlock )
      		mCurrentBlock->flush(); // flush the parent.. not attributes after this though!
        const char *str = EnumToString( (SCHEMA_CLASS) headerId ); // convert it into a string for XML layout
        str+=3;
        assert(str);
      	mCurrentBlock = new SchemaBlock(str,this,mBlocks.size()+1);
      	mBlocks.push_back(mCurrentBlock);
      }
    }
  }

  if ( !isValid() ) ok = false;

	if (!ok)
	{
#if DEBUG_WARNING
		reportWarning("Failed to begin header(%-30s) (%s)", EnumToString( (SCHEMA_CLASS)headerId), parent );
#endif
	}

	return ok;
}


void SchemaStream::endHeader(void)
{
  if ( isValid() )
  {
    if ( mReadMode )
    {
      if ( mBinary )
      {
        if ( mHeaderStackPtr )
        {
          mHeaderStackPtr--;
          mCurrentHeader = mHeaderStack[mHeaderStackPtr];
        }
      }
      else if ( mSchemaXML )
      {
     		mSchemaXML->endHeader();
      }
    }
    else
    {
    	if ( mBinary )
    	{
    		closeLast();
    	}
    	else
    	{
      	if ( mBlocks.size() )
      	{
          SchemaBlock* sb = mBlocks.back();
          delete sb;
          mBlocks.popBack();
      	}
      	if ( mBlocks.size() )
      		mCurrentBlock = mBlocks.back();
      	else
      		mCurrentBlock = 0;
    	}
    }
  }
}

void SchemaStream::write(const void *mem,NxU32 size)
{
  assert(size);
  if ( mFph && size )
  {
    int count = nxu_fwrite(mem,size,1,mFph);
    if ( count == 0 )
    {
      invalidate("Write failure saving NxuStream data");
    }
  }
}

void SchemaStream::read(void *mem,NxU32 size)
{
  assert( size );
  if ( mFph && size)
  {
    int count = nxu_fread(mem,size,1,mFph);
    if ( count == 0 )
    {
      invalidate("Read failure loading NxuStream file.");
    }
  }
}

void SchemaStream::store(NX_BOOL v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
	  	mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
      write(&v,sizeof(NX_BOOL));
  }
}

void SchemaStream::store(NxU32 v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
      write(&v,sizeof(NxU32));
  }
}

void SchemaStream::store(NxF32 v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
      write(&v,sizeof(NxF32));
  }
}

void SchemaStream::store(const char *v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 len = 0;
  		if ( v )
  		{
  			len = (NxU32)strlen(v);
        assert( len < 1024 );
        if ( len >= 1024 )
        {
          len = 0;
        }
  		}

      write(&len,sizeof(NxU32));

  		if ( v && len )
  		{
        write(v,len+1);
  		}
    }
  }
}

void SchemaStream::store(NxArray< NxF32 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxF32)*count );
      }
    }
  }
}

void SchemaStream::store(NxArray< NxU32 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxU32)*count );
      }
    }
  }
}

void SchemaStream::store(NxArray< NxU8 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxU8)*count );
      }
    }
  }
}

void SchemaStream::store(NxArray< NxU16 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxU16)*count );
      }
    }
  }
}

void SchemaStream::store(NxVec3 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
      write(&v.x,sizeof(NxVec3));
  }
}

void SchemaStream::store(NxQuat &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxF32 q[4];
      v.getXYZW(q);
      write(q,sizeof(NxF32)*4);
    }
  }

}

void SchemaStream::store(NxMat34 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxF32 matrix[16];
      v.getRowMajor44(matrix);
      write(matrix,sizeof(NxF32)*16);
    }
  }
}

void SchemaStream::store(NxArray< NxVec3 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0].x, sizeof(NxVec3)*count );
      }
    }
  }
}

void SchemaStream::store(NxArray< NxTri > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxTri)*count );
      }
    }
  }
}

void SchemaStream::store(NxBounds3 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      write(&v.min.x, sizeof(NxVec3) );
      write(&v.max.x, sizeof(NxVec3) );
    }
  }
}


void * SchemaStream::getMemBuffer(int &outputLength)
{
	void * ret = 0;
	outputLength = 0;
	if ( mFph )
	{
		endFlush(); // flush out the end of the data.
		ret = nxu_getMemBuffer(mFph,outputLength);
	}
	return ret;
}

void SchemaStream::printf(const char *fmt,...)
{
	char wbuff[2048];
	vsprintf(wbuff, fmt, (char *)(&fmt+1));
  NxU32 len = (NxU32)strlen(wbuff);
  if ( len )
  {
    write(wbuff,len);
  }
}

void SchemaStream::store(NxU16 v,const char *name,bool attribute)
{
  name = getElement(name);
	if ( mCurrentBlock )
		mCurrentBlock->store(v,name,attribute);
  else
    write(&v,sizeof(NxU16));
}


void SchemaStream::load(NX_BOOL &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      read(&v,sizeof(NX_BOOL));
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxU16 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      read(&v,sizeof(NxU16));
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxU32 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      read(&v,sizeof(NxU32));
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxF32 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() && mBinary )
  {
    read(&v,sizeof(NxF32));
  }
  else if ( mSchemaXML )
  {
    mSchemaXML->load(v,name,attribute);
  }
}

void SchemaStream::load(const char *&v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      v = 0;
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      assert(len<1024);
      if ( len >= 1024 ) len = 0;
      if ( len )
      {
        char scratch[1024];
        memset(scratch,0,len+1);
        read(scratch,len+1);
        v = getGlobalString(scratch);
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxArray< NxF32 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxF32 value;
          read(&value,sizeof(NxF32));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxArray< NxU32 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxU32 value;
          read(&value,sizeof(NxU32));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxArray< NxU16 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxU16 value;
          read(&value,sizeof(NxU16));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxArray< NxU8 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxU8 value;
          read(&value,sizeof(NxU8));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }

  }
}

void SchemaStream::load(NxVec3 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      read(&v.x,sizeof(NxVec3));
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxQuat &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxF32 quat[4];
      read(quat,sizeof(NxF32)*4);
      if ( isValid() )
      {
        v.setXYZW(quat);
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxMat34 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxF32 matrix[16];
      read(matrix,sizeof(NxF32)*16);
      if ( isValid() )
      {
        v.setRowMajor44(matrix);
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxArray< NxVec3 > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxVec3 value;
          read(&value.x,sizeof(NxVec3));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }

  }
}

void SchemaStream::load(NxArray< NxTri > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxTri value;
          read(&value.a,sizeof(NxTri));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

void SchemaStream::load(NxBounds3 &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      read(&v.min.x,sizeof(NxVec3));
      read(&v.max.x,sizeof(NxVec3));
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}

NxU32 SchemaStream::ftell(void)
{
  NxU32 ret = 0;

  if ( mFph )
    ret = nxu_ftell(mFph);

  return ret;
}

bool SchemaStream::peekHeader(NxI32 headerId,
									NxI32 *peek,
                  NxI32 or1,
                  NxI32 or2,
                  NxI32 or3,
                  NxI32 or4,
                  NxI32 or5,
                  NxI32 or6,
                  NxI32 or7,
                  NxI32 or8,
                  NxI32 or9,
                  NxI32 or10)

{
  bool ret = false;

  if ( isValid() )
  {
		if ( mBinary )
		{
			NxU32 loc = (NxU32)ftell();
			SchemaHeader h;
			read(&h.mId1,sizeof(NxU32));
			read(&h.mType,sizeof(NxI32));
			read(&h.mLen,sizeof(NxU32));
			read(&h.mId2,sizeof(NxU32));

			if ( isValid() && h.isValid() )
			{

				if ( h.mType == headerId ) ret = true;
 				else if ( h.mType == or1 ) ret = true;
				else if ( h.mType == or2 ) ret = true;
				else if ( h.mType == or3 ) ret = true;
				else if ( h.mType == or4 ) ret = true;
				else if ( h.mType == or5 ) ret = true;
				else if ( h.mType == or6 ) ret = true;
				else if ( h.mType == or7 ) ret = true;
				else if ( h.mType == or8 ) ret = true;
				else if ( h.mType == or9 ) ret = true;
				else if ( h.mType == or10 ) ret = true;
				if ( ret && peek ) *peek = h.mType;
			}
			if ( mFph )
			  nxu_fseek(mFph,loc,SEEK_SET);
	  }
		else if ( mSchemaXML )
		{
			NxI32 pno = mSchemaXML->peekHeader(headerId,or1,or2,or3,or4,or5,or6,or7,or8,or9,or10);

			if ( pno >= 0 )
			{
				if ( peek ) *peek = pno;
				ret = true;
			}

		}
	}
	return ret;
}


bool SchemaHeader::isValid(void)
{
  bool ret = false;

  if ( mId1 == HEADER1 && mId2 == HEADER2 && mType >= 0 && mType < SC_LAST )
  {
    ret = true;
  }
	return ret;
}


void SchemaStream::invalidate(const char *fmt,...)
{
  mIsValid = false;
  if ( mFph )
  {
    nxu_fclose(mFph);
    mFph = 0;
  }
  char wbuff[2048];
	vsprintf(wbuff, fmt, (char *)(&fmt+1));
  reportError("%s", wbuff );
}

void SchemaStream::setCurrentCollection(NxuPhysicsCollection *c)
{
  mCurrentCollection = c;
}

void SchemaStream::setCurrentScene(NxSceneDesc *s)
{
  mCurrentScene = s;
}

void SchemaStream::setCurrentJoint(NxJointDesc *j) { mCurrentJoint = j; };
void SchemaStream::setCurrentActor(NxActorDesc *a) { mCurrentActor = a; };
void SchemaStream::setCurrentShape(NxShapeDesc *s) { mCurrentShape = s; };
void SchemaStream::setCurrentFluid(NxFluidDesc *f) { mCurrentFluid = f; };
void SchemaStream::setCurrentCloth(NxClothDesc *c) { mCurrentCloth = c; };


NxSceneDesc        *locateSceneDesc(NxuPhysicsCollection *c,const char *name,NxU32 &index)
{
	NxSceneDesc *ret = 0;
	if ( name )
	{
		for (unsigned int i=0; i<c->mScenes.size(); i++)
		{
			NxSceneDesc *s = c->mScenes[i];
			if ( s->mId &&  strcmp(s->mId,name) == 0 )
			{
				index = i;
				ret = s;
				break;
			}
		}
	}
	return ret;
}

NxConvexMeshDesc   *locateConvexMeshDesc(NxuPhysicsCollection *c,const char *name)
{
	NxConvexMeshDesc *ret = 0;
	for (unsigned int i=0; i<c->mConvexMeshes.size(); i++)
	{
		NxConvexMeshDesc *s = c->mConvexMeshes[i];
		if ( s->mId &&  strcmp(s->mId,name) == 0 )
		{
			ret = s;
			break;
		}
	}
	return ret;
}

NxTriangleMeshDesc * locateTriangleMeshDesc(NxuPhysicsCollection *c,const char *name)
{
	NxTriangleMeshDesc *ret = 0;
	for (unsigned int i=0; i<c->mTriangleMeshes.size(); i++)
	{
		NxTriangleMeshDesc *s = c->mTriangleMeshes[i];
		if ( s->mId &&  strcmp(s->mId,name) == 0 )
		{
			ret = s;
			break;
		}
	}
	return ret;
}

NxHeightFieldDesc  *locateHeightFieldDesc(NxuPhysicsCollection *c,const char *name)
{
	NxHeightFieldDesc *ret = 0;

	for (unsigned int i=0; i<c->mHeightFields.size(); i++)
	{
		NxHeightFieldDesc *s = c->mHeightFields[i];
		if ( s->mId && strcmp(s->mId,name) == 0 )
		{
			ret = s;
			break;
		}
	}
	return ret;
}

NxActorDesc        *locateActorDesc(NxSceneDesc *s,const char *name)
{
	NxActorDesc *ret = 0;
	for (unsigned int i=0; i<s->mActors.size(); i++)
	{
		NxActorDesc *a = s->mActors[i];
		if ( a->mId && strcmp(a->mId, name ) == 0 )
		{
			ret = a;
			break;
		}
	}
	return ret;
}

void SchemaStream::store(NxArray< NxTetra > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
  	if ( mCurrentBlock )
  		mCurrentBlock->store(v,name,attribute);
    else if ( mBinary )
    {
      NxU32 count = v.size();
      write(&count,sizeof(NxU32));
      if ( count )
      {
        write(&v[0], sizeof(NxTetra)*count );
      }
    }
  }
}

void SchemaStream::load(NxArray< NxTetra > &v,const char *name,bool attribute)
{
  name = getElement(name);
  if ( isValid() )
  {
    if ( mBinary )
    {
      NxU32 len = 0;
      read(&len,sizeof(NxU32));
      if ( len && isValid() )
      {
        for (NxU32 i=0; i<len; i++)
        {
          NxTetra value;
          read(&value.a,sizeof(NxTetra));
          if ( isValid() )
          {
            v.push_back(value);
          }
          else
          {
            v.clear();
            break;
          }
        }
      }
    }
    else if ( mSchemaXML )
    {
      mSchemaXML->load(v,name,attribute);
    }
  }
}


};
