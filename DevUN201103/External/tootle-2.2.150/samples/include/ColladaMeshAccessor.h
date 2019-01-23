//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmColladaMeshAccessor
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/ColladaMeshAccessor.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _AMD_COLLADA_MESH_ACCESSOR_H_
#define _AMD_COLLADA_MESH_ACCESSOR_H_

#include <vector>

#include "AMDColladaUtils.h"

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief A simple class that stores data pointers to an individual sub_mesh within a mesh.
//=========================================================================================================
class AMDColladaMeshAccessor
{
public:

   //====================================================================================
   /// \brief Constructor
   //====================================================================================
   AMDColladaMeshAccessor::AMDColladaMeshAccessor()
   {
      ResetState();
   }

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual AMDColladaMeshAccessor::~AMDColladaMeshAccessor()
   {
   }

   //====================================================================================
   /// \brief Loads the vertices into the output vertices array for use externally.
   //====================================================================================
   virtual bool LoadVertices( std::vector< ObjVertex3D > & vertices ) const = 0;

   //====================================================================================
   /// \brief Loads the normals into the output normals array for use externally.
   //====================================================================================
   virtual bool LoadNormals( std::vector< ObjVertex3D > & normals ) const = 0;

   //====================================================================================
   /// \brief Loads the texcoords into the output texCoords array for use externally.
   //====================================================================================
   virtual bool LoadTexCoords( std::vector< ObjVertex2D > & texCoords ) const = 0;

   //====================================================================================
   /// \brief Loads the face indices into the output faces array for use externally.
   //====================================================================================
   virtual bool LoadFaces( std::vector< ObjFace > & faces ) const = 0;

   //====================================================================================
   /// \brief Returns true if the mesh has normal data.
   //====================================================================================
   bool HasPositions() const
   {
      return m_bHasPositions;
   }

   //====================================================================================
   /// \brief Returns true if the mesh has Normal data.
   //====================================================================================
   bool HasNormals() const
   {
      return m_bHasNormals;
   }

   //====================================================================================
   /// \brief Returns true if the mesh has texture coordinate data.
   //====================================================================================
   bool HasTexCoords() const
   {
      return m_bHasTexCoords;
   }

   //====================================================================================
   /// \brief Returns the number of faces in the mesh.
   //====================================================================================
   uint32 GetFaceCount() const
   {
      return m_nFaceCount;
   }

   //====================================================================================
   /// \brief Returns the stride of the positional data. 
   //====================================================================================
   uint32 GetPositionStride() const
   {
      return m_nPositionStride;
   }

   //====================================================================================
   /// \brief Returns the stride of the normal data.
   //====================================================================================
   uint32 GetNormalStride() const
   {
      return m_nNormalStride;
   }

   //====================================================================================
   /// \brief Returns the stride of the texture coordinate data.
   //====================================================================================
   uint32 GetTexCoordStride() const
   {
      return m_nTexCoordStride;
   }

   //====================================================================================
   /// \brief Return the offset associated with the position indices.
   //====================================================================================
   uint32 GetPositionOffset() const
   {
      return m_nPositionOffset;
   }

   //====================================================================================
   /// \brief Return the offset associated with the normal indices.
   //====================================================================================
   uint32 GetNormalOffset() const
   {
      return m_nNormalOffset;
   }

   //====================================================================================
   /// \brief Return the offset associated with the texture indices.
   //====================================================================================
   uint32 GetTextureOffset() const
   {
      return m_nTextureOffset;
   }

protected:

   //====================================================================================
   /// \brief Sets the internal state to default values.
   //====================================================================================
   virtual void ResetState();

   //====================================================================================
   /// \brief Sets the number of faces in the sub-mesh
   //====================================================================================
   void SetFaceCount( uint32 i_nFaceCount )
   {
      m_nFaceCount = i_nFaceCount;
   }

   //====================================================================================
   /// \brief Sets the stride between normals.
   //====================================================================================
   void SetNormalStride( uint32 i_nNormalStride )
   {
      m_nNormalStride = i_nNormalStride;
   }

   //====================================================================================
   /// \brief Sets the stride between tex coords.
   //====================================================================================
   void SetTexCoordStride( uint32 i_nTexCoordStride )
   {
      m_nTexCoordStride = i_nTexCoordStride;
   }

   //====================================================================================
   /// \brief Sets the stride between positions.
   //====================================================================================
   void SetPositionStride( uint32 i_nPositionStride )
   {
      m_nPositionStride = i_nPositionStride;
   }

   //====================================================================================
   /// \brief Returns true if the accessor has positional data.
   //====================================================================================
   void SetHasPositions( bool i_bState )
   {
      m_bHasPositions = i_bState;
   }

   //====================================================================================
   /// \brief Returns true if the accessor has normal data.
   //====================================================================================
   void SetHasNormals( bool i_bState )
   {
      m_bHasNormals = i_bState;
   }

   //====================================================================================
   /// \brief Returns true if the accessor has tex coord data.
   //====================================================================================
   void SetHasTexCoords( bool i_bState )
   {
      m_bHasTexCoords = i_bState;
   }

   //====================================================================================
   /// \brief Returns the offset for the position indices
   //====================================================================================
   void SetPositionOffset( uint32 nOffset )
   {
      m_nPositionOffset = nOffset;
   }

   //====================================================================================
   /// \brief Returns the offset for the normal indices
   //====================================================================================
   void SetNormalOffset( uint32 nOffset )
   {
      m_nNormalOffset = nOffset;
   }

   //====================================================================================
   /// \brief Returns the offset for the texture indices
   //====================================================================================
   void SetTextureOffset( uint32 nOffset )
   {
      m_nTextureOffset = nOffset;
   }

private:

   // Bools to record what data is present in the FCollada DB.
   bool     m_bHasPositions;
   bool     m_bHasNormals;
   bool     m_bHasTexCoords;

   // Records the number of faces in the mesh.
   uint32   m_nFaceCount;
   uint32   m_nPositionStride;
   uint32   m_nNormalStride;
   uint32   m_nTexCoordStride;

   uint32   m_nPositionOffset;
   uint32   m_nNormalOffset;
   uint32   m_nTextureOffset;

};

#endif // _AMD_COLLADA_MESH_ACCESSOR_H_
