/** version of UTTeleporter with support for putting the render-to-texture on whatever mesh the LD wants */
class UTTeleporterCustomMesh extends UTTeleporterBase;

var() StaticMeshComponent Mesh;

simulated function InitializePortalEffect(Actor Dest)
{
	if (Mesh.StaticMesh == None)
	{
		`Warn("Teleporter has no StaticMesh assigned!");
	}
	else
	{
		PortalMaterial = Mesh.GetMaterial(0);
		Super.InitializePortalEffect(Dest);
		Mesh.SetMaterial(0, PortalMaterialInstance);
	}
}

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=PortalMesh
		BlockActors=false
		CollideActors=true
		BlockRigidBody=false
		StaticMesh=StaticMesh'EditorMeshes.TexPropPlane'
		Materials[0]=MaterialInterface'EngineMaterials.ScreenMaterial'
		Translation=(Z=125.0)
	End Object
	Mesh=PortalMesh
	CollisionComponent=PortalMesh
	Components.Add(PortalMesh)

	Begin Object Name=SceneCapturePortalComponent0
		MaxUpdateDist=0.0
	End Object

	Begin Object Name=CollisionCylinder
		CollideActors=false
	End Object

	PortalTextureParameter=ScreenTex

	Physics=PHYS_Interpolating //@hack: so IsOverlapping() works (otherwise you get stuck in the teleporter)
}
