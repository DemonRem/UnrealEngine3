//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class AMDIColladaLibrary
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/IColladaLibrary.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _AMD_ICOLLADA_LIBRARY_H_
#define _AMD_ICOLLADA_LIBRARY_H_

#include "AMDColladaUtils.h"
#include "ArgDefines.h"

class AMDColladaMeshAccessor;

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief Abstract Interface class that prescribes the interface any Collada Library must implement. The 
/// cinterface supports the loading and accessing of a COLLADA doc.
/// 
//=========================================================================================================
class AMDIColladaLibrary
{
public:

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual ~AMDIColladaLibrary()
   {
   };

   //====================================================================================
   /// \brief Loads a Collada file..
   //====================================================================================
   virtual bool LoadColladaFile( const char8 *  pFileName,
                                 bool           bDeIndex ) = 0;

   //====================================================================================
   /// \brief Returns the number of meshes that were loaded.
   //====================================================================================
   virtual uint32 GetMeshAccessorCount() const = 0;

   //====================================================================================
   /// \brief Returns a pointer to a specific accessor. 
   //====================================================================================
   virtual const AMDColladaMeshAccessor * GetMeshAccessor( uint32 nMeshIndex ) const = 0;

   //====================================================================================
   /// \brief Prints the Collada geometry data in a human readable form.
   //====================================================================================
   virtual void DumpGeometryToFile( const char8 * pStrFileName ) const = 0;

   //====================================================================================
   /// \brief Extracts the Collada mesh data into RM friendly accessors.
   //====================================================================================
   virtual bool ExtractMeshes() = 0;

   //====================================================================================
   /// \brief Creates a Collada file..
   //====================================================================================
   virtual bool CreateColladaFile( const char * pFileName ) = 0;

   //====================================================================================
   /// \brief Save the current COLLADA doc.
   //====================================================================================
   virtual bool SaveColladaFile() = 0;

   //====================================================================================
   /// Write out a new set of indices for a specific mesh (by index).
   //====================================================================================
   virtual bool OverWriteIndices( unsigned int     nMeshIndex,
                                  unsigned int *   nDataArray,
                                  unsigned int     nIndicesCount,
                                  unsigned int     nTriangleCount,
                                  const char *     pSemantic ) = 0;

   //====================================================================================
   /// \brief Save the current doc to file using input name as the file name.
   //====================================================================================
   virtual bool SaveAs( char *   pNewName,
                        bool     bReplace ) = 0;

private:

};


#endif // _SU_AMDICOLLADALIBRARY_H_
