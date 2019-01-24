/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */


namespace UnrealFrontend
{
	public class UISettings
	{
		public UISettings()
		{
			WindowLeft = 10;
			WindowTop = 10;
			WindowWidth = 970;
			WindowHeight = 800;

			ProfileListWidth = 200;
			ProfilesSectionHeight = 560;
		}

		public double WindowLeft { get; set; }
		public double WindowTop { get; set; }
		public double WindowWidth { get; set; }
		public double WindowHeight { get; set; }

		public double ProfileListWidth { get; set; }
		public double ProfilesSectionHeight { get; set; }
	}
}
