using System;
using System.Xml;
using System.Xml.Serialization;


// Defines optional per-game settings for the cooker
// See ExampleGame\Config\CookerFrontend_game.xml for an .xml file to copy for your own game

namespace CookerTools
{
	public class PathInfo
	{
		/// <summary>
		/// Additional paths to copy to target
		/// </summary>
		[XmlAttribute]
		public string Path;

		/// <summary>
		/// Wildcard inside path to search for files
		/// </summary>
		[XmlAttribute]
		public string Wildcard;

		/// <summary>
		/// Optional destination directory
		/// </summary>
		[XmlAttribute]
		public string DestPath = null;

		/// <summary>
		/// Recursive copy?
		/// </summary>
		[XmlAttribute]
		public bool bIsRecursive;

		/// <summary>
		/// Copy to target? (Will always copy when syncing to a UNC path)
		/// If false, the file will not be put in the TOC
		/// </summary>
		[XmlAttribute]
		public bool bIsForTarget=true;

	};
	/// <summary>
	/// Summary description for CookerSettings.
	/// </summary>
	public class GameSettings
	{
		/// <summary>
		/// This is the set of supported game names
		/// </summary>
		[XmlArray("SyncPaths")]
		public PathInfo[] SyncPaths = new PathInfo[0];

		/// <summary>
		/// This is the set of supported game names
		/// </summary>
		[XmlAttribute]
		public string XGDFileRelativePath;

		/// <summary>
		/// Needed for XML serialization. Does nothing
		/// </summary>
		public GameSettings()
		{
		}
	}

	/// <summary>
	/// Summary description for per-platform CookerSettings.
	/// </summary>
	public class PlatformSettings
	{
		/// <summary>
		/// This is the set of supported game names
		/// </summary>
		[XmlArray("SyncPaths")]
		public PathInfo[] SyncPaths = new PathInfo[0];

		/// <summary>
		/// Needed for XML serialization. Does nothing
		/// </summary>
		public PlatformSettings()
		{
		}
	}
}
