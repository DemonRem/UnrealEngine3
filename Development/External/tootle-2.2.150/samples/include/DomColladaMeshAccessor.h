//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmDOMColladaMeshAccessor
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/DomColladaMeshAccessor.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _DOM_COLLADA_MESH_ACCESSOR_H_
#define _DOM_COLLADA_MESH_ACCESSOR_H_

#include "ColladaMeshAccessor.h"
#include "AMDColladaGeometry.h"
#include "AMDColladaUtils.h"

#include <vector>

struct CrtVec3f;
struct CrtVec2f;
class  CrtTriangles;

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief Responsible for accessing meshes in a COLLADA DOM document.
//=========================================================================================================
class DOMColladaMeshAccessor : public AMDColladaMeshAccessor
{
public:

   //====================================================================================
   /// \brief Constructor
   //====================================================================================
   DOMColladaMeshAccessor::DOMColladaMeshAccessor();

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual DOMColladaMeshAccessor::~DOMColladaMeshAccessor();

   //====================================================================================
   /// \brief Loads the vertices into the output vertices array for use externally.
   //====================================================================================
   virtual bool LoadVertices( std::vector< ObjVertex3D > & vertices ) const;

   //====================================================================================
   /// \brief Loads the normals into the output normals array for use externally.
   //====================================================================================
   virtual bool LoadNormals( std::vector< ObjVertex3D > & normals ) const;

   //====================================================================================
   /// \brief Loads the texcoords into the output texCoords array for use externally.
   //====================================================================================
   virtual bool LoadTexCoords( std::vector< ObjVertex2D > & texCoords ) const;

   //====================================================================================
   /// \brief Loads the face indices into the output faces array for use externally.
   //====================================================================================
   virtual bool LoadFaces( std::vector< ObjFace > & faces ) const;

   //====================================================================================
   /// \brief Sets the internal state to default values.
   //====================================================================================
   virtual void ResetState();

   //====================================================================================
   /// \brief Returns a pointer to a list of position indices.
   //====================================================================================
   const unsigned int *       GetPositionIndices() const
   {
      return m_pnPositionIndices;
   }

   //====================================================================================
   /// \brief Returns a pointer to a list of positions.
   //====================================================================================
   const CrtVec3f *           GetPositionList() const
   {
      return m_pfPositionList;
   }

   //====================================================================================
   /// \brief Returns  pointer to a list of normal indices.
   //====================================================================================
   const unsigned int *       GetNormalIndices() const
   {
      return m_pnNormalIndices;
   }

   //====================================================================================
   /// \brief Returns the a pointer to a list of normal data.
   //====================================================================================
   const CrtVec3f *           GetNormalList() const
   {
      return m_pfNormalList;
   }

   //====================================================================================
   /// \brief Returns  pointer to a list of texture coordinate indices.
   //====================================================================================
   const unsigned int *       GetTexCoordIndices() const
   {
      return m_pnTexCoordIndices;
   }

   //====================================================================================
   /// \brief Returns the a pointer to a list of texture coordinate data.
   //====================================================================================
   const CrtVec2f *           GetTexCoordList() const
   {
      return m_pfTexCoordList;
   }

   //====================================================================================
   /// \brief Returns a pointer to a list of face/vertex count (3,3,3,3,4,3,...etc)
   //====================================================================================
   const unsigned int *       GetFaceIndices() const
   {
      return m_pnFaceIndices;
   }

   //====================================================================================
   /// \brief Sets the required geometry data in the mesh accessor
   //====================================================================================
   bool SetGeometryData( CrtTriangles * pTriMesh );

   //====================================================================================
   /// \brief Sets the required geometry data in the mesh accessor
   //====================================================================================
   void SetGeometryData( AMDColladaGeometry * pAMDColladaGeometry );

   //====================================================================================
   /// \brief Returns the number of vertices in the mesh
   //====================================================================================
   unsigned int GetVertexCount() const
   {
      return ( unsigned int )m_nVertexCount;
   }

protected:

private:

   //====================================================================================
   /// \brief Sets the required position data in the mesh accessor
   //====================================================================================
   bool SetPositionData( CrtTriangles * pTriMesh );

   //====================================================================================
   /// \brief Sets the required normal data in the mesh accessor
   //====================================================================================
   bool SetNormalData( CrtTriangles * pTriMesh );

   //====================================================================================
   /// \brief Sets the required texture data in the mesh accessor
   //====================================================================================
   bool SetTextureData( CrtTriangles * pTriMesh );


   //====================================================================================
   /// \brief Sets the required position data in the mesh accessor
   //====================================================================================
   bool SetPositionData( AMDColladaGeometry * pAMDColladaGeometry );

   //====================================================================================
   /// \brief Sets the required normal data in the mesh accessor
   //====================================================================================
   bool SetNormalData( AMDColladaGeometry * pAMDColladaGeometry );

   //====================================================================================
   /// \brief Sets the required texture data in the mesh accessor
   //====================================================================================
   bool SetTextureData( AMDColladaGeometry * pAMDColladaGeometry );

   //====================================================================================
   /// \brief Sets the required Face indices.
   //====================================================================================
   void SetFaceIndices( unsigned int * i_pnFaceIndices )
   {
      m_pnFaceIndices = i_pnFaceIndices;
   }

   //====================================================================================
   /// \brief Sets the number of vertex indices.
   //====================================================================================
   void SetVertexCount( unsigned int i_nVertexCount )
   {
      m_nVertexCount = i_nVertexCount;
   }

   //====================================================================================
   /// \brief Returns value at the index position.
   //====================================================================================
   bool GetFaceIndex( unsigned int     nIndex,
                      unsigned int &   nOutFaceIndex ) const;

   //====================================================================================
   /// \brief Sets the size of the index array.
   //====================================================================================
   void SetFaceIndices( AMDColladaGeometry * pAMDColladaGeometry );

   //====================================================================================
   /// \brief Returns the index array offset for the position data.
   //====================================================================================
   unsigned int GetPositionOffset() const
   {
      if ( m_pPositionSource )
      {
         return m_pPositionSource->GetIndexMapOffset() ;
      }
     else
      {
         return 0 ;
      }
   }

   //====================================================================================
   /// \brief Returns the index array offset for the texcoord data.
   //====================================================================================
   unsigned int GetTexCoordOffset() const
   {
      if ( m_pTextureSource )
      {
         return m_pTextureSource->GetIndexMapOffset() ;
      }
      else
      {
         return 0 ;
      }
   }

   //====================================================================================
   /// \brief Returns the index array offset for the normal data.
   //====================================================================================
   unsigned int GetNormalOffset() const
   {
      if ( m_pNormalSource )
      {
         return m_pNormalSource->GetIndexMapOffset() ;
      }
     else
      {
         return 0 ;
      }
   }
 
   unsigned int m_nPositionOffset ;
   unsigned int m_nTexCoordOffset ;
   unsigned int m_nNormalOffset ;

   // Non-custodial pointers to index data in the FCollad DB.
   unsigned int *             m_pnPositionIndices;
   unsigned int *             m_pnNormalIndices;
   unsigned int *             m_pnTexCoordIndices;

   // Non-custodial pointers to geometry data in the FCollad DB.
   CrtVec3f *                 m_pfPositionList;
   CrtVec3f *                 m_pfNormalList;
   CrtVec2f *                 m_pfTexCoordList;


   // These point to the source readers for the various input streams.
   DOMColladaSourceReader *   m_pPositionSource;
   DOMColladaSourceReader *   m_pNormalSource;
   DOMColladaSourceReader *   m_pTextureSource;
   DOMColladaSourceReader *   m_pTangentSource;
   DOMColladaSourceReader *   m_pBinormalSource;

   // List of Face indices
   unsigned int *             m_pnFaceIndices;

   unsigned int               m_nVertexCount;

   unsigned int               m_nParraySize;

   AMDColladaGeometry *       m_pAMDColladaGeom;
};

#endif // _DOM_COLLADA_MESH_ACCESSOR_H_
