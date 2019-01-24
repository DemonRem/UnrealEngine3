//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmColladaDoc
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/AMDColladaDoc.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished) 
//=================================================================================================================================

#ifndef _AMD_COLLADA_DOC_H_
#define _AMD_COLLADA_DOC_H_

#include <vector>

#include "IColladaLibrary.h"
#include "ColladaMeshAccessor.h"
#include "AMDColladaUtils.h"

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief This is a simple bridge pattern that supports the use of different Collada libraries to load 
/// and manipulate Collada data.  The constructor requires a Collada Library to work with (DOM or FCollada).  
/// The Collada Library is a on object wrapper around the actual support code for the library and a specific
/// Collada XML document. Public methods are passed onto the library object for a specific implementation.
//=========================================================================================================
class AMDColladaDoc
{

public:

   typedef enum
   {
      DOM_COLLADA_LIB,
      FCOLLADA_COLLADA_LIB
   }                                COLLADA_LIB_TYPE;

   //====================================================================================
   /// \brief Constructor - Takes a Collada Library and print and error print functions.
   //====================================================================================
   AMDColladaDoc( AMDIColladaLibrary * i_pColladaLib,
                  void                 ( * i_pPrintFunction )( const char * ) = NULL,
                  void                 ( * i_pErrorFunction )( const char * ) = NULL );

   //====================================================================================
   /// \brief Constructor - Takes a Collada lib type and print and error print functions.
   //====================================================================================
   AMDColladaDoc( COLLADA_LIB_TYPE  eLibType,
                  void              ( * i_pPrintFunction )( const char * ) = NULL,
                  void              ( * i_pErrorFunction )( const char * ) = NULL );

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   ~AMDColladaDoc();

   //====================================================================================
   /// \brief Loads a Collada file..
   //====================================================================================
   bool LoadColladaFile( const char8 * pFileName,
                         bool          bDeIndex );

   //====================================================================================
   /// \brief Creates a new Collada file..
   //====================================================================================
   bool CreateColladaFile( const char8 * pFileName );

   //====================================================================================
   /// \brief Returns the number os sub-meshes in the document.
   //====================================================================================
   uint32 GetMeshAccessorCount() const;

   //====================================================================================
   /// \brief Returns a pointer to a mesh accessor. The accessor supports acces to the
   /// mesh data.
   //====================================================================================
   const AMDColladaMeshAccessor *   GetMeshAccessor( uint32 nMeshIndex ) const;

   //====================================================================================
   /// \brief Writes the mesh data out in human readable form.
   //====================================================================================
   void DumpGeometryToFile( const char8 * pStrFileName ) const;

   //====================================================================================
   /// \brief Extracts the Collada mesh data into RM friendly accessors.
   //====================================================================================
   bool ExtractMeshes();

   //====================================================================================
   /// \brief Save the current COLLADA doc.
   //====================================================================================
   bool SaveColladaFile();

   //====================================================================================
   /// Write out a new set of indices for a specific mesh (by index).
   //====================================================================================
   bool OverWriteIndices( unsigned int    nMeshIndex,
                          unsigned int *  nDataArray,
                          unsigned int    nIndicesCount,
                          unsigned int    nTriangleCount,
                          const char *    pSemantic );

   //====================================================================================
   /// \brief Save the current doc to file using input name as the file name.
   //====================================================================================
   bool SaveAs( char *  pNewName,
                bool    bReplace );

private:

   // Pointer to the Collada Library wrapper object.
   AMDIColladaLibrary *             m_pColladaLib;

};

#endif // _AMD_COLLADA_DOC_H_
