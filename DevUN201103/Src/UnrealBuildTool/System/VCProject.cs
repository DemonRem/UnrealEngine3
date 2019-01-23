/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.XPath;

namespace UnrealBuildTool
{
	/** A Visual C++ project. */
	class VCProject
	{
		/** Reads the list of files in a project from the specified project file. */
		public static List<string> GetProjectFiles(string ProjectPath)
		{
			using (FileStream ProjectStream = new FileStream(ProjectPath, FileMode.Open, FileAccess.Read))
			{
				// Parse the project's root node.
				XPathDocument Doc = new XPathDocument(ProjectStream);
				XPathNavigator Nav = Doc.CreateNavigator();
				XPathNavigator Version = Nav.SelectSingleNode("/VisualStudioProject/@Version");
				if (Version != null && (Version.Value =="9.00" || Version.Value == "9,00"))
				{
					XPathNodeIterator Iter = Nav.Select("/VisualStudioProject/Files//File/@RelativePath");
					List<string> RelativeFilePaths = new List<string>(Iter.Count);
					foreach (XPathNavigator It in Iter)
					{
						RelativeFilePaths.Add(It.Value);
					}
					return RelativeFilePaths;
				}
			}
			return new List<string>();
		}
	}

	/** A Visual C# project. */
	class VCSharpProject
	{
		/** Reads the list of dependencies from the specified project file. */
		public static List<string> GetProjectFiles(string ProjectPath)
		{
			List<string> RelativeFilePaths = new List<string>();
			XmlDocument Doc = new XmlDocument();
			Doc.Load(ProjectPath);
				
			var Tags = new string[]{ "Compile", "Page", "Resource" };
			foreach( var Tag in Tags )
			{
				var Elements = Doc.GetElementsByTagName( Tag );	
				foreach( XmlElement Element in Elements )
				{		
					RelativeFilePaths.Add( Element.GetAttribute("Include") );
				}
			}

			return RelativeFilePaths;
		}
	}
}
