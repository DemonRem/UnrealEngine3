/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool
{
	class VCToolChain
	{
		static string GetCLArguments_Global(CPPEnvironment CompileEnvironment)
		{
			string Result = "";

			// Prevents the compiler from displaying its logo for each invocation.
			Result += " /nologo";

			// Enable intrinsic functions.
			Result += " /Oi";

			// Pack struct members on 4-byte boundaries.
			Result += " /Zp4";

			// Separate functions for linker.
			Result += " /Gy";

			// Relaxes floating point precision semantics to allow more optimization.
			Result += " /fp:fast";

			// Compile into an .obj file, and skip linking.
			Result += " /c";

			// Allow 400% of the default memory allocation limit.
			Result += " /Zm400";

			// Allow large object files to avoid hitting the 2^16 section limit when running with -StressTestUnity.
			Result += " /bigobj";

			// Disable "The file contains a character that cannot be represented in the current code page" warning for non-US windows.
			Result += " /wd4819";

			// Handle Common Language Runtime support (C++/CLI)
			if( CompileEnvironment.CLRMode == CPPCLRMode.CLREnabled )
			{
				Result += " /clr";
			}

			bool bUseDebugRuntimeLibrary = false;
			bool bUseStaticRuntimeLibrary = false;

			//
			//	Debug
			//
			if( CompileEnvironment.TargetConfiguration == CPPTargetConfiguration.Debug )
			{
				// Use debug runtime in debug configuration.
				bUseDebugRuntimeLibrary = true;

				// Disable compiler optimization.
				Result += " /Od";

				// Favor code size (especially useful for embedded platforms).
				Result += " /Os";

				// Allow inline method expansion unless E&C support is requested.
				if( !BuildConfiguration.bSupportEditAndContinue )
				{
					Result += " /Ob2";
				}

				if( CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win64 )
				{
					// Runtime stack checks are not allowed when compiling for CLR
					if( CompileEnvironment.CLRMode == CPPCLRMode.CLRDisabled )
					{
						Result += " /RTCs";
					}
				}
			}
			//
			//	Release and LTCG
			//
			else
			{
				// Maximum optimizations.
				Result += " /Ox";

				// Favor code speed.
				Result += " /Ot";

				// Only omit frame pointers on the PC (which is implied by /Ox) if wanted.
                if ( BuildConfiguration.bOmitFramePointers == false
                &&  (CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win64) )
                {
                    Result += " /Oy-";
                }

				// Allow inline method expansion.			
				Result += " /Ob2";

				//
				// LTCG
				//
				if( CompileEnvironment.TargetConfiguration == CPPTargetConfiguration.ReleaseLTCG )
				{
					if( BuildConfiguration.bAllowLTCG )
					{
						// Enable link-time code generation.
						Result += " /GL";
					}
	
					// Xbox 360 specific LTCG settings
					if( CompileEnvironment.TargetPlatform == CPPTargetPlatform.Xbox360 )
					{
						// Disable generation of trap instructions around integer divides.
						Result += " /Oc";

						// Perform an additional code scheduling pass before the register allocation phase.
						Result += " /Ou";

						// Enable inline assembly optimization.
						Result += " /Oz";
					}
				}
			}

			//
			//	PC
			//
			if( CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || CompileEnvironment.TargetPlatform == CPPTargetPlatform.Win64 )
			{

			    // SSE options are not allowed when using CLR compilation or the 64 bit toolchain
			    if( CompileEnvironment.CLRMode == CPPCLRMode.CLRDisabled &&
					CompileEnvironment.TargetPlatform != CPPTargetPlatform.Win64 )
			    {
				    // Allow the compiler to generate SSE instructions.
					Result += " /arch:SSE";
			    }
    
			    // Prompt the user before reporting internal errors to Microsoft.
			    Result += " /errorReport:prompt";
    
			    if( CompileEnvironment.CLRMode == CPPCLRMode.CLRDisabled )
			    {
				    // Enable C++ exception handling, but not C exceptions.
				    Result += " /EHsc";
			    }
			    else
			    {
				    // For C++/CLI all exceptions must be left enabled
				    Result += " /EHa";
			    }
			}
			//
			//	Xbox 360
			//
			else if( CompileEnvironment.TargetPlatform == CPPTargetPlatform.Xbox360 )
			{
				// Use static runtime on Xbox 360.
				bUseStaticRuntimeLibrary = true;

				// Pool strings as read-only.
				Result += " /GF";
			}

			// If enabled, create debug information.
			if( CompileEnvironment.bCreateDebugInfo )
			{
				// Store debug info in .pdb files.
				if( BuildConfiguration.bUsePDBFiles )
				{
					// Create debug info suitable for E&C if wanted.
					if( BuildConfiguration.bSupportEditAndContinue
					// We only need to do this in debug as that's the only configuration that supports E&C.
					&& CompileEnvironment.TargetConfiguration == CPPTargetConfiguration.Debug )
					{
						Result += " /ZI";
					}
					// Regular PDB debug information.
					else
					{
						Result += " /Zi";
					}
				}
				// Store C7-format debug info in the .obj files, which is faster.
				else
				{
					Result += " /Z7";
				}
			}

			// Specify the appropriate runtime library based on the platform and config.
			Result += string.Format(
				" /M{0}{1}",
				bUseStaticRuntimeLibrary ? 'T' : 'D',
				bUseDebugRuntimeLibrary ? "d" : ""
				);

			return Result;
		}

		static string GetCLArguments_CPP( CPPEnvironment CompileEnvironment )
		{
			string Result = "";

			// Explicitly compile the file as C++.
			Result += " /TP";

			// C++/CLI requires that RTTI is left enabled
			if( CompileEnvironment.CLRMode == CPPCLRMode.CLRDisabled )
			{
				// Disable C++ RTTI.
				Result += " /GR-";
			}

			// Treat warnings as errors.
			Result += " /WX";

			// Level 4 warnings.
			Result += " /W4";

			return Result;
		}
		
		static string GetCLArguments_C()
		{
			string Result = "";
			
			// Explicitly compile the file as C.
			Result += " /TC";
		
			// Level 0 warnings.  Needed for external C projects that produce warnings at higher warning levels.
			Result += " /W0";

			return Result;
		}

		static string GetLinkArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			// Don't create a side-by-side manifest file for the executable.
			Result += " /MANIFEST:NO";

			// Prevents the linker from displaying its logo for each invocation.
			Result += " /NOLOGO";

			// Output debug info for the linked executable.
			Result += " /DEBUG";

			// Prompt the user before reporting internal errors to Microsoft.
			Result += " /errorReport:prompt";

			//
			//	PC
			//
			if( LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win64 )
			{
				// Set machine type/ architecture to be 64 bit.
				if( LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win64 )
				{
					Result += " /MACHINE:x64";
				}
				// 32 bit executable/ target.
				else
				{
					Result += " /MACHINE:x86";
				}

				// Link for Windows.
				Result += " /SUBSYSTEM:WINDOWS";

				// Allow the OS to load the EXE at different base addresses than its preferred base address.
				Result += " /FIXED:No";

				// Option is only relevant with 32 bit toolchain.
				if( LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win32 )
				{
					// Disables the 2GB address space limit on 64-bit Windows and 32-bit Windows with /3GB specified in boot.ini
					Result += " /LARGEADDRESSAWARE";
				}

				// Explicitly declare that the executable is compatible with Data Execution Prevention.
				Result += " /NXCOMPAT";

				// Set the default stack size.
				Result += " /STACK:5000000,5000000";

				// E&C can't use /SAFESEH.  Also, /SAFESEH isn't compatible with 64-bit linking
				if( !BuildConfiguration.bSupportEditAndContinue &&
					LinkEnvironment.TargetPlatform != CPPTargetPlatform.Win64 )
				{
					// Generates a table of Safe Exception Handlers.  Documentation isn't clear whether they actually mean
					// Structured Exception Handlers.
					Result += " /SAFESEH";
				}

				// Include definition file required for PixelMine's UnrealScript debugger.
				Result += " /DEF:UnrealEngine3.def";

				// Allow delay-loaded DLLs to be explicitly unloaded.
				Result += " /DELAY:UNLOAD";

				if( BuildConfiguration.bCompileAsdll )
				{
					Result += " /DLL";
				}
			}
			//
			//	Xbox 360
			//
			else if (LinkEnvironment.TargetPlatform == CPPTargetPlatform.Xbox360)
			{
				// Set the default stack size.
                if ( LinkEnvironment.TargetConfiguration == CPPTargetConfiguration.Debug )
                {
                    Result += " /STACK:393216,393216";
                }
                else
                {
				    Result += " /STACK:262144,262144";
                }
			}

			//
			//	ReleaseLTCG
			//
			if( LinkEnvironment.TargetConfiguration == CPPTargetConfiguration.ReleaseLTCG )
			{
				// Use link-time code generation.
				Result += " /ltcg";
			}

			//
			//	Shipping binary
			//
			if( LinkEnvironment.bIsShippingBinary )
			{
				// Generate an EXE checksum.
				Result += " /RELEASE";

				// Eliminate unreferenced symbols.
				Result += " /OPT:REF";

				// Remove redundant COMDATs.
				Result += " /OPT:ICF";
			}
			//
			//	Regular development binary. 
			//
			else
			{
				// Keep symbols that are unreferenced.
				Result += " /OPT:NOREF";

				// Disable identical COMDAT folding.
				Result += " /OPT:NOICF";
			}

			// Enable incremental linking if wanted.
			if( BuildConfiguration.bUseIncrementalLinking )
			{
				Result += " /INCREMENTAL";
			}
			// Disabled by default as it can cause issues and forces local execution.
			else
			{
				Result += " /INCREMENTAL:NO";
			}

			return Result;
		}

		public static CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles)
		{
			string Arguments = GetCLArguments_Global(CompileEnvironment);

			// Add include paths to the argument list.
			foreach (string IncludePath in CompileEnvironment.IncludePaths)
			{
				Arguments += string.Format(" /I \"{0}\"", IncludePath);
			}
			foreach (string IncludePath in CompileEnvironment.SystemIncludePaths)
			{
				Arguments += string.Format(" /I \"{0}\"", IncludePath);
			}


			if( CompileEnvironment.CLRMode == CPPCLRMode.CLREnabled )
			{
				// Add .NET framework assembly paths.  This is needed so that C++/CLI projects
				// can reference assemblies with #using, without having to hard code a path in the
				// .cpp file to the assembly's location.				
				foreach (string AssemblyPath in CompileEnvironment.SystemDotNetAssemblyPaths)
				{
					Arguments += string.Format(" /AI \"{0}\"", AssemblyPath);
				}

				// Add explicit .NET framework assembly references				
				foreach( string AssemblyName in CompileEnvironment.FrameworkAssemblyDependencies )
				{
					Arguments += string.Format( " /FU \"{0}\"", AssemblyName );
				}

				// Add private assembly references				
				foreach( PrivateAssemblyInfo CurAssemblyInfo in CompileEnvironment.PrivateAssemblyDependencies )
				{
					Arguments += string.Format( " /FU \"{0}\"", CurAssemblyInfo.FileItem.AbsolutePath );
				}
			}


			// Add preprocessor definitions to the argument list.
			foreach (string Definition in CompileEnvironment.Definitions)
			{
				Arguments += string.Format(" /D \"{0}\"", Definition);
			}

			// Create a compile action for each source file.
			CPPOutput Result = new CPPOutput();
			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = new Action();
				string FileArguments = "";
				bool bIsPlainCFile = Path.GetExtension(SourceFile.AbsolutePathUpperInvariant) == ".C";

				// Add the C++ source file and its included files to the prerequisite item list.
				CompileAction.PrerequisiteItems.Add(SourceFile);
				foreach (FileItem IncludedFile in CompileEnvironment.GetIncludeDependencies(SourceFile))
				{
					CompileAction.PrerequisiteItems.Add(IncludedFile);
				}

				// If this is a CLR file then make sure our dependent assemblies are added as prerequisites
				if( CompileEnvironment.CLRMode == CPPCLRMode.CLREnabled )
				{
					foreach( PrivateAssemblyInfo CurPrivateAssemblyDependency in CompileEnvironment.PrivateAssemblyDependencies )
					{
						CompileAction.PrerequisiteItems.Add( CurPrivateAssemblyDependency.FileItem );
					}
				}

				if (CompileEnvironment.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					// Generate a CPP File that just includes the precompiled header.
					string PCHCPPFilename = Path.GetFileName(CompileEnvironment.PrecompiledHeaderIncludeFilename) + ".PCH.cpp";
					string PCHCPPPath = Path.Combine(CompileEnvironment.OutputDirectory, PCHCPPFilename);
					FileItem PCHCPPFile = FileItem.CreateIntermediateTextFile(
						PCHCPPPath,
						string.Format("#include \"{0}\"\r\n", CompileEnvironment.PrecompiledHeaderIncludeFilename)
						);

					// Add the precompiled header file to the produced items list.
					FileItem PrecompiledHeaderFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + ".pch"
							)
						);
					CompileAction.ProducedItems.Add(PrecompiledHeaderFile);
					Result.PrecompiledHeaderFile = PrecompiledHeaderFile;

					// Add the parameters needed to compile the precompiled header file to the command-line.
					FileArguments += string.Format(" /Yc\"{0}\"", CompileEnvironment.PrecompiledHeaderIncludeFilename);
					FileArguments += string.Format(" /Fp\"{0}\"", PrecompiledHeaderFile.AbsolutePath);
					FileArguments += string.Format(" \"{0}\"", PCHCPPFile.AbsolutePath);
				}
				else
				{
					if (CompileEnvironment.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
					{
						CompileAction.bIsUsingPCH = true;
						CompileAction.PrerequisiteItems.Add(CompileEnvironment.PrecompiledHeaderFile);
						FileArguments += string.Format(" /Yu\"{0}\"", CompileEnvironment.PrecompiledHeaderIncludeFilename);
						FileArguments += string.Format(" /Fp\"{0}\"", CompileEnvironment.PrecompiledHeaderFile.AbsolutePath);
					}
					
					// Add the source file path to the command-line.
					FileArguments += string.Format(" \"{0}\"", SourceFile.AbsolutePath);
				}

				// Add the object file to the produced item list.
				FileItem ObjectFile = FileItem.GetItemByPath(
					Path.Combine(
						CompileEnvironment.OutputDirectory,
						Path.GetFileName(SourceFile.AbsolutePath) + ".obj"
						)
					);
				CompileAction.ProducedItems.Add(ObjectFile);
				Result.ObjectFiles.Add(ObjectFile);
				FileArguments += string.Format(" /Fo\"{0}\"", ObjectFile.AbsolutePath);

				// create PDBs per-file when not using debug info, otherwise it will try to share a PDB file, which causes
				// PCH creation to be serial rather than parallel (when debug info is disabled)
				// See https://udn.epicgames.com/lists/showpost.php?id=50619&list=unprog3
				if (!CompileEnvironment.bCreateDebugInfo || BuildConfiguration.bUsePDBFiles)
				{
					string PDBFileName;
					bool bActionProducesPDB = false;

					// All files using the same PCH are required to share a PDB.
					if( CompileEnvironment.PrecompiledHeaderAction == PrecompiledHeaderAction.Include )
					{
						PDBFileName = Path.GetFileName( CompileEnvironment.PrecompiledHeaderIncludeFilename );
					}
					// Files creating a PCH or ungrouped C++ files use a PDB per file.
					else if (CompileEnvironment.PrecompiledHeaderAction == PrecompiledHeaderAction.Create || !bIsPlainCFile)
					{
						PDBFileName = Path.GetFileName(SourceFile.AbsolutePath);
						bActionProducesPDB = true;
					}
					// Group all plain C files that doesn't use PCH into the same PDB
					else
					{
						PDBFileName = "MiscPlainC";
					}

					// Specify the PDB file that the compiler should write to.
					FileItem PDBFile = FileItem.GetItemByPath(
							Path.Combine(
								CompileEnvironment.OutputDirectory,
								PDBFileName + ".pdb"
								)
							);
					FileArguments += string.Format(" /Fd\"{0}\"", PDBFile.AbsolutePath);

					// Only use the PDB as an output file if we want PDBs and this particular action is
					// the one that produces the PDB (as opposed to no debug info, where the above code
					// is needed, but not the output PDB, or when multiple files share a single PDB, so
					// only the action that generates it should count it as output directly)
					if( BuildConfiguration.bUsePDBFiles && bActionProducesPDB )
					{
						CompileAction.ProducedItems.Add(PDBFile);
						Result.DebugDataFiles.Add(PDBFile);
					}
				}

				// Add C or C++ specific compiler arguments.
				if (bIsPlainCFile)
				{
					FileArguments += GetCLArguments_C();
				}
				else
				{
					FileArguments += GetCLArguments_CPP( CompileEnvironment );
				}

				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = GetVCToolPath(CompileEnvironment.TargetPlatform, CompileEnvironment.TargetConfiguration, "cl");
				CompileAction.bIsVCCompiler = true;
				CompileAction.CommandArguments = Arguments + FileArguments + CompileEnvironment.AdditionalArguments;
				CompileAction.StatusDescription = string.Format("{0}", Path.GetFileName(SourceFile.AbsolutePath));
				CompileAction.StatusDetailedDescription = SourceFile.Description;
				CompileAction.bShouldLogIfExecutedLocally = false;

				// Don't farm out creation of precomputed headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely = CompileEnvironment.PrecompiledHeaderAction != PrecompiledHeaderAction.Create;
			}
			return Result;
		}

		public static CPPOutput CompileRCFiles(CPPEnvironment Environment, List<FileItem> RCFiles)
		{
			CPPOutput Result = new CPPOutput();

			foreach (FileItem RCFile in RCFiles)
			{
				Action CompileAction = new Action();
				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = GetVCToolPath(Environment.TargetPlatform, Environment.TargetConfiguration, "rc");
				CompileAction.StatusDescription = string.Format("{0}", Path.GetFileName(RCFile.AbsolutePath));

				// If we're compiling for 64-bit Windows, also add the _WIN64 definition to the resource
				// compiler so that we can switch on that in the .rc file using #ifdef.
				if( Environment.TargetPlatform == CPPTargetPlatform.Win64 )
				{
					CompileAction.CommandArguments += " /D _WIN64";
				}

				// Language
				CompileAction.CommandArguments += " /l 0x409";

				// Include paths.
				foreach (string IncludePath in Environment.IncludePaths)
				{
					CompileAction.CommandArguments += string.Format(" /i \"{0}\"", IncludePath);
				}

				// Preprocessor definitions.
				foreach (string Definition in Environment.Definitions)
				{
					CompileAction.CommandArguments += string.Format(" /d \"{0}\"", Definition);
				}

				// Add the RES file to the produced item list.
				FileItem CompiledResourceFile = FileItem.GetItemByPath(
					Path.Combine(
						Environment.OutputDirectory,
						Path.GetFileName(RCFile.AbsolutePath) + ".res"
						)
					);
				CompileAction.ProducedItems.Add(CompiledResourceFile);
				CompileAction.CommandArguments += string.Format(" /fo \"{0}\"", CompiledResourceFile.AbsolutePath);
				Result.ObjectFiles.Add(CompiledResourceFile);

				// Add the RC file as a prerequisite of the action.
				CompileAction.PrerequisiteItems.Add(RCFile);
				CompileAction.CommandArguments += string.Format(" \"{0}\"", RCFile.AbsolutePath);

				// Add the files included by the RC file as prerequisites of the action.
				foreach (FileItem IncludedFile in Environment.GetIncludeDependencies(RCFile))
				{
					CompileAction.PrerequisiteItems.Add(IncludedFile);
				}
			}

			return Result;
		}

		public static FileItem LinkFiles(LinkEnvironment LinkEnvironment)
		{
			// Create an action that invokes the linker.
			Action LinkAction = new Action();
			LinkAction.bIsLinker = true;
			LinkAction.WorkingDirectory = Path.GetFullPath(".");
            LinkAction.CommandPath = GetVCToolPath(LinkEnvironment.TargetPlatform, LinkEnvironment.TargetConfiguration, "link");
            
			// Get link arguments.
			LinkAction.CommandArguments = GetLinkArguments(LinkEnvironment);

			// Add delay loaded DLLs.
			if( LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || LinkEnvironment.TargetPlatform == CPPTargetPlatform.Win64 )
			{
				// Delay-load these DLLs.
				foreach (string DelayLoadDLL in LinkEnvironment.DelayLoadDLLs)
				{
					LinkAction.CommandArguments += string.Format(" /DELAYLOAD:\"{0}\"", DelayLoadDLL);
				}
			}

			// Add the library paths to the argument list.
			foreach (string LibraryPath in LinkEnvironment.LibraryPaths)
			{
				LinkAction.CommandArguments += string.Format(" /LIBPATH:\"{0}\"", LibraryPath);
			}

			// Add the excluded default libraries to the argument list.
			foreach (string ExcludedLibrary in LinkEnvironment.ExcludedLibraries)
			{
				LinkAction.CommandArguments += string.Format(" /NODEFAULTLIB:\"{0}\"", ExcludedLibrary);
			}

			// Add the output file as a production of the link action.
			FileItem OutputFile = FileItem.GetItemByPath(LinkEnvironment.OutputFilePath);
			LinkAction.ProducedItems.Add(OutputFile);
			LinkAction.StatusDescription = string.Format("{0}", Path.GetFileName(OutputFile.AbsolutePath));

			// Add the input files to a response file, and pass the response file on the command-line.
			List<string> InputFileNames = new List<string>();
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
				InputFileNames.Add(string.Format("\"{0}\"", InputFile.AbsolutePath));
				LinkAction.PrerequisiteItems.Add(InputFile);
			}
			foreach (string AdditionalLibrary in LinkEnvironment.AdditionalLibraries)
			{
				InputFileNames.Add(string.Format("\"{0}\"", AdditionalLibrary));
			}
			string ResponseFileName = Path.Combine( LinkEnvironment.OutputDirectory, Path.GetFileName( LinkEnvironment.OutputFilePath ) + ".response" );
			LinkAction.CommandArguments += string.Format( " @\"{0}\"", ResponseFile.Create( ResponseFileName, InputFileNames ) );

			// Add the output file to the command-line.
			LinkAction.CommandArguments += string.Format(" /OUT:\"{0}\"", OutputFile.AbsolutePath);

			// Xbox 360 LTCG does not seem to produce those.
			if( LinkEnvironment.TargetPlatform != CPPTargetPlatform.Xbox360 || LinkEnvironment.TargetConfiguration != CPPTargetConfiguration.ReleaseLTCG )
			{
				// Write the import library to the output directory for nFringe support.
				string ImportLibraryFilePath = Path.Combine(
					LinkEnvironment.OutputDirectory,
					Path.GetFileNameWithoutExtension( OutputFile.AbsolutePath ) + ".lib"
					);
				FileItem ImportLibraryFile = FileItem.GetItemByPath( ImportLibraryFilePath );
				LinkAction.CommandArguments += string.Format( " /IMPLIB:\"{0}\"", ImportLibraryFilePath );
				LinkAction.ProducedItems.Add( ImportLibraryFile );

				// An export file is written to the output directory implicitly; add it to the produced items list.
				string ExportFilePath = Path.ChangeExtension( ImportLibraryFilePath, ".exp" );
				FileItem ExportFile = FileItem.GetItemByPath( ExportFilePath );
				LinkAction.ProducedItems.Add( ExportFile );
			}

			if( LinkEnvironment.bCreateDebugInfo )
			{
				// Write the PDB file to the output directory.			
				string PDBFilePath = Path.Combine(LinkEnvironment.OutputDirectory,Path.GetFileNameWithoutExtension(OutputFile.AbsolutePath) + ".pdb");
				FileItem PDBFile = FileItem.GetItemByPath(PDBFilePath);
				LinkAction.CommandArguments += string.Format(" /PDB:\"{0}\"",PDBFilePath);
				LinkAction.ProducedItems.Add(PDBFile);			
			}

			// Add the additional arguments specified by the environment.
			LinkAction.CommandArguments += LinkEnvironment.AdditionalArguments;

			// Only execute linking on the local PC.
			LinkAction.bCanExecuteRemotely = false;

			return OutputFile;
		}

		public static void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
			var BuildProjectAction = new Action();

			// Specify the source file (prerequisite) for the action
			var ProjectFileItem = FileItem.GetExistingItemByPath(ProjectFileName);
			if (ProjectFileItem == null)
			{
				throw new BuildException("Expected C# project file {0} to exist.", ProjectFileName);
			}
			
			// Add the project and the files contained to the prereqs.
			BuildProjectAction.PrerequisiteItems.Add(ProjectFileItem);
			var ProjectPreReqs = VCSharpProject.GetProjectFiles(ProjectFileName);
			var ProjectFolder = Path.GetDirectoryName(ProjectFileName);
			foreach( string ProjectPreReqRelativePath in ProjectPreReqs )
			{
				string ProjectPreReqAbsolutePath = Path.Combine( ProjectFolder, ProjectPreReqRelativePath );
				var ProjectPreReqFileItem = FileItem.GetExistingItemByPath(ProjectPreReqAbsolutePath);
				if( ProjectPreReqFileItem == null )
				{
					throw new BuildException("Expected C# dependency {0} to exist.", ProjectPreReqAbsolutePath);
				}
				BuildProjectAction.PrerequisiteItems.Add(ProjectPreReqFileItem);
			}

			// We might be able to distribute this safely, but it doesn't take any time.
			BuildProjectAction.bCanExecuteRemotely = false;

			// Setup execution via MSBuild.
			BuildProjectAction.WorkingDirectory = Path.GetFullPath(".");
			BuildProjectAction.StatusDescription = string.Format("{0}", Path.GetFileName(ProjectFileName));
			BuildProjectAction.CommandPath = GetDotNetFrameworkToolPath(CompileEnvironment.EnvironmentTargetPlatform, "MSBuild");
			if (CompileEnvironment.TargetConfiguration == CSharpTargetConfiguration.Debug)
			{
				BuildProjectAction.CommandArguments = " /target:rebuild /property:Configuration=Debug";
			}
			else
			{
				BuildProjectAction.CommandArguments = " /target:rebuild /property:Configuration=Release";
			}
			BuildProjectAction.CommandArguments += String.Format(" \"{0}\"", ProjectFileItem.AbsolutePath);

			// We don't want to display all of the regular MSBuild output to the console
			BuildProjectAction.bShouldBlockStandardOutput = false;

			// Specify the output files.
			string PDBFilePath = Path.Combine( Path.GetDirectoryName(DestinationFile), Path.GetFileNameWithoutExtension(DestinationFile) + ".pdb" );
			FileItem PDBFile = FileItem.GetItemByPath( PDBFilePath );
			BuildProjectAction.ProducedItems.Add( FileItem.GetItemByPath(DestinationFile) );		
			BuildProjectAction.ProducedItems.Add( PDBFile );
		}

		/** Accesses the bin directory for the VC toolchain for the specified platform. */
		static string GetVCToolPath(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, string ToolName)
		{	
			// Initialize environment variables required for spawned tools.
			InitializeEnvironmentVariables( Platform );

			// Out variable that is going to contain fully qualified path to executable upon return.
			string VCToolPath = "";

			// On Xbox 360 all tools reside in the same folder.
			if (Platform == CPPTargetPlatform.Xbox360)
			{
                VCToolPath = Path.Combine( Xbox360ToolChain.GetBinDirectory(Configuration), ToolName + ".exe");
			}
			// We need to differentiate between 32 and 64 bit toolchain on Windows.
			else
			{
				// rc.exe resides in the Windows SDK folder.
				if( ToolName.ToUpperInvariant() == "RC" )
				{
					// 64 bit -- we can use the 32 bit version to target 64 bit on 32 bit OS.
					if( Platform == CPPTargetPlatform.Win64 && bSupports64bitExecutables )
					{
						VCToolPath = Path.Combine( WindowsSDKDir, "bin/x64/rc.exe" );
					}
					// 32 bit
					else
					{
						VCToolPath = Path.Combine( WindowsSDKDir, "bin/rc.exe" );
					}						
				}
				// cl.exe and link.exe are found in the toolchain specific folders (32 vs. 64 bit). We do however use the 64 bit linker if available
				// even when targeting 32 bit as it's noticeably faster.
				else
				{
					bool bIsRequestingLinkTool = ToolName.ToUpperInvariant() == "LINK";
					// 64 bit (named AMD64 with Visual Studio). If available, use 64 bit linker on 64 bit system even when targeting 32 bit as it's faster.
					if( Platform == CPPTargetPlatform.Win64 || (bIsRequestingLinkTool && bSupports64bitExecutables) )
					{
						// XGE does not support intercepting 64 bit tools. Use cross compiler. Ditto for 32 bit systems.
						if( (BuildConfiguration.bAllowXGE && !bIsRequestingLinkTool) || !bSupports64bitExecutables )
						{
							VCToolPath = Path.Combine( Environment.GetEnvironmentVariable( "VS90COMNTOOLS" ), "../../VC/bin/x86_amd64/" + ToolName + ".exe" );
						}
						// Use native 64 bit apps if either linking (executed locally) or when not using XGE.
						else						
						{
							VCToolPath = Path.Combine( Environment.GetEnvironmentVariable( "VS90COMNTOOLS" ), "../../VC/bin/amd64/" + ToolName + ".exe" );
						}
					}
					// Use 32 bit for cl.exe and other tools, or for link.exe if 64 bit path doesn't exist and we're targeting 32 bit.
					if( VCToolPath.Length == 0 || !File.Exists(VCToolPath) )
					{
						VCToolPath = Path.Combine( Environment.GetEnvironmentVariable("VS90COMNTOOLS"), "../../VC/bin/" + ToolName + ".exe" );
					}
				}
			}

			return VCToolPath;
		}

		/** Accesses the directory for .NET Framework binaries such as MSBuild */
		static string GetDotNetFrameworkToolPath(CPPTargetPlatform Platform, string ToolName)
		{
			// Initialize environment variables required for spawned tools.
			InitializeEnvironmentVariables(Platform);

			string DotNetFrameworkBinDir = Path.Combine(Environment.GetEnvironmentVariable("FrameworkDir"), Environment.GetEnvironmentVariable("Framework35Version"));			
			string ToolPath = Path.Combine(DotNetFrameworkBinDir, ToolName + ".exe");
			return ToolPath;
		}

		/** Helper to only initialize environment variables once. */
		static bool bAreEnvironmentVariablesAlreadyInitialized = false;
		
		/** Whether we can execute 64 bit executables. E.g. the 64 bit linker might be installed on a 32 bit OS. */
		static bool bSupports64bitExecutables = true;

		/** Installation folder of the Windows SDK, e.g. C:\Program Files\Microsoft SDKs\Windows\v6.0A\ */
		static string WindowsSDKDir = "";

		/**
		 * Initializes environment variables required by toolchain. Different for 32 and 64 bit.
		 */
		static void InitializeEnvironmentVariables( CPPTargetPlatform Platform )
		{
			if( !bAreEnvironmentVariablesAlreadyInitialized )
			{
				string VCVarsBatchFile = "";

				// 64 bit tool chain.
				if( Platform == CPPTargetPlatform.Win64 )
				{
					VCVarsBatchFile = Path.Combine( Environment.GetEnvironmentVariable( "VS90COMNTOOLS" ), "../../VC/bin/x86_amd64/vcvarsx86_amd64.bat" );
				}
				// The 32 bit vars batch file in the binary folder simply points to the one in the common tools folder.
				else
				{
					VCVarsBatchFile = Path.Combine( Environment.GetEnvironmentVariable( "VS90COMNTOOLS" ), "vsvars32.bat" );
				}
				Utils.SetEnvironmentVariablesFromBatchFile(VCVarsBatchFile);

				// @todo remove: We only need to add the XDK bin path to the environment to find imagexex when using the TechPreview compiler.
				Environment.SetEnvironmentVariable( "PATH", Utils.ResolveEnvironmentVariable( "%PATH%;%XEDK%\\bin\\win32") );
				
				// Retrieve the Windows SDK path from the registry. This is needed as Visual Studio 2008 no longer includes
				// the Platform/ Windows SDK but rather installs it.
				RegistryKey Key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\");
				WindowsSDKDir = (string) Key.GetValue("CurrentInstallFolder");

				// Patch up the SDK dir to make sure we point to the expected version
				string WindowsSdkCurentVersion = (string)Key.GetValue( "CurrentVersion" );
				string WindowsSdkProductVersion = (string)Key.GetValue( "ProductVersion" );
				WindowsSDKDir = WindowsSDKDir.Replace( WindowsSdkCurentVersion, WindowsSdkProductVersion );

				Trace.Assert( WindowsSdkProductVersion == "v6.0A", "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\ProductVersion NOT equal to expected version (v6.0A)!" );

				// Lib and bin folders have a x64 subfolder for 64 bit development.
				string ConfigSuffix = "";
				if( Platform == CPPTargetPlatform.Win64 )
				{
					ConfigSuffix = "\\x64";
				}

				// Manually include the Windows SDK path in the LIB/ INCLUDE/ PATH variables.
				Environment.SetEnvironmentVariable( "PATH", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "bin" + ConfigSuffix + ";%PATH%") );
				Environment.SetEnvironmentVariable( "LIB", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "lib" + ConfigSuffix + ";%LIB%") );
				Environment.SetEnvironmentVariable( "INCLUDE", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "include;%INCLUDE%") );

				// Determine 32 vs 64 bit. Worth noting that WOW64 will report x86.
				bSupports64bitExecutables = Environment.GetEnvironmentVariable( "PROCESSOR_ARCHITECTURE" ) == "AMD64" ? true : false;

				bAreEnvironmentVariablesAlreadyInitialized = true;
			}			
		}
	};
}