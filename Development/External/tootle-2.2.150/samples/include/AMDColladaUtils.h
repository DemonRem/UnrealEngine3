//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmColladaLoaderUtils
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/AMDColladaUtils.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished) 
//=================================================================================================================================

#ifndef _AMD_COLLADA_UTILS_H_
#define _AMD_COLLADA_UTILS_H_

#define PVERIFY(ptr) \
   if (!ptr) {  Printer::Instance()->PrintVerifyString( #ptr, __FILE__, __LINE__ ); return false; }

#define PVERIFY_RETURN(ptr) \
   if (!ptr) {  Printer::Instance()->PrintVerifyString( #ptr, __FILE__, __LINE__ ); return; }

#include <vector>
#include <map>
#include <sstream>

#include "ArgDefines.h"

// Use for default code
//#define USE_RT

//=========================================================================================================
/// \ingroup AMDColladaLib 
/// \brief Singleton class to support all print function
//=========================================================================================================
class Printer
{

public:

   static Printer *  Instance()
   {
      if ( m_pInstance == 0 )  // is it the first call?
      {
         m_pInstance = new Printer; // create sole instance
      }
      return m_pInstance; // address of sole instance
   }

   static void Destroy()
   {
      if ( m_pInstance != NULL )
      {
         delete m_pInstance;
         m_pInstance = NULL;
      }
   }

   void SetPrintFunction( void ( * i_pPrintFunction )( const char * ) )
   {
      m_pPrintFunction = i_pPrintFunction;
   }

   void SetErrorFunction( void ( * i_pErrorFunction )( const char * ) )
   {
      m_pErrorFunction = i_pErrorFunction;
   }

   void PrintString( const char * pStr ) const
   {
      PVERIFY_RETURN ( pStr );

      if ( m_pErrorFunction )
      {
         std::string str1 ( "COLLADA Library: " );
         std::string str2( pStr );
         m_pPrintFunction( str1.append( str2 ).c_str() );
      }
   }

   void PrintString( std::string & str )
   {
      PrintString( str.c_str() );
   }

   void ErrorString( const char * pStr ) const
   {
      PVERIFY_RETURN ( pStr );

      if ( m_pErrorFunction )
      {
         std::string str1 ( "COLLADA Library: Error: " );
         std::string str2( pStr );
         m_pErrorFunction( str1.append( str2 ).c_str() );
      }
   }

   void ErrorString( const char *   pStr1,
                     const char *   pStr2 ) const
   {
      ErrorString( ( std::string( pStr1 ).append( std::string( pStr2 ) ) ).c_str() );
   }

   void ErrorString( const char *   pStr,
                     unsigned int   nValue ) const
   {
      std::string str1( pStr );

      std::string strValue;
      std::ostringstream o;
      o << nValue;
      strValue = o.str();

      ErrorString( str1.append( strValue ).c_str() );
   }

   //=========================================================================================================
   /// Prints a formatted string to the RM console to support pointer error detection macro.
   ///
   /// \param pStrError Error string.
   /// \param pFile     Filename.
   /// \param nLine     Line number. 
   ///
   //=========================================================================================================
   void PrintVerifyString( const char *   pStrError,
                           const char *   pFile,
                           long           nLine ) const
   {
      std::string error( pStrError );
      std::string file( pFile );
      std::string line ( " is NULL: Line: " );

      std::ostringstream o;
      o << nLine;
      std::string linenum = o.str();

      ErrorString( ( error.append( file ).append( line ).append( linenum ) ).c_str() );
   }

protected:

   Printer( void  ( * i_pPrintFunction )( const char * ) = NULL,
            void  ( * i_pErrorFunction )( const char * ) = NULL )
   : m_pPrintFunction ( i_pPrintFunction ),
     m_pErrorFunction ( m_pErrorFunction )
   {
   }

private:

   static Printer *  m_pInstance;

   void ( * m_pPrintFunction )( const char * );
   void ( * m_pErrorFunction )( const char * );
};

// 3D Vector ( for posisiton and normals )
struct ObjVertex3D
{
   float32  fX;
   float32  fY;
   float32  fZ;
}; // End of ObjVertex3D

// 2D Vector ( for tetxure coordinates )
struct ObjVertex2D
{
   float32  fX;
   float32  fY;
}; // End of ObjVertex2D

// Final Vertex Structure 
struct ObjVertexFinal
{
   ObjVertexFinal()
   {
      pos.fX = 0.0f;
      pos.fY = 0.0f;
      pos.fZ = 0.0f;

      normal.fX = 0.0f;
      normal.fY = 0.0f;
      normal.fZ = 0.0f;

      texCoord.fX = 0.0f;
      texCoord.fY = 0.0f;
   }; // End of Constructor

   ObjVertex3D pos;
   ObjVertex3D normal;
   ObjVertex2D texCoord;
}; // End of ObjVertexFinal

// Face
struct ObjFace
{
   ObjFace()
   {
      int nIndex;
      for ( nIndex = 0; nIndex < 3; nIndex++ )
      {
         nVertexIndices[nIndex] = 0;
         nTexCoordIndices[nIndex] = 0;
         nNormalIndices[nIndex] = 0;
         nFinalVertexIndices[nIndex] = 0;
      } // End for
   }; // End of ObjFace

   int nVertexIndices[3];
   int nTexCoordIndices[3];
   int nNormalIndices[3];

   // This is the index used to for rendering
   int nFinalVertexIndices[3];
}; // End of ObjFace

// VertexHashData
struct VertexHashData
{
   int   nVertexIndex;
   int   nTexCoordIndex;
   int   nNormalIndex;
   int   nFinalIndex;
}; // End of VertexHashData

//====================================================================================
/// \brief Function used in hash map to compare vertex positions.
//====================================================================================
struct vertex_less : public std::less< VertexHashData >
{
   bool operator()( const VertexHashData &   x,
                    const VertexHashData &   y ) const
   {
      if ( x.nVertexIndex < y.nVertexIndex )
      {
         return true;
      }

      if ( x.nVertexIndex > y.nVertexIndex )
      {
         return false;
      }

      if ( x.nTexCoordIndex < y.nTexCoordIndex )
      {
         return true;
      }

      if ( x.nTexCoordIndex > y.nTexCoordIndex )
      {
         return false;
      }

      if ( x.nNormalIndex < y.nNormalIndex )
      {
         return true;
      }

      if ( x.nNormalIndex > y.nNormalIndex )
      {
         return false;
      }

      return false;
   }; // End of operator()
}; // End of vertex_less


//====================================================================================
/// \brief Enumerations used to differentiate between Common and GLSL COLLADA elements.
//====================================================================================
enum ProfileType
{
   HELPER_GLSL, HELPER_COMMON
};

//====================================================================================
/// \brief Enumerations for the various sampler types. 
//====================================================================================
enum SurfaceType
{
   HELPER_1D, HELPER_2D, HELPER_3D, HELPER_RECT, HELPER_CUBE, HELPER_DEPTH
};

//====================================================================================
/// \brief Enumerations for camera projections.
//====================================================================================
enum DOMHelperProjectionMode
{
   DOMHELPER_PERSPECTIVE, DOMHELPER_ORTHOGRAPHIC
};

//====================================================================================
/// \brief This struct is used to import texture state setting. Note the settings should
/// already be defined using COLLADA enums - NOT RM enums.
//====================================================================================
typedef struct
{
   unsigned long  nStateType;
   unsigned long  nStateValue;
   float          fStateValue;
}                                                  DOMHelperTextureStateType;

//====================================================================================
/// \brief Used to import texture states into the helper.
//====================================================================================
typedef std::vector< DOMHelperTextureStateType >   DOMHelperTextureStateVec;


//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief This class is the mechanism used to import camera data into the DOMHelper. The camera type should
/// be accessible to all users of te DOMHelper.
/// 
//=========================================================================================================
class DOMHelperCamera
{

public:

   DOMHelperCamera( float        fFieldOfView,
                    float        fNearClip,
                    float        fFarClip,
                    const char * pName,
                    bool         bMainViewLock )
   : m_fFieldOfView ( fFieldOfView ),
     m_fNearClip ( fNearClip ),
     m_fFarClip ( fFarClip ),
     m_bMainViewLock ( bMainViewLock )
   {
      SetName( pName );
      // Default to perspective
      m_nProjectionMode = DOMHELPER_PERSPECTIVE;
   }

   void SetProjectionMode( DOMHelperProjectionMode eMode )
   {
      m_nProjectionMode = eMode;
   }

   void SetMainViewLock( bool bLock )
   {
      m_bMainViewLock = bLock;
   }

   void SetName( const char * pName )
   {
      strcpy_s( &m_Name[0], 256, pName );
   }

   void SetPosition( double   fX,
                     double   fY,
                     double   fZ )
   {
      m_fPosition[0] = fX;
      m_fPosition[1] = fY;
      m_fPosition[2] = fZ;

   }

   void SetLookAt( double  fX,
                   double  fY,
                   double  fZ )
   {
      m_fLookAt[0] = fX;
      m_fLookAt[1] = fY;
      m_fLookAt[2] = fZ;
   }

   void SetUp( double   fX,
               double   fY,
               double   fZ )
   {
      m_fUp[0] = fX;
      m_fUp[1] = fY;
      m_fUp[2] = fZ;
   }

   DOMHelperProjectionMode GetProjectionMode()
   {
      return m_nProjectionMode;
   }

   float GetFieldOfView()
   {
      return m_fFieldOfView;
   }

   float GetNearClipPlane()
   {
      return m_fNearClip;
   }

   float GetFarClipPlane()
   {
      return m_fFarClip;
   }

   bool GetMainViewLock()
   {
      return m_bMainViewLock;
   }

   char *                  GetName()
   {
      return &m_Name[0];
   }

   double *                GetPosition()
   {
      return &m_fPosition[0];
   }

   double *                GetLookAt()
   {
      return &m_fLookAt[0];
   }

   double *                GetUp()
   {
      return &m_fUp[0];
   }

private:

   DOMHelperProjectionMode m_nProjectionMode;
   float                   m_fFieldOfView;
   float                   m_fNearClip;
   float                   m_fFarClip;
   bool                    m_bMainViewLock;
   char m_Name[256];

   double m_fPosition[3];
   double m_fLookAt[3];
   double m_fUp[3];
};

class daeElement;
class daeURI;
daeElement *                                       getElementFromURI( daeURI & uri );

#endif // _AMD_COLLADA_UTILS_H_
