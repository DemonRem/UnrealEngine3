/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;

namespace UnrealBuildTool
{
	class PS3ToolChain
	{
		public static CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, IEnumerable<FileItem> SourceFiles)
		{
			return null;
		}

		public static FileItem LinkFiles(LinkEnvironment LinkEnvironment)
		{
			return null;
		}
	};

	class SNCToolChain
	{
		public static CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, IEnumerable<FileItem> SourceFiles)
		{
			return null;
		}

		public static FileItem LinkFiles(LinkEnvironment LinkEnvironment)
		{
			return null;
		}

	};

	class SPUToolChain
	{
		public static CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, IEnumerable<FileItem> SourceFiles)
		{
			return null;
		}

		public static FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bIsSpursTask)
		{
			return null;
		}
	};

}
