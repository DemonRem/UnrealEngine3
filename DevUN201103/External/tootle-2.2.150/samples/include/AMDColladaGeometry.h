//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class AMDColladaGeometry
//
// Description: This is a helper class that that maps COLLADA geometry data into a format that is directly
// accessible to user applications.
// It stores source reader objects. A source reader exists for each input stream in COLLADA geometry.
// For example a source input exists for positions, normals, tex-coords etc. It also processes the P array (the 
// indexes into the sources that are used to define the triangles).
//
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/AMDColladaGeometry.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished) 
//=================================================================================================================================

#ifndef _AMD_COLLADA_GEOMETRY_H_
#define _AMD_COLLADA_GEOMETRY_H_

#include <map>

#include "dom/domCOLLADA.h"
#include "DOMColladaSourceReader.h"
#include "AMDColladaUtils.h"

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief Stores collection of sources (source readers) that make up a mesh.
//=========================================================================================================
class AMDColladaGeometry
{
public:

   // Store the source readers by element pointer. An element pointer points to an XML node. 
   typedef std::map< daeElement *,
                     DOMColladaSourceReader * > SourcesByElementMap;
   // Store the source by semantic, e.g. POISTIONS, NORMALS etc.
   typedef std::map< std::string,
                     DOMColladaSourceReader * > SourcesBySemanticMap;
   // Used to store a simple P array. These are the triangle indices.
   typedef std::vector< unsigned int >          IntVec;

   /// Constructor
   AMDColladaGeometry::AMDColladaGeometry();

   /// Destructor
   AMDColladaGeometry::~AMDColladaGeometry();

   //====================================================================================
   /// \brief Processe the input data and creates a new source object.
   //====================================================================================
   void AddSourceElement( daeElement * pElement,
                          domSource *  pSource );

   //====================================================================================
   /// \brief Helper function that locates a COLLADA source element by a semantic name.
   //====================================================================================
   template< typename TInputArray,
             typename TInputType >
      bool findInputSourceBySemantic( TInputArray &   inputs,
                                      const char *    semantic,
                                      daeElement *&   element,
                                      TInputType **   input = NULL,
                                      int             unit = 0 )
      {
         element = NULL;
         int count = 0;
         for ( size_t i = 0; i < inputs.getCount(); i++ )
         {
            if ( !strcmp( semantic, inputs[i]->getSemantic() ) )
            {
               if ( count == unit )
               {
                  element = getElementFromURI( inputs[i]->getSource() );
                  *input = ( TInputType * )inputs[i];
                  return true;
               }
               count++;
            }
         }
         return false;
      }

   //====================================================================================
   /// \brief Main method for extracting triangle data.
   //====================================================================================
   bool ProcessTriangles( domTriangles * pTriangles );

   //====================================================================================
   /// \brief Makes a local copy of the P array
   //====================================================================================
   void ProcessPArray( domP *    p,
                       IntVec &  vPArray );

   //====================================================================================
   /// \brief Finds the semantic inputs and creates source objects for them
   //====================================================================================
   bool ResolveArrays( domInputLocalOffset_Array & inputs,
                       SourcesByElementMap &       sources,
                       IntVec &                    vPArray );

   //====================================================================================
   // \brief Helper function to load a source by semantic.
   //====================================================================================
   template< typename TInputArray >
   bool HandleSourceSemantic( SourcesByElementMap &   rSourceMap,
                              TInputArray &           rInputs,
                              std::string &           rStrSemantic,
                              unsigned int            nOffset );

   //====================================================================================
   /// \brief Returns the source with the input semantic 
   //====================================================================================
   DOMColladaSourceReader *                     GetSourceBySemantic( std::string & rSemantic )
   {
      return m_SourcesBySemanticMap[rSemantic];
   }

   //====================================================================================
   /// \brief Returns the number of faces.
   //====================================================================================
   unsigned int GetFaceCount()
   {
      return m_nFaceCount;
   }

   //====================================================================================
   /// \brief Sets the number of faces.
   //====================================================================================
   void SetFaceCount( unsigned int nFcount )
   {
      m_nFaceCount = nFcount;
   }

   //====================================================================================
   /// \brief Returns the vertex count.
   //====================================================================================
   unsigned int GetVertexCount()
   {
      return m_nVertexCount;
   }

   //====================================================================================
   /// \brief Sets the vertex count.
   //====================================================================================
   void SetVertexCount( unsigned int nVcount )
   {
      m_nVertexCount = nVcount;
   }
   //====================================================================================
   /// \brief Returns the position stride.
   //====================================================================================
   unsigned int GetPositionStride()
   {
      return m_nPositionStride;
   }

   //====================================================================================
   /// \brief Retunrs the normal stride.
   //====================================================================================
   unsigned int GetNormalStride()
   {
      return m_nNormalStride;
   }

   //====================================================================================
   /// \brief Returns the tex coord stride.
   //====================================================================================
   unsigned int GetTexCoordStride()
   {
      return m_nTexCoordStride;
   }

   //====================================================================================
   /// \brief Returns the length of the P array.
   //====================================================================================
   unsigned int GetPArraySize()
   {
      return ( unsigned int )( m_PArray.size() );
   }

   //====================================================================================
   /// \brief  Return the value in the P array at the index.
   //====================================================================================
   unsigned int GetPArrayIndex( int nIndex )
   {
      return m_PArray[nIndex];
   }
   //====================================================================================
   ///  Process an individual source (map its semantic to a source object)
   //====================================================================================
   bool HandleSource(   SourcesByElementMap  &  rSourceMap, 
                        std::string          &  rStrSemantic,
                        unsigned int            nOffset,
                        daeElement           *  pSourceElement );

private:

   // Mapping from source element to its extracted source.
   SourcesByElementMap                          m_SourcesByElementMap;

   // Mapping from semantic name to its extracted source.
   SourcesBySemanticMap                         m_SourcesBySemanticMap;

   IntVec                                       m_PArray;

   unsigned int                                 m_nFaceCount;
   unsigned int                                 m_nVertexCount;
   unsigned int                                 m_nPositionStride;
   unsigned int                                 m_nNormalStride;
   unsigned int                                 m_nTexCoordStride;
};

//=========================================================================================================
/// \brief Templetized method to handle COLLADA source inputs.
//=========================================================================================================
template< typename TInputArray >
bool AMDColladaGeometry::HandleSourceSemantic( SourcesByElementMap & rSourceMap,
                                               TInputArray &         rInputs,
                                               std::string &         rStrSemantic,
                                               unsigned int          nOffset  )
{
   domInputLocal * pTmp;

   daeElement * pSourceElement = NULL;

   // Locate the source element
   if ( findInputSourceBySemantic( rInputs, rStrSemantic.c_str(), pSourceElement, &pTmp ) )
   {
      return HandleSource ( rSourceMap, rStrSemantic, nOffset, pSourceElement );
   }
   return false;
}

#endif // _AMD_COLLADA_GEOMETRY_H_
