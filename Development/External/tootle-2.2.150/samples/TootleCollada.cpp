//=================================================================================================================================
//
// Author: Peter Lohrmann
//         GPG Developer Tools Group
//         AMD, Inc.
//
// This application is based on the sample application that is provided with Tootle, but is intended to be slightly more robust
// and allows for Tootling Collada documents (*.dae). The input and output format is a collada document, where the output document
// will have re-ordered faces which optimizes vertex cache performance as well as overdraw.
//
// This application has similar support for viewpoint files as the sample application.
// 
// A user may want to use custom viewpoints to optimize a mesh if there are restrictions on the angles from which the object can be
// viewed.  For example, if an object is always viewed from above, and never from below, then it is advantageous to place all of 
// the sample viewpoints on the upper hemisphere.
//
// The sample also allows the user to specify a target cluster count.  In general, users will want to let Tootle cluster automatically,
// but manually setting a high cluster count can sometimes provide greater overdraw reductions, at the expense of poor vertex cache
// performance.  Note that the cluster count that is passed to TootleClusterMesh is only a hint.  If the mesh is highly disconnected,
// then the algorithm may need to generate a larger number of clusters
//
//=================================================================================================================================
//   (C) AMD, Inc. 2007 All rights reserved. 
//=================================================================================================================================

// fix VC++ 2005 warnings about fopen, fscanf, etc being deprecated
// this is just a sample app after all :)
#if defined( _MSC_VER )
   #if _MSC_VER >= 1400
      #define _CRT_SECURE_NO_DEPRECATE
   #endif
#endif

#include "option.h"
#include "AMDColladaDoc.h"
#include "tootlelib.h"
#include <vector>
#include <iostream>
#include <fstream>


//=================================================================================================================================
/// A simple structure to store the settings for this app
//=================================================================================================================================
struct TootleSettings
{
   const char *pMeshName;
   const char *pOptimizedMeshName;
   const char *pViewpointName;
   unsigned int nClustering;
   unsigned int nCacheSize;
   TootleFaceWinding eWinding;
};


//=================================================================================================================================
/// A simple function to allow Collada to print messages when in a DEBUG build
//=================================================================================================================================
void printMsg( const char* pMsg )
{
#ifdef _DEBUG
   std::cerr << pMsg << std::endl;
#endif
}


//=================================================================================================================================
/// A simple structure to hold Tootle statistics
//=================================================================================================================================
struct TootleStats
{
   unsigned int nClusters;
   float        fVCacheIn;
   float        fVCacheOut;
   float        fOverdrawIn;
   float        fOverdrawOut;
   float        fMaxOverdrawIn;
   float        fMaxOverdrawOut;
};


//=================================================================================================================================
/// Reads a list of camera positions from a viewpoint file.
///
/// \param pFileName   The name of the file to read
/// \param rVertices   A vector which will be filled with vertex positions
/// \param rIndices    A vector which will will be filled with face indices
//=================================================================================================================================
bool LoadViewpoints( const char* pFileName, std::vector<ObjVertex3D>& rViewPoints ) 
{

   if ( pFileName == NULL )
   {
      return false;
   }

   FILE* pFile = fopen(pFileName, "r");
   if( !pFile )
   {
      return false;
   }

   int iSize;
   if( fscanf(pFile, "%i\n", &iSize) != 1 )
   {
      return false;
   }
    
   for (int i = 0; i < iSize; i++)
   {
      float x, y, z;
      if( fscanf(pFile, "%f %f %f\n", &x, &y, &z) != 3 )
      {
         return false;
      }
      ObjVertex3D vert;
      vert.fX = x;
      vert.fY = y;
      vert.fZ = z;
      rViewPoints.push_back( vert );
   }

   fclose( pFile );

   return true;
}


//=================================================================================================================================
/// Displays usage instructions, and calls exit()
/// \param nRet  Return code to pass to exit
//=================================================================================================================================
void ShowHelpAndExit( int nRet )  
{
   fprintf(stderr,
         "Syntax:\n"
         "   TootleCollada.exe [-v viewpointfile] [-c clusters] [-s cachesize] [-f] in.dae out.dae\n"
         "        If -f is specified, counter-clockwise faces are front facing.\n");
   exit( nRet );
}


//=================================================================================================================================
/// Displays a tootle error message
/// \param result The tootle result that is being displayed
//=================================================================================================================================
void DisplayTootleErrorMessage( TootleResult eResult )
{
   std::cerr << "Tootle returned error: " << std::endl;
   switch( eResult )
   {
   case TOOTLE_INVALID_ARGS:     std::cerr << " TOOTLE_INVALID_ARGS"    << std::endl; break; 
   case TOOTLE_OUT_OF_MEMORY:    std::cerr << " TOOTLE_OUT_OF_MEMORY"   << std::endl; break;      
   case TOOTLE_3D_API_ERROR:     std::cerr << " TOOTLE_3D_API_ERROR"    << std::endl; break;      
   case TOOTLE_INTERNAL_ERROR:   std::cerr << " TOOTLE_INTERNAL_ERROR"  << std::endl; break;  
   case TOOTLE_NOT_INITIALIZED:  std::cerr << " TOOTLE_NOT_INITIALIZED" << std::endl; break;   
   }
}



//=================================================================================================================================
/// Displays usage instructions, and calls exit()
/// \param nRet  Return code to pass to exit
//=================================================================================================================================
void ParseCommandLine(int argc, char *argv[], TootleSettings* pSettings ) 
{
   Option::Definition options[] = {
       { 'h', "Help" },
       { 'v', "Viewpoint file" },
       { 'c', "Number of clusters" },
       { 's', "Post TnL vcache size" },
       { 'f', "Treat counter-clockwise faces as front facing (instead clockwise faces)." },
       { 0, NULL },
   };

   Option opt;
   char cOption = opt.Parse(argc, argv, options);

   while( cOption != -1 )
   {
      switch ( cOption ) 
      {
      case 'h':
         ShowHelpAndExit(0);
         break;

      case 'v':
         if (!pSettings->pViewpointName) 
         {
            pSettings->pViewpointName = opt.GetArgument(argc, argv);
         }
         else
         {
            ShowHelpAndExit(1);
         }
         break;
      
      case 'f':
         pSettings->eWinding = TOOTLE_CCW;
         break;

      case 'c':
         pSettings->nClustering = atoi(opt.GetArgument(argc, argv));
         break;

      case 's':
         pSettings->nCacheSize = atoi(opt.GetArgument(argc, argv));
         break;

         // option with no switch.  This is the mesh filename
      case '?':
         if (!pSettings->pMeshName) 
         {
            pSettings->pMeshName = opt.GetArgument(argc, argv);
         }
         else 
         {
            if ( !pSettings->pOptimizedMeshName )
            {
               pSettings->pOptimizedMeshName = opt.GetArgument(argc, argv);
            }
            else
            {
               ShowHelpAndExit(1);
            }
         }
         break;

      default:
         ShowHelpAndExit(1);
         break;
      }

      cOption = opt.Parse(argc, argv, options);
   }

   // make sure we got a mesh name and an optimized mesh name
   if ( !pSettings->pMeshName && !pSettings->pOptimizedMeshName ) 
   {
     ShowHelpAndExit(1); 
   }
}


//=================================================================================================================================
/// A helper function to print formatted TOOTLE statistics
/// \param f      A file to print the statistics to
/// \param pStats The statistics to be printed
//=================================================================================================================================

void PrintStats(FILE *f, TootleStats* pStats)
{
   fprintf(f, "Tootle Stats\n" ); 
   fprintf(f, "------------\n" ); 
   fprintf(f, "Clusters: %u\nCacheIn/Out: %.3fx (%.3f/%.3f)\nOverdrawIn/Out: %.3fx (%.3f/%.3f)\nOverdrawMaxIn/Out: %.3fx (%.3f/%.3f)\n", 
              pStats->nClusters,
              pStats->fVCacheIn/pStats->fVCacheOut, 
              pStats->fVCacheIn, 
              pStats->fVCacheOut,
              pStats->fOverdrawIn/pStats->fOverdrawOut, 
              pStats->fOverdrawIn, 
              pStats->fOverdrawOut,
              pStats->fMaxOverdrawIn/pStats->fMaxOverdrawOut, 
              pStats->fMaxOverdrawIn, 
              pStats->fMaxOverdrawOut);
}

//=================================================================================================================================
/// The main application
/// After parsing commandline options, it loads the viewpoint file, then loads the collada document,
/// and finally performs tootle on each individual mesh contained within the file.
/// The Collada document is saved given the specified name.
//=================================================================================================================================
int main( int argc, char* argv[] )
{
   // initialize settings to defaults
   TootleSettings settings;
   settings.pMeshName = NULL;
   settings.pOptimizedMeshName = NULL;
   settings.pViewpointName = NULL;
   settings.nClustering = 0;
   settings.nCacheSize = TOOTLE_DEFAULT_VCACHE_SIZE;
   settings.eWinding = TOOTLE_CW;

   // parse the command line
   ParseCommandLine( argc, argv, &settings );
   
   fprintf( stdout, "Tootle Settings\n");
   fprintf( stdout, "---------------\n");
   fprintf( stdout, "Input Mesh: \t%s\n", settings.pMeshName );
   fprintf( stdout, "Output Mesh: \t%s\n", settings.pOptimizedMeshName );
   if ( settings.pViewpointName != NULL )
   {
      fprintf( stdout, "Viewpoints File: \t%s\n", settings.pViewpointName );
   }

   if ( settings.nClustering == 0 )
   {
      fprintf( stdout, "Automatic Clustering\n" ); 
   }
   else
   {
      fprintf( stdout, "Suggested Clusters: %u\n", settings.nClustering ); 
   }

   fprintf( stdout, "Target Cache Size: %u\n", settings.nCacheSize );

   if ( settings.eWinding == TOOTLE_CW )
   {
      fprintf( stdout, "Clock-Wise Face Winding\n" );
   }
   else
   {
      fprintf( stdout, "Counter Clock-Wise Face Winding\n" );
   }

   fprintf( stdout, "\n" );

   // ******************************************
   //    Load viewpoints if necessary
   // ******************************************

   // read viewpoints if needed
   std::vector<ObjVertex3D> viewpoints;
   if( settings.pViewpointName != NULL )
   {
      if ( !LoadViewpoints(settings.pViewpointName, viewpoints) )
      {
         std::cerr << "Unable to load viewpoints from file: " << settings.pViewpointName;
         return 1; 
      }
   }

   // if we didn't get any viewpoints, then use a NULL array
   const float* pViewpoints = NULL;
   unsigned int nViewpoints = (unsigned int) viewpoints.size();
   if( viewpoints.size() > 0 )
   {
      pViewpoints = (const float*) &viewpoints[0];
   }

   // ***************************************************
   //   Load the mesh
   // ***************************************************

   // read the mesh from the file
   std::vector<ObjVertexFinal> objVertices;
   std::vector<ObjFace> objFaces;

   AMDColladaDoc colladaDoc( AMDColladaDoc::DOM_COLLADA_LIB, printMsg, printMsg );

   if ( !colladaDoc.LoadColladaFile( settings.pMeshName, false ) )
   {
      std::cerr << "Error loading file: " << settings.pMeshName << std::endl;
      return 1;
   }

   if ( colladaDoc.ExtractMeshes() == false )
   {
      std::cerr << "Could not extract meshes from the Collada document contained in: " << settings.pMeshName << std::endl;
      return 1;
   }

   unsigned int nMeshCount = colladaDoc.GetMeshAccessorCount();

   if ( nMeshCount == 0 )
   {
      std::cerr << "Collada document does not contain any mesh data: " << settings.pMeshName << std::endl;
      return 1;
   }

   // build buffers containing only the vertex positions and indices, since this is what Tootle requires
   std::vector<ObjVertex3D> vertices;
   std::vector<ObjFace> faces;
   std::vector<unsigned int> indices;

   TootleStats stats;
   TootleResult result;
   bool bError = false;

   // initialize Tootle
   result = TootleInit();
   if( result != TOOTLE_OK )
   {
      DisplayTootleErrorMessage( result );
      return 1;
   }

   // ***************************************************
   //   Check to see if we have no position data
   // ***************************************************
   bool bHasPositions = false;
   for ( unsigned int i = 0; i < nMeshCount; i++ )
   {
      const AMDColladaMeshAccessor* pMeshAccessor = colladaDoc.GetMeshAccessor(i);
      if ( !pMeshAccessor )
      {
         bError = true;
         continue;
      }

      // make sure position data exists
      if ( pMeshAccessor->HasPositions() && 
           pMeshAccessor->GetFaceCount() > 0 )
      {
         bHasPositions = true;
      }
   }

   // If we have no positions then complain and return.
   if ( !bHasPositions )
   {
      std::cerr << "Error: The COLLADA importer currently only supports triangles."  << std::endl;
      return false;
   }

   // ***************************************************
   //   Tootle each mesh contained in the Collada doc
   // ***************************************************
   for ( unsigned int i = 0; i < nMeshCount; i++ )
   {
      // ***************************************************
      //   Load and prepare a mesh in the the doc
      // ***************************************************

      const AMDColladaMeshAccessor* pMeshAccessor = colladaDoc.GetMeshAccessor(i);

      if ( !pMeshAccessor )
      {
         bError = true;
         continue;
      }

      // make sure position data exists
      if ( !pMeshAccessor->HasPositions() || 
           pMeshAccessor->GetFaceCount() <= 0 )
      {
         bError = true;
         std::cerr << "Error: The COLLADA importer currently only supports triangles."  << std::endl;
         continue;
      }

      // get positions
      if ( pMeshAccessor->HasPositions() )
      {
         if ( !pMeshAccessor->LoadVertices( vertices ) )
         {
            bError = true;
            continue;
         }
      }

      // assume collada doc has unified indices,
      // but confirm they are the same along the way
      // and error if something isn't unified.
      bool bHasUnifiedIndices = true;
      bool bConfirmUnifiedNormalIndices = false;
      bool bConfirmUnifiedTexCoordIndices = false;
      if ( pMeshAccessor->HasNormals() )
      {
         bConfirmUnifiedNormalIndices = true;
         if ( pMeshAccessor->HasTexCoords() )
         {
            bConfirmUnifiedTexCoordIndices = true;
         }
      }

      pMeshAccessor->LoadFaces( faces );

      indices.resize( faces.size()*3 );
      for( unsigned int index = 0; index < indices.size(); index++ )
      {
         int iFace = index / 3;
         int iAttrib = index % 3;
         indices[index] = faces[ iFace ].nVertexIndices[ iAttrib ];
         if ( bConfirmUnifiedNormalIndices == true )
         {
            if ( indices[ index ] != faces[ iFace ].nNormalIndices[ iAttrib ] )
            {
               bHasUnifiedIndices = false;
               break;
            }
         }
         if ( bConfirmUnifiedTexCoordIndices == true )
         {
            if ( indices[ index ] != faces[ iFace ].nTexCoordIndices[ iAttrib ] )
            {
               bHasUnifiedIndices = false;
               break;
            }
         }
      }

      if ( bHasUnifiedIndices == false )
      {
         std::cerr << "Collada document must have unified indices: " << settings.pMeshName << std::endl;
         bError = true;
         continue;;
      }

      // *****************************************************************
      //   Optimize the mesh
      // *****************************************************************

      unsigned int nTriangles = (unsigned int) indices.size() / 3;
      unsigned int nVertices = (unsigned int) vertices.size();
      const float* pVB = (float*) (&vertices[0]);
      unsigned int* pIB = (unsigned int*) (&indices[0]);
      unsigned int nStride = 3*sizeof(float);

      // measure input VCache efficiency
      result = TootleMeasureCacheEfficiency( pIB, nTriangles, settings.nCacheSize, &stats.fVCacheIn );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      // measure input overdraw.  Note that we assume counter-clockwise vertex winding. 
      result = TootleMeasureOverdraw( pVB, pIB, nVertices, nTriangles, nStride, pViewpoints, nViewpoints, settings.eWinding, 
                                      &stats.fOverdrawIn, &stats.fMaxOverdrawIn );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      // allocate an array to hold the cluster ID for each face
      std::vector<unsigned int> faceClusters;
      faceClusters.resize( nTriangles + 1 );

      // cluster the mesh, and sort faces by cluster
      result = TootleClusterMesh( pVB, pIB, nVertices, nTriangles, nStride, settings.nClustering, pIB, &faceClusters[0], NULL );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      stats.nClusters = 1 + faceClusters[ nTriangles - 1 ];

      // perform vertex cache optimization on the clustered mesh
      result = TootleVCacheClusters( pIB, nTriangles, nVertices, settings.nCacheSize, &faceClusters[0], pIB, NULL );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      // optimize the draw order
      result = TootleOptimizeOverdraw( pVB, pIB, nVertices, nTriangles, nStride, pViewpoints, nViewpoints, settings.eWinding,
                                       &faceClusters[0], pIB, NULL );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }
      
      // measure output VCache efficiency
      result = TootleMeasureCacheEfficiency( pIB, nTriangles, settings.nCacheSize, &stats.fVCacheOut );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      // measure output overdraw
      result = TootleMeasureOverdraw( pVB, pIB, nVertices, nTriangles, nStride, pViewpoints, nViewpoints, settings.eWinding, 
                                      &stats.fOverdrawOut, &stats.fMaxOverdrawOut );
      if( result != TOOTLE_OK )
      {
         DisplayTootleErrorMessage( result );
         bError = true;
         continue;
      }

      // set the output index array
      if ( colladaDoc.OverWriteIndices( i, pIB, nTriangles*3, nTriangles, NULL ) == false )
      {
         std::cerr << "Could not write out indices." << std::endl;
         continue;
      }

      // print tootle statistics to stdout and stderr
      PrintStats( stdout, &stats ); 

      // clear the arrays for the next mesh
      vertices.clear();
      faces.clear();
      indices.clear();
   }

   // clean up tootle
   TootleCleanup();

   if ( bError == true )
   {
      std::cerr << "An error occured during processing, the file will still be saved, but the result may not be useable or may not be optimized." << std::endl;
   }

   // ***************************************************
   //   Save the Tootled mesh
   // ***************************************************

   if ( colladaDoc.SaveAs( const_cast<char*> (settings.pOptimizedMeshName), true ) == false )
   {
      std::cerr << "FAILED to save optimized collada file: " << settings.pOptimizedMeshName << std::endl;
      return 1;
   }

   return 0;
}

