
using System;
using System.Xml;
using System.Xml.Serialization;


namespace UnrealConsole
{
	/// <summary>
	/// Summary description for SavedSettings.
	/// </summary>
	public class SavedSettings
	{
		/// <summary>
		/// Holds the last used platform
		/// </summary>
		[XmlAttribute]
		public string LastPlatform = null;
		/// <summary>
		/// Holds the last connected targets for each platform
		/// </summary>
		[XmlArray]
		public string[] LastTargets;

		/// <summary>
		/// Default ctor for XML serialization
		/// </summary>
		public SavedSettings()
		{
		}
	}
}
