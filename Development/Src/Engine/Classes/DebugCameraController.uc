// PCF Begin (Debug Camera)
//-----------------------------------------------------------
// Debug Camera Controller
//
// To turn it on, please press Alt+C or both (left and right) analogs on xbox pad
// After turning:
//   WASD  | Left Analog - moving
//   Mouse | Right Analog - rotating
//   Shift | XBOX_KeyB - move faster
//   Q/E   | LT/RT - move Up/Down
//   Enter | XBOX_KeyA - to call "FreezeRendering" console command
//   Alt+C | LeftThumbstick - to toggle debug camera
//
//-----------------------------------------------------------
class DebugCameraController extends PlayerController;

var PlayerController        OryginalControllerRef;
var Player                  OryginalPlayer;
var bool                    bIsFrozenRendering;
var	DrawFrustumComponent	DrawFrustum;

simulated event PostBeginPlay()
{
    super.PostBeginPlay();

    // if hud is existing, delete it and create new hud for debug camera
	if ( myHUD != None )
		myHUD.Destroy();
	myHUD = Spawn( class'DebugCameraHUD', self);
}

 /*
 *  Function called on activation debug camera controller
 */
 function OnActivate( PlayerController PC )
{
    if(self.DrawFrustum==None) {
        self.DrawFrustum = new(PC.PlayerCamera) class'DrawFrustumComponent';
    }
    self.DrawFrustum.SetHidden( false );
    PC.SetHidden(false);
    PC.PlayerCamera.SetHidden(false);

    self.DrawFrustum.FrustumAngle = PC.PlayerCamera.CameraCache.POV.FOV;
    self.DrawFrustum.SetAbsolute(true, true, false);
    self.DrawFrustum.SetTranslation(PC.PlayerCamera.CameraCache.POV.Location);
    self.DrawFrustum.SetRotation(PC.PlayerCamera.CameraCache.POV.Rotation);

    PC.PlayerCamera.AttachComponent(self.DrawFrustum);
    ConsoleCommand("show camfrustums"); //called to render camera frustums from oryginal player camera
}

 /*
 *  Function called on deactivation debug camera controller
 */
function OnDeactivate( PlayerController PC )
{
    self.DrawFrustum.SetHidden( true );
    ConsoleCommand("show camfrustums");
    PC.PlayerCamera.DetachComponent(self.DrawFrustum);
    PC.SetHidden(true);
    PC.PlayerCamera.SetHidden(true);
}

//function called from key bindings command to save information about
// turrning on/off FreezeRendering command.
exec function SetFreezeRendering()
{
     ConsoleCommand("FreezeRendering");
     bIsFrozenRendering = !bIsFrozenRendering;
}

//function called from key bindings command
exec function MoreSpeed()
{
    self.bRun = 2;
}

//function called from key bindings command
exec function NormalSpeed()
{
    self.bRun = 0;
}

/*
 * Switch from debug camera controller to local player controller
 */
function DisableDebugCamera()
{
    if( OryginalControllerRef != none )
    {
        // restore FreezeRendering command state before quite
        if( bIsFrozenRendering==true ) {
            ConsoleCommand("FreezeRendering");
            bIsFrozenRendering = false;
        }
        if( OryginalPlayer != none )
        {
            self.OnDeactivate( OryginalControllerRef );
            OryginalPlayer.SwitchController( OryginalControllerRef );
            OryginalControllerRef = none;
        }
    }
}

defaultproperties
{
 	InputClass=class'Engine.DebugCameraInput'
    OryginalControllerRef=None
    OryginalPlayer=None
    bIsFrozenRendering=false

    DrawFrustum=none
	bHidden=FALSE
	bHiddenEd=FALSE

}
// PCF End
