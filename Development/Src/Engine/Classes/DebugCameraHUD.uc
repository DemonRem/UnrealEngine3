// PCF Begin (Debug Camera)
//-----------------------------------------------------------
//
//-----------------------------------------------------------
class DebugCameraHUD extends HUD
	config(Game);

simulated event PostBeginPlay()
{
    super.PostBeginPlay();
}

event PostRender()
{
    local   DebugCameraController DCC;
	local	float	xl,yl,Y;
	local	String	MyText;
    local   vector  CamLoc, ZeroVec;
    local   rotator CamRot;
   	local   TraceHitInfo	HitInfo;
	local   Actor			HitActor;

    local   vector HitLoc, HitNormal;
    super.PostRender();

    DCC = DebugCameraController( self.PlayerOwner );
    if( DCC != none )
    {
	    Canvas.SetDrawColor(0, 0, 255, 255);
	    MyText = "DebugCameraHUD";
	    Canvas.Font = class'Engine'.Static.GetSmallFont();
	    Canvas.StrLen(MyText, XL, YL);
	    Y = YL;//*1.67;
	    YL += 2*Y;
	    Canvas.SetPos( 4, YL);
	    Canvas.DrawText(MyText, true);

	    Canvas.SetDrawColor(128, 128, 128, 255);
        //DCC.GetPlayerViewPoint( CamLoc, CamRot );
 	    CamLoc = DCC.PlayerCamera.CameraCache.POV.Location;
	    CamRot = DCC.PlayerCamera.CameraCache.POV.Rotation;

	    YL += Y;
	    Canvas.SetPos(4,YL);
      	Canvas.DrawText("CamLoc:" $ CamLoc @ "CamRot:" $ CamRot );

        HitActor = Trace(HitLoc, HitNormal, vector(camRot) * 5000 * 20 + CamLoc, CamLoc, true, ZeroVec, HitInfo);
        if( HitActor != None)
        {
           YL += Y;
	       Canvas.SetPos(4,YL);
           Canvas.DrawText("HitLoc:" $ HitLoc @ "HitNorm:" $ HitNormal );
           YL += Y;
	       Canvas.SetPos(4,YL);
           Canvas.DrawText("HitActor: '" $ HitActor.Name $ "'" );

           YL += Y;
	       Canvas.SetPos(4,YL);
	       if( HitInfo.Material != none )
               Canvas.DrawText("HitMaterial:" $ HitInfo.Material.Name );
           else
               Canvas.DrawText("HitMaterial: NONE" );

           DrawDebugLine( HitLoc, HitLoc+HitNormal*30, 255,255,1255 );
        }
        else
        {
           YL += Y;
	       Canvas.SetPos(4,YL);
           Canvas.DrawText( "Not trace hit" );
        }
	}
}

DefaultProperties
{
	bHidden=false
}
// PCF Begin (Debug Camera)
