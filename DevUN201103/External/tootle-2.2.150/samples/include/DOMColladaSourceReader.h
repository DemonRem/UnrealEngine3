//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class DOMColladaSourceReader
// Description: The source reader extracts one stream of geometry input data into a local vector. The type is 
// specific to the input data (e.g. float2, float3 etc). The source reader knows what type of data it has.
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/DOMColladaSourceReader.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _DOM_COLLADA_SOURCE_READER_H_
#define _DOM_COLLADA_SOURCE_READER_H_

#include <vector>

class domSource;

//=========================================================================================================
/// \brief Struct for single float
//=========================================================================================================
struct vf1
{
   vf1::vf1( double dVal )
   {
      data[0] = dVal;
   }
   double data[1];
};

//=========================================================================================================
/// \brief Struct for two floats
//=========================================================================================================
struct vf2
{
   vf2::vf2( double  dVal0,
             double  dVal1 )
   {
      data[0] = dVal0;
      data[1] = dVal1;
   }
   double data[2];
};

//=========================================================================================================
/// \brief Struct for three floats
//=========================================================================================================
struct vf3
{
   vf3::vf3( double  dVal0,
             double  dVal1,
             double  dVal2 )
   {
      data[0] = dVal0;
      data[1] = dVal1;
      data[2] = dVal2;
   }
   double data[3];
};

//=========================================================================================================
/// \brief Struct for four floats
//=========================================================================================================
struct vf4
{
   vf4::vf4( double  dVal0,
             double  dVal1,
             double  dVal2,
             double  dVal3 )
   {
      data[0] = dVal0;
      data[1] = dVal1;
      data[2] = dVal2;
      data[3] = dVal3;
   }
   double data[4];
};

//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief The source reader is responsible for makeing a local copy of the raw COLLADA stream input.
//=========================================================================================================
class DOMColladaSourceReader
{
public:

   enum ArrayType
   {
      None, Float1, Float2, Float3, Float4
   };

   //=========================================================================================================
   /// \brief Constructor
   //=========================================================================================================
   DOMColladaSourceReader::DOMColladaSourceReader( domSource * src );

   //=========================================================================================================
   /// \brief Destructor
   //=========================================================================================================
   DOMColladaSourceReader::~DOMColladaSourceReader()
   {
      m_f1_array.clear();
      m_f2_array.clear();
      m_f3_array.clear();
      m_f4_array.clear();
   }

   //=========================================================================================================
   /// \brief Return the type of the array (Float1, 2, 3, etc)
   //=========================================================================================================
   ArrayType getArrayType() const
   {
      return m_array_type;
   };

   //=========================================================================================================
   /// \brief Return float1 array
   //=========================================================================================================
   std::vector< vf1 > * getFloat1Array()
   {
      return &m_f1_array;
   };
   //=========================================================================================================
   /// \brief Return float2 array
   //=========================================================================================================
   std::vector< vf2 > * getFloat2Array()
   {
      return &m_f2_array;
   };
   //=========================================================================================================
   /// \brief Return float3 array
   //=========================================================================================================
   std::vector< vf3 > * getFloat3Array()
   {
      return &m_f3_array;
   };
   //=========================================================================================================
   /// \brief Return float4 array
   //=========================================================================================================
   std::vector< vf4 > * getFloat4Array()
   {
      return &m_f4_array;
   };

   //=========================================================================================================
   /// \brief Return the number of elements in a source
   //=========================================================================================================
   unsigned int GetCount() const
   {
      return m_nCount;
   };

   //=========================================================================================================
   /// \brief Return the value at index.
   //=========================================================================================================
   vf1 const & GetF1( int nIndex )
   {
      if ( m_array_type != Float1 )
      {
         // Handle error
      }
      return m_f1_array[nIndex];
   }

   //=========================================================================================================
   /// \brief Return the value at index.
   //=========================================================================================================
   vf2 const & GetF2( int nIndex )
   {
      if ( m_array_type != Float2 )
      {
         // Handle error
      }
      return m_f2_array[nIndex];
   }

   //=========================================================================================================
   /// \brief Return the value at index.
   //=========================================================================================================
   vf3 const & GetF3( int nIndex )
   {
      if ( m_array_type != Float3 )
      {
         // Handle error
      }
      return m_f3_array[nIndex];
   }

   //=========================================================================================================
   /// \brief Return the value at index.
   //=========================================================================================================
   vf4 const & GetF4( int nIndex )
   {
      if ( m_array_type != Float4 )
      {
         // Handle error
      }
      return m_f4_array[nIndex];
   }

   //=========================================================================================================
   /// \brief Set the semantic name of the source
   //=========================================================================================================
   void SetSemantic( std::string & strSemantic )
   {
      m_strSemantic = strSemantic;
   }

   //=========================================================================================================
   /// \brief Return the stride of the source. 
   //=========================================================================================================
   unsigned int GetStride()
   {
      return m_nStride;
   }

   //=========================================================================================================
   /// \brief Set the offset of the data within the P array (interleaved data).
   //=========================================================================================================
   void SetIndexMapOffset( unsigned int nIndexMapOffset )
   {
      m_nIndexMapOffset = nIndexMapOffset;
   }

   //=========================================================================================================
   /// \brief Return the Interleaved data offset.
   //=========================================================================================================
   unsigned int GetIndexMapOffset()
   {
      return m_nIndexMapOffset ;
   }

private:

   ArrayType            m_array_type;
   unsigned int         m_nCount;

   std::vector< vf1 >   m_f1_array;
   std::vector< vf2 >   m_f2_array;
   std::vector< vf3 >   m_f3_array;
   std::vector< vf4 >   m_f4_array;

   std::string          m_strSemantic;

   unsigned int         m_nStride;

   unsigned int         m_nIndexMapOffset;
};

#endif //_DOM_COLLADA_SOURCE_READER_H_
