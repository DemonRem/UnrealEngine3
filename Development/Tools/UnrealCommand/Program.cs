/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Threading;
using System.Diagnostics;

namespace UnrealCommand
{
	partial class Program
	{
		static int ReturnCode = 0;
		static string MainCommand = "";
		static string GameName = "";
		static string GameConfiguration = "";

		/**
		 * Main control loop
		 */
		static int Main(string[] args)
		{
			if (!ParseCommandLine(args))
			{
				Log("Usage: UnrealCommand <Command> <GameName> <Configuration> [parameters]");
				Log("");
				Log("Common commands:");
				Log("  ... PreprocessShader <GameName> <Configuration> <InputFile.glsl> <OutputFile.i> [-CleanWhitespace]");
				Log("");
				Log("Commands: PreprocessShader");
				Log("");
				Log("Examples:");
				Log("  UnrealCommand PreprocessShader SwordGame Shipping Input.glsl Output.i");
				return (1);
			}

			switch (MainCommand.ToLower())
			{
				case "preprocessshader":
					PreprocessShader(args);
					break;
				default:
					break;
			}

			return ( ReturnCode );
		}

		static private void Log(string Line)
		{
			Console.WriteLine(Line);
		}

		static void Error(string Line)
		{
			Console.ForegroundColor = ConsoleColor.Red;
			Log("ITP ERROR: " + Line);
			Console.ResetColor();
		}

		static private void Warning(string Line)
		{
			Console.ForegroundColor = ConsoleColor.Yellow;
			Log("ITP WARNING: " + Line);
			Console.ResetColor();
		}

		static string GetVC9Path()
		{
			string ProgramFilesFolder = "C:\\Program Files (x86)";

			string Path = ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\Common7\\IDE;";
			Path += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\BIN;";
			Path += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\Common7\\Tools;";
			Path += "C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\bin;";
			Path += "C:\\Windows\\Microsoft.NET\\Framework\\v3.5;";
			Path += "C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;";
			Path += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\VCPackages;";

			return Path;
		}

		static string GetCommonPath()
		{
			string ProgramFilesFolder = "C:\\Program Files (x86)";

			string Path = ProgramFilesFolder;
			Path += "C:\\Windows\\system32;";
			Path += "C:\\Windows;";
			Path += "C:\\Windows\\System32\\Wbem;";

			return (Path);
		}

		static void SetMSVC9EnvVars()
		{
			string ProgramFilesFolder = "C:\\Program Files (x86)";

			Environment.SetEnvironmentVariable("VSINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0");
			Environment.SetEnvironmentVariable("VCINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC");

			Environment.SetEnvironmentVariable("INCLUDE", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\INCLUDE;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\INCLUDE;C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\include;");
			Environment.SetEnvironmentVariable("LIB", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\LIB;C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\lib;");
			Environment.SetEnvironmentVariable("LIBPATH", "C:\\Windows\\Microsoft.NET\\Framework\\v3.5;C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\LIB;");
			Environment.SetEnvironmentVariable("Path", GetVC9Path() + GetCommonPath());
		}

		static int RunExecutableAndWait(string ExeName, string ArgumentList, out string StdOutResults)
		{
			// Create the process
			ProcessStartInfo PSI = new ProcessStartInfo(ExeName, ArgumentList);
			PSI.RedirectStandardOutput = true;
			PSI.UseShellExecute = false;
			PSI.CreateNoWindow = true;
			Process NewProcess = Process.Start(PSI);

			// Wait for the process to exit and grab it's output
			StdOutResults = NewProcess.StandardOutput.ReadToEnd();
			NewProcess.WaitForExit();
			return NewProcess.ExitCode;
		}

		static private bool ParseCommandLine(string[] Arguments)
		{
			if (Arguments.Length == 0)
			{
				return (false);
			}

			if (Arguments.Length == 1)
			{
				MainCommand = Arguments[0];
			}
			else if (Arguments.Length == 2)
			{
				MainCommand = Arguments[0];
				GameName = Arguments[1];
			}
			else if (Arguments.Length >= 3)
			{
				MainCommand = Arguments[0];
				GameName = Arguments[1];
				GameConfiguration = Arguments[2];
			}

			return (true);
		}

		static private bool CheckArguments()
		{
			if (GameName.Length == 0 || GameConfiguration.Length == 0)
			{
				Error("Invalid number of arguments");
				return (false);
			}

			return (true);
		}
	}
}
