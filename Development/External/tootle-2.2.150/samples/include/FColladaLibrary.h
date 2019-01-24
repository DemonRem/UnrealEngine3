//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmFColladaLibrary
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/FColladaLibrary.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _SU_RMFCOLLADALIBRARY_H_
#define _SU_RMFCOLLADALIBRARY_H_

#if 0

#include "IColladaLibrary.h"
#include "FColladaMeshAccessor.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUFileManager.h"


//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief This class is responsible for supporting the FCollada Library from Feeling Software.
/// 
//=========================================================================================================
class CRmFColladaLibrary : public AMDIColladaLibrary
{
public:

   //====================================================================================
   /// \brief Constructor
   //====================================================================================
   CRmFColladaLibrary();

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual ~CRmFColladaLibrary();

   //====================================================================================
   /// \brief Create a new COLLADA file with the input name.
   //====================================================================================
   bool CreateColladaFile( const char *pFileName );

   //=========================================================================================================
   /// Save the current COLLADA doc.
   ///
   /// \return True if no error.
   //=========================================================================================================
   bool SaveColladaFile();

   //====================================================================================
   /// \brief Loads a Collada file and supplies a print function for feedback.
   //====================================================================================
   virtual bool LoadColladaFile( const char8 *pFileName, bool bDeIndexer );

   //====================================================================================
   /// \brief Returns the number of meshes stored.
   //====================================================================================
   virtual uint32 GetMeshAccessorCount() const
   {
      return (uint32)m_MeshAccessors.size();
   }

   //====================================================================================
   /// \brief Returns a pointer to a specific accessor. 
   //====================================================================================
   virtual const FColladaMeshAccessor *GetMeshAccessor ( uint32 nMeshIndex ) const;

   //=========================================================================================================
   /// \brief Prints the Collada geometry data in a human readable form.
   //=========================================================================================================
   virtual void DumpGeometryToFile ( const char8 * pStrFileName ) const;

   //====================================================================================
   /// \brief Extracts the Collada mesh data into RM friendly accessors.
   //====================================================================================
   virtual bool ExtractMeshes();

   //====================================================================================
   /// Write out a new set of indices for a specific mesh (by index).
   //====================================================================================
   virtual bool OverWriteIndices ( unsigned int    nMeshIndex,
                           unsigned int *  nDataArray,
                           unsigned int    nIndicesCount,
                           unsigned int    nTriangleCount,
                           const char   *  pSemantic ) ;

   //====================================================================================
   /// \brief Save the current doc to file using input name as the file name.
   //====================================================================================
   virtual bool SaveAs( char * pNewName, bool bReplace ) ;

protected:

private:

   //====================================================================================
   /// \brief Removes the internal Collada doc and resets the internal state using ResetState().
   //====================================================================================
   void CleanUp();

   //====================================================================================
   /// \brief Resets the internal state ready for the loading of a new Collada document.
   //====================================================================================
   void ResetState();

   //====================================================================================
   /// \brief Start the process to identify the geometry data.
   //====================================================================================
   bool LoadMeshFromDoc();

   //====================================================================================
   /// \brief Triangulate any faces with more than 3 vertices.    
   //====================================================================================
   bool TriangulateGeometry( FCDGeometryLibrary* pLibrary );

   //====================================================================================
   /// \brief Triangulate any faces with more than 3 vertices.
   //====================================================================================
   void TriangulateGeometry( FCDGeometryMesh* pMesh );

   //====================================================================================
   /// \brief Identify geometry sources in the Library part of the document.
   //====================================================================================
   bool LoadGeometryLibrary( FCDGeometryLibrary* pLibrary );

   //====================================================================================
   /// \brief Identify geometry sources in the mesh.    
   //====================================================================================
   bool LoadGeometryMesh( FCDGeometryMesh* pMesh );

   //====================================================================================
   /// \brief Print a string object to the user supplied print function.
   //====================================================================================
   void PrintFColladaString ( const string strMessage ) const;


   // Pointer to the loaded Collada doc.
   FCDocument  *m_pFCDoc;

   // List of mesh accessor (1 per sub-mesh)
   std::vector <FColladaMeshAccessor> m_MeshAccessors ;

};

#endif

#endif // _SU_RMFCOLLADALIBRARY_H_
