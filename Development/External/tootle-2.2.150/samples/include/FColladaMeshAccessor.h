//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmFColladaMeshAccessor
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/FColladaMeshAccessor.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _FCOLLADA_MESH_ACCESSOR_H_
#define _FCOLLADA_MESH_ACCESSOR_H_

#define DONT_USE_PLATFORM_H

#if 0

#include "ColladaMeshAccessor.h"
#include "FCollada.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"

#include <vector>

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief Responsible for accessing an FCollada mesh.
/// 
//=========================================================================================================
class FColladaMeshAccessor : public AMDColladaMeshAccessor
{
public:

   //====================================================================================
   /// \brief Constructor
   //====================================================================================
   FColladaMeshAccessor::FColladaMeshAccessor();

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual FColladaMeshAccessor::~FColladaMeshAccessor();

   //====================================================================================
   /// \brief Loads the vertices into the output vertices array for use externally.
   //====================================================================================
   virtual bool LoadVertices( std::vector<ObjVertex3D> &vertices ) const;

   //====================================================================================
   /// \brief Loads the normals into the output normals array for use externally.
   //====================================================================================
   virtual bool LoadNormals( std::vector<ObjVertex3D>  &normals ) const;

   //====================================================================================
   /// \brief Loads the texcoords into the output texCoords array for use externally.
   //====================================================================================
   virtual bool LoadTexCoords( std::vector<ObjVertex2D> &texCoords ) const;

   //====================================================================================
   /// \brief Loads the face indices into the output faces array for use externally.
   //====================================================================================
   virtual bool LoadFaces( std::vector<ObjFace>    &faces ) const;

   //====================================================================================
   /// \brief Sets the internal state to default values.
   //====================================================================================
   virtual void ResetState();

   //====================================================================================
   /// \brief Returns a pointer to a list of position indices.
   //====================================================================================
   const UInt32List* GetPositionIndices() const { return m_pnPositionIndices; }

   //====================================================================================
   /// \brief Returns a pointer to a list of positions.
   //====================================================================================
   const FloatList*  GetPositionList() const { return m_pfPositionList; }

   //====================================================================================
   /// \brief Returns  pointer to a list of normal indices.
   //====================================================================================
   const UInt32List* GetNormalIndices() const { return m_pnNormalIndices; }

   //====================================================================================
   /// \brief Returns the a pointer to a list of normal data.
   //====================================================================================
   const FloatList*  GetNormalList() const { return m_pfNormalList; }

   //====================================================================================
   /// \brief Returns  pointer to a list of texture coordinate indices.
   //====================================================================================
   const UInt32List* GetTexCoordIndices() const { return m_pnTexCoordIndices; }

   //====================================================================================
   /// \brief Returns the a pointer to a list of texture coordinate data.
   //====================================================================================
   const FloatList*  GetTexCoordList() const { return m_pfTexCoordList; }

   //====================================================================================
   /// \brief Returns a pointer to a list of face/vertex count (3,3,3,3,4,3,...etc)
   //====================================================================================
   const UInt32List* GetFaceVertexCounts() const { return m_pnFaceVertexCounts; }

   //====================================================================================
   /// \brief Sets the required geometry data in the mesh accessor
   //====================================================================================
   bool SetGeometryData ( FCDGeometryPolygons* pPolygons );

protected:

private:

   //====================================================================================
   /// \brief Sets the required position data in the mesh accessor
   //====================================================================================
   bool SetPositionData ( FCDGeometryPolygons* pPolygons );

   //====================================================================================
   /// \brief Sets the required normal data in the mesh accessor
   //====================================================================================
   bool SetNormalData ( FCDGeometryPolygons* pPolygons );

   //====================================================================================
   /// \brief Sets the required texture data in the mesh accessor
   //====================================================================================
   bool SetTextureData ( FCDGeometryPolygons* pPolygons );

   //====================================================================================
   /// \brief Sets the required FaceVertex indices data in the mesh accessor
   //====================================================================================
   void SetFaceVertexCounts ( UInt32List *i_pnFaceVertexCounts )
   {
      m_pnFaceVertexCounts = i_pnFaceVertexCounts;
   }

   // Non-custodial pointers to index data in the FCollad DB.
   UInt32List  *m_pnPositionIndices;
   UInt32List  *m_pnNormalIndices ;
   UInt32List  *m_pnTexCoordIndices;

   // Non-custodial pointers to geometry data in the FCollad DB.
   FloatList   *m_pfPositionList;
   FloatList   *m_pfNormalList;
   FloatList   *m_pfTexCoordList;

   // List of Face/Vertex counts
   UInt32List  *m_pnFaceVertexCounts;
};

#endif // 0

#endif // _FCOLLADA_MESH_ACCESSOR_H_
