using System;
using System.Xml;
using System.Xml.Serialization;

namespace UnrealFrontend
{
	/// <summary>
	/// Summary description for CookerSettings.
	/// </summary>
	public class CookerSettings
	{
		/// <summary>
		/// This is the set of supported game names
		/// </summary>
		[XmlArray("Platforms")]
		public string[] Platforms = new string[0];
		/// <summary>
		/// This is the set of supported game names
		/// </summary>
		[XmlArray("Games")]
		public string[] Games = new string[0];
		/// <summary>
		/// This is the set of supported PC build configurations
		/// </summary>
		[XmlArray("PCConfigs")]
		public string[] PCConfigs = new string[0];
		/// <summary>
		/// This is the set of supported Console build configurations
		/// </summary>
		[XmlArray("ConsoleConfigs")]
		public string[] ConsoleConfigs = new string[0];

		/// <summary>
		/// Needed for XML serialization. Does nothing
		/// </summary>
		public CookerSettings()
		{
		}

		/// <summary>
		/// Constructor that sets the defaults for the app
		/// </summary>
		/// <param name="Ignored">Ignored. Used to have a specialized ctor</param>
		public CookerSettings(int Ignored)
		{
			// Build the default set of platforms
			Platforms = new string[2];
			Platforms[0] = "Xenon";
			Platforms[1] = "PS3";
			// Build the default set of games
			Games = new string[3];
			Games[0] = "ExampleGame";
			Games[1] = "UTGame";
			Games[2] = "WarGame";
			// Now the default set of PC configs
			PCConfigs = new string[2];
			PCConfigs[0] = "Release";
			PCConfigs[1] = "Debug";
			// Now the default set of Console configs
			ConsoleConfigs = new string[3];
			ConsoleConfigs[0] = "Release";
			ConsoleConfigs[1] = "ReleaseLTCG";
			ConsoleConfigs[2] = "Debug";
		}
	}
}
