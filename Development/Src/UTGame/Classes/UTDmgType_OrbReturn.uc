class UTDmgType_OrbReturn extends UTDamageType
	abstract;

simulated static function DrawKillIcon(Canvas Canvas, float ScreenX, float ScreenY, float HUDScaleX, float HUDScaleY)
{
	local color CanvasColor;

	`log("DRAW ORB");
	// save current canvas color
	CanvasColor = Canvas.DrawColor;

	// draw orb shadow
	Canvas.DrawColor = class'UTHUD'.default.BlackColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX - 2, ScreenY - 2 );
	Canvas.DrawTile(class'UTMapInfo'.default.HUDIconsT, 4 + HUDScaleX * 96, 4 + HUDScaleY * 64, class'UTOnslaughtFlag'.default.IconXStart,  class'UTOnslaughtFlag'.default.IconYStart,  class'UTOnslaughtFlag'.default.IconXWidth,  class'UTOnslaughtFlag'.default.IconYWidth);

	// draw the orb icon
	Canvas.DrawColor =  class'UTHUD'.default.WhiteColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX, ScreenY );
	Canvas.DrawTile(class'UTMapInfo'.default.HUDIconsT, HUDScaleX * 96, HUDScaleY * 64, class'UTOnslaughtFlag'.default.IconXStart,  class'UTOnslaughtFlag'.default.IconYStart,  class'UTOnslaughtFlag'.default.IconXWidth,  class'UTOnslaughtFlag'.default.IconYWidth);
	Canvas.DrawColor = CanvasColor;
}

defaultproperties
{
}