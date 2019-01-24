//=================================================================================================================================
//
// Author: Gordon Selley
//         3D Application Research Group
//         ATI Research, Inc.
//
// Definition of class RmDOMColladaLibrary
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Common/Collada/AMDColladaLib/Source/DOMColladaLibrary.h#1 $ 
// 
// Last check-in:  $DateTime: 2009/06/03 12:02:18 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//=================================================================================================================================
//   * Copyright (c) 2007 Advanced Micro Devices, Inc. (unpublished)
//   * Copyright (c) 2007 ATI Technologies Inc. (unpublished)
//=================================================================================================================================

#ifndef _DOM_COLLADA_LIBRARY_H_
#define _DOM_COLLADA_LIBRARY_H_

#include "IColladaLibrary.h"
#include "DOMColladaMeshAccessor.h"
#include "dom/domTypes.h"
#include "AMDColladaUtils.h"
#include "dom/domCOLLADA.h"
#include "DOMColladaSourceReader.h"
#include "AMDColladaGeometry.h"

// COLLADA DOM header. Allows us to work on a DAE file through the DOM interface.
#include "dom/domConstants.h"
#include "dom/domCOLLADA.h"
#include "dom/domGeometry.h"
#include "dom/domTypes.h"
#include "dom/domElements.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domProfile_GLSL.h"
#include "dom/domFloat_array.h"

class DAE;
class domMesh;
class domCOLLADA;
class CrtScene;
class CrtGeometry;
class CrtTriangles;
class daeElement;
class domFloat_array;
class domInputLocal;
class domInputLocalOffset;
class domInstance_material;
class domGl_sampler1D;
class RmScene;
class DOMColladaSourceReader;


const uint32   nMaxPathSize = 1024;
const uint32   nCoversionStringMax = 1024;


//=========================================================================================================
/// \ingroup AMDColladaLib
/// \brief This class is responsible for supporting the COLLADA_DOM library interface to COLLADA docs. 
/// 
//=========================================================================================================
class DOMColladaLibrary : public AMDIColladaLibrary
{
public:

   typedef std::vector< AMDColladaGeometry * >  Geometries;

   typedef std::vector <char *> StringCache;

   //====================================================================================
   /// \brief Constructor
   //====================================================================================
   DOMColladaLibrary();

   //====================================================================================
   /// \brief Destructor
   //====================================================================================
   virtual ~DOMColladaLibrary();

   //====================================================================================
   /// \brief Loads a Collada file.
   //====================================================================================
   virtual bool LoadColladaFile( const char8 *  pFileName,
                                 bool           bDeIndex );

   //====================================================================================
   /// \brief Create a new COLLADA file.
   //====================================================================================
   virtual bool CreateColladaFile( const char * pFileName );

   //====================================================================================
   /// \brief Returns the number of meshes that were loaded.
   //====================================================================================
   virtual uint32 GetMeshAccessorCount() const
   {
      return ( uint32 )m_MeshAccessors.size();
   }

   //====================================================================================
   /// \brief Returns a pointer to a specific accessor. 
   //====================================================================================
   virtual const DOMColladaMeshAccessor *       GetMeshAccessor( uint32 nMeshIndex ) const;

   //====================================================================================
   /// \brief Prints the Collada geometry data in a human readable form.
   //====================================================================================
   virtual void DumpGeometryToFile( const char * pStrFileName ) const;

   //====================================================================================
   /// \brief Extracts the Collada mesh data into RM friendly accessors.
   //====================================================================================
   virtual bool ExtractMeshes();

   //====================================================================================
   /// \brief Return the pointer to the DAE
   //====================================================================================
   DAE *                                        GetDAE() const
   {
      return m_pDAE;
   }

   //====================================================================================
   /// \brief Return the pointer to the DOM
   //====================================================================================
   domCOLLADA *                                 GetDOM() const
   {
      return m_pDOM;
   }

   //====================================================================================
   /// \brief Print a string to the print function pointer.
   //====================================================================================
   void PrintString( const char * pStr ) const;

   //====================================================================================
   /// \brief Print a string to the error print function pointer.
   //====================================================================================
   void ErrorString( const char * pStr ) const;

   //====================================================================================
   /// \brief Print a string to the error print function pointer.
   //====================================================================================
   void ErrorString( const char *   pStr1,
                     const char *   pStr2 ) const;

   //====================================================================================
   /// \brief Print a string to the error print function pointer.
   //====================================================================================
   void ErrorString( const char *   pStr1,
                     unsigned int   nValue ) const;

   //====================================================================================
   /// \brief Print a string to the error print function pointer (used in the PVERIFY macros).
   //====================================================================================
   void PrintVerifyString( const char *   pStrError,
                           const char *   pFile,
                           long           nLine ) const;

   //====================================================================================
   /// \brief Save the current doc to file.
   //====================================================================================
   bool SaveColladaFile();

   //====================================================================================
   /// \brief Save the current doc to file using input name as the file name.
   //====================================================================================
   bool SaveAs( char *  pNewName,
                bool    bReplace );

   //====================================================================================
   /// \brief Render Target Enumeration.
   //====================================================================================
   enum RenderTargetType
   {
      COLOR_TARGET, DEPTH_TARGET, STENCIL_TARGET
   };

   //====================================================================================
   /// \brief Clear Target Enumeration.
   //====================================================================================
   enum ClearTargetType
   {
      CLEAR_COLOR, CLEAR_DEPTH, CLEAR_STENCIL
   };

   //====================================================================================
   /// \brief Adds a new GLSL effect element.
   //====================================================================================
   daeElement *                                 AddGLSLEffect( daeElement *   pParent,
                                                               const char *   pElementSID,
                                                               float          fTimeCycle ) const;

   //====================================================================================
   /// \brief Adds a new GLSL code element.
   //====================================================================================
   daeElement *                                 AddGLSLCodeElement( daeElement * pParent,
                                                                    const char * pElementSID,
                                                                    const char * pCode ) const;

   //====================================================================================
   /// \brief Adds a new GLSL profile element.
   //====================================================================================
   daeElement *                                 AddGLSLProfile( daeElement *  pParent,
                                                                const char *  pElementSID ) const;

   //====================================================================================
   /// \brief Adds a new Common profile element.
   //====================================================================================
   daeElement *                                 AddCommonProfile( daeElement *   pParent,
                                                                  const char *   pElementSID,
                                                                  const char *   pTechniqueName ) const;

   //====================================================================================
   /// \brief Adds a GLSL NewParam element.
   //====================================================================================
   daeElement *                                 AddGLSLNewParam( daeElement * pParent,
                                                                 const char * pElementSID ) const;

   //====================================================================================
   /// \brief Adds a GLSL Technique element.
   //====================================================================================
   daeElement *                                 AddGLSLTechnique( daeElement *   pParent,
                                                                  const char *   pElementSID ) const;

   //====================================================================================
   /// \brief Adds a Common Technique element.
   //====================================================================================
   daeElement *                                 AddCommonTechnique( daeElement * pParent,
                                                                    const char * pElementSID ) const;

   //====================================================================================
   /// \brief Adds a GLSL Pass element.
   //====================================================================================
   daeElement *                                 AddGLSLPass( daeElement *  pParent,
                                                             const char *  pElementSID ) const;

   //====================================================================================
   /// \brief Adds a GLSL Shader element.
   //====================================================================================
   daeElement *                                 AddGLSLShader( daeElement *            pParent,
                                                               domGlsl_pipeline_stage  eStage,
                                                               const char *            pCompilerTarget,
                                                               const char *            pSourceRef,
                                                               const char *            pCodeEntryPoint,
                                                               bool                    bReUseExistingShader = false ) const;

   //====================================================================================
   /// \brief Adds a GLSL compiler element.
   //====================================================================================
   bool AddGLSLCompilerTarget( daeElement *  pParent,
                               const char *  pTargetString ) const;

   //====================================================================================
   /// \brief Sets a GPU program source reference and code entry point element.
   //====================================================================================
   bool AddGLSLSourceReference( daeElement * pParent,
                                const char * pSourceRef,
                                const char * pCodeEntryPoint ) const;

   //====================================================================================
   /// \brief Adds a bind element that binds an internal reference to a uniform name.
   //====================================================================================
   daeElement *                                 AddGLSLBinding( daeElement *  pParent,
                                                                const char *  pBindSymbol,
                                                                const char *  pBindReference ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to 3 floats.
   //====================================================================================
   bool SetGLSLParamValueF3( daeElement * pParent,
                             float        f1,
                             float        f2,
                             float        f3 ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to 2 floats.
   //====================================================================================
   bool SetGLSLParamValueF2( daeElement * pParent,
                             float        f1,
                             float        f2 ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to 4 floats.
   //====================================================================================
   bool SetGLSLParamValueF4( daeElement * pParent,
                             float        f1,
                             float        f2,
                             float        f3,
                             float        f4 ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to input matrix.
   //====================================================================================
   bool SetGLSLParamValueF4x4( daeElement *  pParent,
                               float         fInputMatrix[] ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to a float.
   //====================================================================================
   bool SetGLSLParamValueF1( daeElement * pParent,
                             float        fInputValue ) const;

   //====================================================================================
   /// \brief Sets the values of a GLSL NewParam to an int.
   //====================================================================================
   bool SetGLSLParamValueI1( daeElement * pParent,
                             int          nInputValue ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueI2( daeElement * pParent,
                             int          nVal1,
                             int          nVal2 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueI3( daeElement * pParent,
                             int          nVal1,
                             int          nVal2,
                             int          nVal3 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueI4( daeElement * pParent,
                             int          nVal1,
                             int          nVal2,
                             int          nVal3,
                             int          nVal4 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueB1( daeElement * pParent,
                             bool         bInputValue ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueB2( daeElement * pParent,
                             bool         b1,
                             bool         b2 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueB3( daeElement * pParent,
                             bool         b1,
                             bool         b2,
                             bool         b3 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a GLSL NewParam.
   //====================================================================================
   bool SetGLSLParamValueB4( daeElement * pParent,
                             bool         b1,
                             bool         b2,
                             bool         b3,
                             bool         b4 ) const;

   //====================================================================================
   //
   // RENDER TARGET specific Methods
   //
   //====================================================================================

   //====================================================================================
   /// \brief Adds a RenderTarget to the state.
   //====================================================================================
   // enum domFx_surface_face_enum {
   // FX_SURFACE_FACE_ENUM_POSITIVE_X,
   // FX_SURFACE_FACE_ENUM_NEGATIVE_X,
   // FX_SURFACE_FACE_ENUM_POSITIVE_Y,
   // FX_SURFACE_FACE_ENUM_NEGATIVE_Y,
   // FX_SURFACE_FACE_ENUM_POSITIVE_Z,
   // FX_SURFACE_FACE_ENUM_NEGATIVE_Z,
   daeElement *                                 AddRenderTarget( daeElement *             pParent,
                                                                 RenderTargetType         eRenderType,
                                                                 int                      nIndex,
                                                                 domFx_surface_face_enum  eSurfaceFaceType,
                                                                 int                      nSlice,
                                                                 int                      nMip ) const;

   //====================================================================================
   /// \brief Adds a ClearTarget to the state.
   //====================================================================================
   daeElement *                                 AddClearTarget( daeElement *     pParent,
                                                                ClearTargetType  eClearType,
                                                                int              nIndex ) const;

   //====================================================================================
   //
   // PIPELINE specific Methods
   //
   //====================================================================================

   //====================================================================================
   /// \brief Sets the AlphaFunc.
   //====================================================================================
   // enum domGl_func_type
   // GL_FUNC_TYPE_NEVER = 0x0200,
   // GL_FUNC_TYPE_LESS = 0x0201,
   // GL_FUNC_TYPE_LEQUAL = 0x0203,
   // GL_FUNC_TYPE_EQUAL = 0x0202,
   // GL_FUNC_TYPE_GREATER = 0x0204,
   // GL_FUNC_TYPE_NOTEQUAL = 0x0205,
   // GL_FUNC_TYPE_GEQUAL = 0x0206,
   // GL_FUNC_TYPE_ALWAYS = 0x0207,
   bool SetPipeline_AlphaFunc( daeElement *     pParent,
                               domGl_func_type  eFuncType,
                               float            fValue ) const;

   //====================================================================================
   /// \brief Sets the BlendFunc.
   //====================================================================================
   // enum domGl_blend_equation_type
   // GL_BLEND_EQUATION_TYPE_FUNC_ADD = 0x8006,
   // GL_BLEND_EQUATION_TYPE_FUNC_SUBTRACT = 0x800A,
   // GL_BLEND_EQUATION_TYPE_FUNC_REVERSE_SUBTRACT = 0x800B,
   // GL_BLEND_EQUATION_TYPE_MIN = 0x8007,
   // GL_BLEND_EQUATION_TYPE_MAX = 0x8008,
   // enum domGl_blend_type
   // GL_BLEND_TYPE_ZERO = 0x0,
   // GL_BLEND_TYPE_ONE = 0x1,
   // GL_BLEND_TYPE_SRC_COLOR = 0x0300,
   // GL_BLEND_TYPE_ONE_MINUS_SRC_COLOR = 0x0301,
   // GL_BLEND_TYPE_DEST_COLOR = 0x0306,
   // GL_BLEND_TYPE_ONE_MINUS_DEST_COLOR = 0x0307,
   // GL_BLEND_TYPE_SRC_ALPHA = 0x0302,
   // GL_BLEND_TYPE_ONE_MINUS_SRC_ALPHA = 0x0303,
   // GL_BLEND_TYPE_DST_ALPHA = 0x0304,
   // GL_BLEND_TYPE_ONE_MINUS_DST_ALPHA = 0x0305,
   // GL_BLEND_TYPE_CONSTANT_COLOR = 0x8001,
   // GL_BLEND_TYPE_ONE_MINUS_CONSTANT_COLOR = 0x8002,
   // GL_BLEND_TYPE_CONSTANT_ALPHA = 0x8003,
   // GL_BLEND_TYPE_ONE_MINUS_CONSTANT_ALPHA = 0x8004,
   // GL_BLEND_TYPE_SRC_ALPHA_SATURATE = 0x0308,
   bool SetPipeline_BlendFunc( daeElement *              pParent,
                               domGl_blend_equation_type eBlendEquationType,
                               domGl_blend_type          eSrcBlendType,
                               domGl_blend_type          eDstBlendType ) const;

   //====================================================================================
   /// \brief Sets the DepthTestEnable.
   /// \param pParent Parent node to attach the new element to.
   /// \param 
   //====================================================================================
   bool SetPipeline_DepthTestEnable( daeElement *  pParent,
                                     bool          bState ) const;

   //====================================================================================
   /// \brief Sets the DepthFunction.
   //====================================================================================
   bool SetPipeline_DepthFunction( daeElement *    pParent,
                                   domGl_func_type eDepthFuncType ) const;

   //====================================================================================
   //
   // Non specific methods
   //
   //====================================================================================

   //====================================================================================
   /// \brief Adds a new Include element to the parent.
   //====================================================================================
   bool AddInclude( daeElement * pParent,
                    const char * pElementSID,
                    const char * pURL ) const;

   //====================================================================================
   /// \brief Adds a new Semantic element to the parent.
   //====================================================================================
   bool AddSemantic( daeElement *   pParent,
                     const char *   pSemantic ) const;

   //====================================================================================
   /// \brief Adds a new Material element to the parent.
   //====================================================================================
   daeElement *                                 AddMaterial( daeElement *  pParent,
                                                             const char *  pElementSID,
                                                             const char *  pName ) const;

   //====================================================================================
   /// \brief Adds a new InstanceEffect element to the parent.
   //====================================================================================
   daeElement *                                 AddInstanceEffect( daeElement *  pParent,
                                                                   const char *  pElementSID,
                                                                   const char *  pURL ) const;

   //====================================================================================
   /// \brief Adds a new TechniqueHint element to the parent.
   //====================================================================================
   daeElement *                                 AddTechniqueHint( daeElement *   pParent,
                                                                  const char *   pProfile,
                                                                  const char *   pPlatform,
                                                                  const char *   pRef ) const;

   //====================================================================================
   /// \brief Adds a new SetParam element to the parent.
   //====================================================================================
   daeElement *                                 AddSetParam( daeElement *  pParent,
                                                             const char *  pRef ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam tag to be the input matrix.
   //====================================================================================
   bool SetSetParamValueF4x4( daeElement *   pParent,
                              float          fVMatrix[] ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam to be two floating point variables.
   //====================================================================================
   bool SetSetParamValueF2( daeElement *  pParent,
                            float         f1,
                            float         f2 ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam to be a floating point triple.
   //====================================================================================
   bool SetSetParamValueF3( daeElement *  pParent,
                            float         f1,
                            float         f2,
                            float         f3 ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam to be four floating points.
   //====================================================================================
   bool SetSetParamValueF4( daeElement *  pParent,
                            float         f1,
                            float         f2,
                            float         f3,
                            float         f4 ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam tag to be the input float.
   //====================================================================================
   bool SetSetParamValueF1( daeElement *  pParent,
                            float         nInputValue ) const;

   //====================================================================================
   /// \brief Sets the value of a SetParam tag to be the input float.
   //====================================================================================
   bool SetSetParamValueI1( daeElement *  pParent,
                            int           nInputValue ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueI2( daeElement *  pParent,
                            int           nVal1,
                            int           nVal2 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueI3( daeElement *  pParent,
                            int           nVal1,
                            int           nVal2,
                            int           nVal3 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueI4( daeElement *  pParent,
                            int           nVal1,
                            int           nVal2,
                            int           nVal3,
                            int           nVal4 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueB4( daeElement *  pParent,
                            bool          nVal1,
                            bool          nVal2,
                            bool          nVal3,
                            bool          nVal4 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueB3( daeElement *  pParent,
                            bool          nVal1,
                            bool          nVal2,
                            bool          nVal3 ) const;

   //====================================================================================
   /// \brief Sets the value(s) of a setparam.
   //====================================================================================
   bool SetSetParamValueB2( daeElement *  pParent,
                            bool          nVal1,
                            bool          nVal2 ) const;

   //====================================================================================
   /// \brief Adds an image reference to the image library.
   //====================================================================================
   daeElement *                                 AddImage( daeElement *  pParent,
                                                          const char *  pTextureRefName,
                                                          const char *  pTextureFileName ) const;

   //====================================================================================
   /// \brief Add a nen source element to the geometry
   //==================================================================================== 
   daeElement *                                 AddSource( daeElement * pParent,
                                                           const char * pID,
                                                           const char * pName ) const;

   //====================================================================================
   /// \brief Creates a specific DOM Element based on input Type and TypeString.
   //====================================================================================
   template< class Type >
      bool GenericCreation( daeElement *  pParent,
                            Type **       element,
                            const char *  pDomTypeString ) const;

   //====================================================================================
   /// \brief Returns a specific library in a Collada doc. Creates a new empty library
   /// if one does not exist.
   //====================================================================================
   daeElement *                                 GetLibrary( DAE *          pDAE,
                                                            domCOLLADA *   pDOM,
                                                            const char *   pLibraryName ) const;

   //====================================================================================
   /// \brief Helper method to create new library.
   //====================================================================================
   daeElement *                                 MakeNewLibrary( DAE *         pDAE,
                                                                domCOLLADA *  pDOM,
                                                                const char *  pLibraryName ) const;

   //====================================================================================
   /// \brief Clear the Materials library
   //==================================================================================== 
   bool ClearMaterialsLibrary( DAE *         pDAE,
                               domCOLLADA *  pDOM ) const;

   //====================================================================================
   /// \brief Clear the Cameras library
   //==================================================================================== 
   daeElement *                                 ClearCamerasLibrary( DAE *          pDAE,
                                                                     domCOLLADA *   pDOM )  const;

   //====================================================================================
   /// \brief Clear the Image library
   //==================================================================================== 
   daeElement *                                 ClearImageLibrary( DAE *         pDAE,
                                                                   domCOLLADA *  pDOM ) const;

   //====================================================================================
   /// \brief Clear the Effects library
   //==================================================================================== 
   daeElement *                                 ClearEffectsLibrary( DAE *          pDAE,
                                                                     domCOLLADA *   pDOM ) const;

   //====================================================================================
   /// \brief Clear the Geometry library
   //==================================================================================== 
   daeElement *                                 ClearGeometryLibrary( DAE *         pDAE,
                                                                      domCOLLADA *  pDOM ) const;

   //====================================================================================
   /// \brief Clear the visual scene library.
   //==================================================================================== 
   daeElement *                                 ClearVisualScenesLibrary( DAE *        pDAE,
                                                                          domCOLLADA * pDOM ) const;

   //====================================================================================
   /// \brief Add a geometry element to the geonmetries library.
   //==================================================================================== 
   daeElement *                                 AddGeometry( daeElement *  pGeomLib,
                                                             const char *  pName,
                                                             const char *  pID ) const;

   //====================================================================================
   /// \brief Add a new mesh elemenet to the geometry element.
   //==================================================================================== 
   daeElement *                                 AddMesh( daeElement * pGeom ) const;

   //====================================================================================
   /// \brief Add a new source for the positions.
   //==================================================================================== 
   daeElement *                                 AddSourcePosition( domListOfFloats &   pFloatList,
                                                                   daeElement *        pMesh,
                                                                   char *              rMeshID,
                                                                   unsigned int        nVertCount,
                                                                   unsigned int        nOffset,
                                                                   unsigned int        nStride ) const;

   //====================================================================================
   /// \brief Add a new source for the normals.
   //==================================================================================== 
   daeElement *                                 AddSourceNormal( domListOfFloats &  pFloatList,
                                                                 daeElement *       pMesh,
                                                                 char *             rMeshID,
                                                                 unsigned int       nVertCount,
                                                                 unsigned int       nOffset,
                                                                 unsigned int       nStride ) const;

   //====================================================================================
   /// \brief Add a new source for the texcoords.
   //==================================================================================== 
   daeElement *                                 AddSourceTexCoord( domListOfFloats &   pFloatList,
                                                                   daeElement *        pMesh,
                                                                   char *              rMeshID,
                                                                   unsigned int        nVertCount,
                                                                   unsigned int        nOffset,
                                                                   unsigned int        nStride ) const;

   //====================================================================================
   /// \brief Add a new source for the tangents.
   //==================================================================================== 
   daeElement *                                 AddSourceTangent( domListOfFloats & pFloatList,
                                                                  daeElement *      pMesh,
                                                                  char *            rMeshID,
                                                                  unsigned int      nVertCount,
                                                                  unsigned int      nOffset,
                                                                  unsigned int      nStride ) const;

   //====================================================================================
   /// \brief Add a new source for the binormals.
   //==================================================================================== 
   daeElement *                                 AddSourceBiNormal( domListOfFloats &   pFloatList,
                                                                   daeElement *        pMesh,
                                                                   char *              rMeshID,
                                                                   unsigned int        nVertCount,
                                                                   unsigned int        nOffset,
                                                                   unsigned int        nStride ) const;

   //====================================================================================
   /// \brief Add a vertices element to a mesh.
   //==================================================================================== 
   daeElement *                                 AddVertices( daeElement *  pMesh,
                                                             const char *  pVerticesID,
                                                             const char *  pSemantic,
                                                             const char *  pSource ) const;

   //====================================================================================
   /// \brief Add a triangles element to a mesh.
   //==================================================================================== 
   daeElement *                                 AddTriangles( daeElement *    pMesh,
                                                              unsigned int    nTriCount,
                                                              unsigned int    nIndexCount,
                                                              unsigned int *  pIndices,
                                                              const char *    pMaterialName ) const;

   //====================================================================================
   /// \brief Add and input element to the vertices element.
   //==================================================================================== 
   domInputLocal *                              AddInput( daeElement *  pVertices,
                                                          const char *  pSemantic,
                                                          const char *  pSource ) const;

   //====================================================================================
   /// \brief Add and input local element to the triangles element.
   //==================================================================================== 
   domInputLocalOffset *                        AddVerticesInput( daeElement *   pVertices,
                                                                  const char *   pSemantic,
                                                                  const char *   pSource ) const;

   //====================================================================================
   /// \brief Add an accessor to source.
   //==================================================================================== 
   daeElement *                                 AddAccessorParam( daeElement *   pParent,
                                                                  const char *   pName,
                                                                  const char *   pType ) const;

   //====================================================================================
   /// \brief Add a new visual scene element.
   //==================================================================================== 
   daeElement *                                 AddVisualScene( daeElement *  pLibrary,
                                                                const char *  pID,
                                                                const char *  pName ) const;

   //====================================================================================
   /// \brief Adds an node element to a visual scene.
   //==================================================================================== 
   daeElement *                                 AddNode( daeElement *   pVisualScene,
                                                         const char *   pID,
                                                         const char *   pName ) const;

   //====================================================================================
   /// \brief Adds an instance geometry element to a visual scene node.
   //==================================================================================== 
   daeElement *                                 AddInstanceGeometry( daeElement *   pParent,
                                                                     const char *   pGeomRefURL ) const;

   //====================================================================================
   /// \brief Adds a bind material element to a visual scene node.
   //==================================================================================== 
   daeElement *                                 AddBindMaterial( daeElement *             pParent,
                                                                 const char *             pMaterialName,
                                                                 domInstance_material **  pInstanceMaterial ) const;

   //====================================================================================
   /// \brief Adds a binding for a generic vertex attribute.
   //==================================================================================== 
   daeElement *                                 AddBindVertexInput( daeElement * pParent,
                                                                    const char * pSemantic,
                                                                    const char * pAttributeName,
                                                                    unsigned int eVertexAttributeIndex ) const;

   //====================================================================================
   /// \brief Add the scene element to the document.
   //====================================================================================
   daeElement *                                 AddScene( DAE *         pDAE,
                                                          domCOLLADA *  pDOM ) const;

   //====================================================================================
   /// \brief Add the single visual scene instance. 
   //====================================================================================
   daeElement *                                 AddVisualSceneInstance( daeElement *   pParent,
                                                                        const char *   pVisSceneURL ) const;

   //====================================================================================
   /// \brief Add a new asset element.
   //====================================================================================
   daeElement *                                 AddAsset( daeElement *  pParent,
                                                          const char *  pTitle,
                                                          const char *  pCreatedTime,
                                                          const char *  pModifiedTime,
                                                          float         fUnitValue,
                                                          const char *  pUnitName,
                                                          domUpAxisType eUpAxis ) const;

   //====================================================================================
   /// \brief Add a new contributor element and set its data.
   //====================================================================================
   daeElement *                                 AddContributor( daeElement *  pParent,
                                                                const char *  pStrAuthor,
                                                                const char *  pStrTool,
                                                                const char *  pStrCopyright,
                                                                const char *  pStrComment,
                                                                const char *  pStrSourceDataDir ) const;

   //====================================================================================
   /// \brief Add a new bool value to an existing set param.
   //====================================================================================
   bool SetSetParamValueBool( daeElement *   pParent,
                              bool           bValue ) const;

   //====================================================================================
   /// \brief Responsible for translating Rm/GL render state settings to COLLADA equivalents.
   //====================================================================================
   daeElement *                                 AddPipelineSettings( daeElement * pPass ) const;

   //====================================================================================
   /// \brief Set the write to Depth Mask on or off. 
   //====================================================================================
   bool SetDepthMask( daeElement *  pStateSettings,
                      bool          bVal ) const;
   //====================================================================================
   /// \brief Set the write to Stencil Mask on or off. 
   //====================================================================================
   bool SetStencilMask( daeElement *   pStateSettings,
                        unsigned int   nVal ) const;
   //====================================================================================
   /// \brief Set the write to Color Mask on or off. 
   //====================================================================================
   bool SetColorMask( daeElement *  pStateSettings,
                      bool *        bArray ) const;
   //====================================================================================
   /// \brief Set culling on or off.
   //====================================================================================
   bool SetCullFaceEnable( daeElement *   pStateSettings,
                           bool           bVal ) const;
   //====================================================================================
   /// \brief Set the type of face culling to use.
   //====================================================================================
   bool SetCullFace( daeElement *      pStateSettings,
                     domGl_face_type   eFaceType )const;
   //====================================================================================
   /// \brief Set blending on or off.
   //====================================================================================
   bool SetBlendEnable( daeElement *   pStateSettings,
                        bool           bVal ) const;

   //====================================================================================
   /// \brief Set a blending componenty (e.g. GL_BlendDestRGB to be GL_SRC_ALPHA etc.) 
   //====================================================================================
   bool SetBlendFuncSeparate( daeElement *   pStateSettings,
                              unsigned int   eBlendFuncType,
                              unsigned int   nValue,
                              daeElement **  ppOutBlendFuncSeparate ) const;

   //====================================================================================
   /// \brief Create and setup default values for the blend func separate element.
   //====================================================================================
   bool IntitializeBlendFuncSeparate( daeElement *    pStateSettings,
                                      daeElement **   ppOutBlendFuncSeparate ) const;

   //====================================================================================
   /// \brief Render state block - Set the blend equation.
   //====================================================================================
   bool SetBlendEquationSeparate( daeElement *              pStateSettings,
                                  domGl_blend_equation_type nValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the depth test enable.
   //====================================================================================
   bool SetDepthTestEnable( daeElement *  pStateSettings,
                            bool          bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the stencil test enable.
   //====================================================================================
   bool SetStencilTestEnable( daeElement *   pStateSettings,
                              bool           bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the alpha test enable.
   //====================================================================================
   bool SetAlphaTestEnable( daeElement *  pPass,
                            bool          bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the line smooth enable.
   //====================================================================================
   bool SetLineSmoothEnable( daeElement * pStateSettings,
                             bool         bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the point smooth enable.
   //====================================================================================
   bool SetPointSmoothEnable( daeElement *   pStateSettings,
                              bool           bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set two sided lighting.
   //====================================================================================
   bool SetTwoSidedLighting( daeElement * pStateSettings,
                             bool         bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the polygon smooth enable.
   //====================================================================================
   bool SetPolygonSmoothEnable( daeElement * pPass,
                                bool         bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the depth function.
   //====================================================================================
   bool SetDepthFunction( daeElement *    pStateSettings,
                          domGl_func_type nValue ) const;

   //====================================================================================
   /// \brief Render state block - Set fog mode.
   //====================================================================================
   bool SetFogMode( daeElement *    pStateSettings,
                    domGl_fog_type  nFogType ) const;

   //====================================================================================
   /// \brief Render state block - Set fog color.
   //====================================================================================
   bool SetFogColor( daeElement *   pStateSettings,
                     float *        fColorArray ) const;

   //====================================================================================
   /// \brief Render state block - Set fog mode enable.
   //====================================================================================
   bool SetFogModeEnable( daeElement * pStateSettings,
                          bool         bVal ) const;

   //====================================================================================
   /// \brief Render state block - Set the line width
   //====================================================================================
   bool SetLineWidth( daeElement *  pStateSettings,
                      float         fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the clear depth
   //====================================================================================
   bool SetClearDepth( daeElement * pStateSettings,
                       float        fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the point size
   //====================================================================================
   bool SetPointSize( daeElement *  pStateSettings,
                      float         fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the point min
   //====================================================================================
   bool SetPointMin( daeElement *   pStateSettings,
                     float          fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the point max
   //====================================================================================
   bool SetPointMax( daeElement *   pStateSettings,
                     float          fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the fog density
   //====================================================================================
   bool SetFogDensity( daeElement * pStateSettings,
                       float        fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the fog start
   //====================================================================================
   bool SetFogStart( daeElement *   pStateSettings,
                     float          fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the fog end
   //====================================================================================
   bool SetFogEnd( daeElement *  pStateSettings,
                   float         fValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the clear stencil
   //====================================================================================
   bool SetClearStencil( daeElement *  pStateSettings,
                         unsigned long nValue ) const;

   //====================================================================================
   /// \brief Render state block - Set the clear color
   //====================================================================================
   bool SetClearColor( daeElement * pStateSettings,
                       float *      fColorArray ) const;

   //====================================================================================
   /// \brief Render state block - Set the clear color
   //====================================================================================
   bool SetBlendColor( daeElement * pStateSettings,
                       float *      fColorArray ) const;

   // NOTE: there is no current color in the GLSL profile. This is an ommission and is bugged. 
   //====================================================================================
   /// \brief Render state block - Set the current color
   //====================================================================================
   //bool SetCurrentColor ( daeElement   *  pStateSettings,
   //                     float        *  fColorArray ) const ;

   //====================================================================================
   /// \brief Render state block - Set the poly back mode.
   //====================================================================================
   bool SetPolyMode( daeElement *            pStateSettings,
                     domGl_polygon_mode_type eVal,
                     domGl_face_type         eFaceType ) const;

   //====================================================================================
   /// \brief Render state block - Set the polygon winding order.
   //====================================================================================
   bool SetFrontFace( daeElement *           pStateSettings,
                      domGl_front_face_type  nValue ) const;

   //====================================================================================
   /// \brief Loop through the texture states and map them to COLLADA state elements.
   //====================================================================================
   template < class Type >
      bool SetTextureState1D( Type *                     pParent,
                              DOMHelperTextureStateVec & vTextureStateVec ) const;

   //====================================================================================
   /// \brief Loop through the texture states and map them to COLLADA state elements.
   //====================================================================================
   template < class Type >
      bool SetTextureState2D( Type *                     pParent,
                              DOMHelperTextureStateVec & vTextureStateVec ) const;

   //====================================================================================
   /// \brief Loop through the texture states and map them to COLLADA state elements.
   //====================================================================================
   template < class Type >
      bool SetTextureState3D( Type *                     pParent,
                              DOMHelperTextureStateVec & vTextureStateVec ) const;

   //====================================================================================
   /// \brief Add the camera to the library.
   //====================================================================================
   bool AddCamera( daeElement *        pCameraLib,
                   DOMHelperCamera &   rCamera ) const;

   //====================================================================================
   /// \brief adds a Camera node to the current visual_scene and defines it lookat data.
   //====================================================================================
   bool AddCameraNode( daeElement *       pVisualScene,
                       DOMHelperCamera &  pRmCam,
                       const char *       strFullCamNodeName,
                       const char *       strConvCameraReference ) const;

   //====================================================================================
   /// \brief Adds and evaluate_scene element to the visual_scene (fullscreen effects).
   //====================================================================================
   bool AddEvaluateScene( daeElement * pVisualScene,
                          daeElement * pInstanceEffect,
                          const char * strCamNodeName );

   //====================================================================================
   /// \brief Used to set float values in elements.
   //====================================================================================
   bool SetTargetableFloat( daeElement *  pParent,
                            const char *  pStrName,
                            float         fValue ) const;

   //====================================================================================
   /// \brief Set the image format for an RTT surface.
   //====================================================================================
   template< class Type >
      bool SetSurfaceRTTFormat( Type *       pFormat,
                                unsigned int eFormat ) const;

   //====================================================================================
   /// \brief Adds a color_target element to the pass. Used do define which RTT surface to use.
   //====================================================================================
   bool AddRTTColorTargetToPass( daeElement *   pPass,
                                 unsigned int   eFormat,
                                 const char *   pTargetSurfaceName,
                                 unsigned int   nRTTIndex ) const;

   //====================================================================================
   /// \brief Adds the color to clear the RTT texture to.
   //====================================================================================
   bool AddRTTColorClearToPass( daeElement * pPass,
                                float        fR,
                                float        fG,
                                float        fB,
                                float        fA,
                                unsigned int nRTTIndex ) const;

   //====================================================================================
   /// \brief Adds the depth to clear the RTT texture to.
   //====================================================================================
   bool AddRTTDepthClearToPass( daeElement * pPass,
                                float        fDepth,
                                unsigned int nRTTIndex ) const;

   //====================================================================================
   /// \brief Add a Perspective camera to the library.
   //====================================================================================
   bool AddPerspective( daeElement *      pDomTechnique,
                        DOMHelperCamera & Camera ) const;

   //====================================================================================
   /// \brief Add an Orthographic camera to the library.
   //====================================================================================
   bool AddOrthographic( daeElement *        pDomTechnique,
                         DOMHelperCamera &   Camera ) const;

   //====================================================================================
   /// \brief Add a semantic tag to the input param.
   //====================================================================================
   bool AddGLSLSemantic( daeElement *  pNewParam,
                         const char *  pStrSemantic );

   //====================================================================================
   /// \brief Set the factor component of the poly offset element.
   //====================================================================================
   bool SetPolyOffsetFactor( daeElement *    pStateSettings,
                             float           fValue,
                             daeElement *&   pOutPolyOffset ) const;

   //====================================================================================
   /// \brief Set the units component of the poly offset element.
   //====================================================================================
   bool SetPolyOffsetUnits( daeElement *  pStateSettings,
                            float         fValue,
                            daeElement *& pOutPolyOffset ) const;

   //====================================================================================
   /// \brief Set the Near and Far depth bounds.
   //====================================================================================
   bool SetDepthBounds( daeElement *   pStateSettings,
                        unsigned int   nIndex,
                        float          fValue,
                        daeElement *&  pOutDepthBounds )const;

   //====================================================================================
   /// \brief Set the Alpha function
   //====================================================================================
   bool SetAlphaFunction( daeElement *    pStateSettings,
                          domGl_func_type nAlphaFunctionType,
                          daeElement *&   pOutAlphaFunctionElement ) const;

   //====================================================================================
   /// \brief Set the Stencil function
   //====================================================================================
   bool SetStencilFunction( daeElement *     pStateSettings,
                            domGl_func_type  nStencilFunctionType,
                            daeElement *&    pOutStencilFunctionElement ) const;

   //====================================================================================
   /// \brief Set the Alpha reference value
   //====================================================================================
   bool SetAlphaReference( daeElement *   pStateSettings,
                           float          fAlphaValue,
                           daeElement *&  pOutAlphaFunctionElement ) const;

   //====================================================================================
   /// \brief Set the Stencil reference value
   //====================================================================================
   bool SetStencilReference( daeElement *    pStateSettings,
                             unsigned int    nStencilValue,
                             daeElement *&   pOutStencilFunctionElement ) const;

   //====================================================================================
   /// \brief Set the Stencil value mask value
   //====================================================================================
   bool SetStencilValueMask( daeElement *    pStateSettings,
                             unsigned int    nStencilValue,
                             daeElement *&   pOutStencilFunctionElement ) const;

   //====================================================================================
   /// \brief Set the Stencil fail value
   //====================================================================================
   bool SetStencilFail( daeElement *            pStateSettings,
                        domGl_stencil_op_type   nFailValue,
                        daeElement *&           pOutStencilOpElement ) const;

   //====================================================================================
   /// \brief Set the Stencil depth fail value
   //====================================================================================
   bool SetStencilPassDepthFail( daeElement *            pStateSettings,
                                 domGl_stencil_op_type   nFailValue,
                                 daeElement *&           pOutStencilOpElement ) const;

   //====================================================================================
   /// \brief Set the Stencil depth pass value
   //====================================================================================
   bool SetStencilPassDepthPass( daeElement *            pStateSettings,
                                 domGl_stencil_op_type   nFailValue,
                                 daeElement *&           pOutStencilOpElement ) const;

   //====================================================================================
   /// \brief Enable or Disable poly offset fill.
   //====================================================================================
   bool SetPolyOffsetFill( daeElement *   pStateSettings,
                           bool           bVal )const;

   //====================================================================================
   /// \brief Enable or Disable poly offset fill.
   //====================================================================================
   bool SetShadeModel( daeElement *             pStateSettings,
                       domGl_shade_model_type   nModelType )const;

   //====================================================================================
   /// \brief Enable or Disable poly offset for points.
   //====================================================================================
   bool SetPolyOffsetPoint( daeElement *  pStateSettings,
                            bool          bVal )const;

   //====================================================================================
   /// \brief Enable or Disable poly offset for points.
   //====================================================================================
   bool SetPolyOffsetLine( daeElement *   pStateSettings,
                           bool           bVal )const;

   //====================================================================================
   /// \brief Main handler for exporting a sampler
   //====================================================================================
   daeElement *                                 AddSampler( ProfileType                eProfileType,
                                                            SurfaceType                eSurfaceType,
                                                            daeElement *               pParent,
                                                            const char *               pSamplerName,
                                                            const char *               pSurfaceName,
                                                            DOMHelperTextureStateVec & vTextureStateVec ) const;

   //====================================================================================
   /// \brief Exports GLSL sampler
   //====================================================================================
   daeElement *                                 SetSamplerGLSL( SurfaceType                  eSurfaceType,
                                                                daeElement *                 pParent,
                                                                const char *                 pSamplerName,
                                                                const char *                 pSurfaceSource,
                                                                DOMHelperTextureStateVec &   vTextureStateVec ) const;

   //====================================================================================
   /// \brief Exports common sampler
   //====================================================================================
   daeElement *                                 SetSamplerCommon( SurfaceType                eSurfaceType,
                                                                  daeElement *               pParent,
                                                                  const char *               pSamplerName,
                                                                  const char *               pSurfaceSource,
                                                                  DOMHelperTextureStateVec & vTextureStateVec ) const;

   //====================================================================================
   /// \brief Exports a sampler of any profile.
   //====================================================================================
   template < class Type >
      daeElement *                              SetSamplerSpecific( Type **      pType,
                                                                    daeElement * pParent,
                                                                    const char * pSurfaceSource,
                                                                    const char * pSamplerTypeName ) const;

   //====================================================================================
   /// \brief Main handler for exporting a surface os 1,2,3D.
   //====================================================================================
   daeElement *                                 AddSurface123D( ProfileType   eProfileType,
                                                                daeElement *  pParent,
                                                                const char *  pSurfaceName,
                                                                const char *  pTextureRefName,
                                                                const char *  pFormat,
                                                                SurfaceType   eSurfaceType,
                                                                std::string * strCubemapReferenceNames ) const;

   //====================================================================================
   /// \brief Exports a surface of any profile.
   //====================================================================================
   template < class Type >
      bool SetSurfaceSpecific( daeElement *              pParent,
                               Type **                   ppOutSurface,
                               const char *              pFileReference,
                               const char *              pImageFormat,
                               domFx_surface_type_enum   eSurfaceType,
                               std::string *             strCubemapReferenceNames ) const;

   //====================================================================================
   /// \brief Exports an RTT surface of any profile - calls SetRTTSurfaceSpecific.
   //====================================================================================
   daeElement *                                 AddRTTSurface( ProfileType    eProfileType,
                                                               daeElement *   pParent,
                                                               const char *   pSurfaceName,
                                                               int            nWidth,
                                                               int            nHeight,
                                                               unsigned int   nFormat );

   //====================================================================================
   /// \brief Exports an RTT surface of any profile.
   //====================================================================================
   template < class Type >
      bool SetRTTSurfaceSpecific( daeElement *  pParent,
                                  Type **       ppOutSurface,
                                  int           nWidth,
                                  int           nHeight,
                                  unsigned int  nFormat ) const;

   //====================================================================================
   /// Adds the color data to the COMMON Phong profile.
   //====================================================================================
   bool AddPhongColor( daeElement * pParent,
                       const char * pElementTypeName,
                       float        fR,
                       float        fG,
                       float        fB,
                       float        fA ) const;

   //====================================================================================
   /// Adds the transparency data to the COMMON Phong profile.
   //====================================================================================
   bool AddPhongTransparency( daeElement *   pParent,
                              const char *   pElementTypeName,
                              float          fR,
                              float          fG,
                              float          fB,
                              float          fA ) const;

   //====================================================================================
   /// Adds the float data to the COMMON Phong profile.
   //====================================================================================
   bool AddPhongComponent( daeElement *   pParent,
                           const char *   pElementTypeName,
                           float          fVal ) const;

   //====================================================================================
   /// Write out a new set of indices for a specific mesh (by index).
   //====================================================================================
   bool OverWriteIndices( unsigned int    nMeshIndex,
                          unsigned int *  nDataArray,
                          unsigned int    nIndicesCount,
                          unsigned int    nTriangleCount,
                          const char *    pSemantic );

   //====================================================================================
   /// Return the cleaned current document name.
   //====================================================================================
   char8 *                                      GetDocumentName()
   {
      return m_strCleanedFilename;
   }

   //====================================================================================
   /// Add a draw element to the pass.
   //====================================================================================
   bool AddDrawToPass( daeElement * pPass,
                       const char * pDrawTypeName ) const;

   //====================================================================================
   /// \brief Create an extra element.
   //====================================================================================
   domParam *  CreateExtra(   daeElement *  pParent,
                              const char *  pProfileName,
                              const char *  pElementName,
                              const char *  pSid ) const;


   //====================================================================================
   /// Clear all temporary string copies from the cache
   //====================================================================================
   void ClearStringCache();

   //====================================================================================
   /// Add a string to the cache.
   //====================================================================================
   char * CacheString ( const char * pStr );

private:

   //====================================================================================
   /// \brief Convert windows file path to URI.
   //====================================================================================
   bool CleanInputString( char8 *         pOut,
                          const char8 *   pIn );

   //====================================================================================
   /// \brief Get pointer to the file name at the end of the full path.
   //====================================================================================
   const char8 *                                GetFileName( const char8 * pPath ) const;

   //====================================================================================
   /// \brief Load the meshes within a scene.
   //====================================================================================
   bool LoadMeshFromDoc( RmScene * pCrtScene );

   //====================================================================================
   /// \brief Load the meshes from a DAE doc.
   //====================================================================================
   bool LoadMeshFromDoc( DAE *         m_pDAE,
                         domCOLLADA *  m_pDOM );

   //====================================================================================
   /// \brief Helper function to load a library of meshes.
   //====================================================================================
   bool LoadGeometry( CrtGeometry * pGeometry );

   //====================================================================================
   /// \brief Helper function to load specific mesh.
   //====================================================================================
   bool LoadTriangleMesh( CrtTriangles * pTriMesh );

   //====================================================================================
   /// \brief Convert float to internal string
   //====================================================================================
   const inline char *                          ToString( float fValue )
   {
      sprintf( m_strConversionString, "%f", fValue );	

      // Terminate the string
      m_strConversionString[nCoversionStringMax - 1] = '\0';

      return &m_strConversionString[0];
   }

   //====================================================================================
   /// \brief Convert int to internal string
   //====================================================================================
   const inline char *                          ToString( int nValue )
   {
      sprintf( m_strConversionString, "%d", nValue );	

      // Terminate the string
      m_strConversionString[nCoversionStringMax - 1] = '\0';

      return &m_strConversionString[0];
   }

   //====================================================================================
   /// Add the float value as a string to the param.
   //====================================================================================
   void SetFloatParam( domParam *   pParam,
                       float        fTimeCycle ) const;

   bool LoadGeometryFromNode( domNode * node );

   bool LoadInstanceGeometry( domInstance_geometry * pGeomInst );

private: // Data

   // List of mesh accessor (1 per sub-mesh)
   std::vector< DOMColladaMeshAccessor >        m_MeshAccessors;

   Geometries                                   m_Geometries;

   // Full path name of the COllada file that was loaded
   char8 m_strCleanedFilename[nMaxPathSize];	

   char8 m_strConversionString[nCoversionStringMax];

   DAE *                                        m_pDAE;
   domCOLLADA *                                 m_pDOM;
   RmScene *                                    m_pScene;

   StringCache m_StringCache;
};


#endif // _DOM_COLLADA_LIBRARY_H_
