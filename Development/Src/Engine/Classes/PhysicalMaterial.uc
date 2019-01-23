/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class PhysicalMaterial extends Object
	native(Physics)
	collapsecategories
	hidecategories(Object);


// Used internally by physics engine.
var	transient int	MaterialIndex;


// Surface properties
var()	float	Friction;
var()	float	Restitution;
var()	bool	bForceConeFriction;

// Object properties
var()	float	Density;
var()	float	AngularDamping;
var()	float	LinearDamping;
var()	float	MagneticResponse;
var()	float	WindResponse;

// This impact/slide system is temporary. We need a system that looks at both PhysicalMaterials, but that is something for the future.

// Impact effects 
var(Impact)		float						ImpactThreshold;
var(Impact)		float						ImpactReFireDelay;
var(Impact)		ParticleSystem				ImpactEffect;
var(Impact)		SoundCue					ImpactSound;

// Slide effects
var(Slide)		float						SlideThreshold;
var(Slide)		float						SlideReFireDelay;
var(Slide)		ParticleSystem				SlideEffect;
var(Slide)		SoundCue					SlideSound;


/**
* The PhysicalMaterial objects now have a parent reference / pointer.  This allows
* you to make single inheritance hierarchies of PhysicalMaterials.  Specifically
* this allows one to set default data and then have subclasses over ride that data.
* (e.g.  For all materials in the game we are going to say the default Impact Sound
* is SoundA.  Now for a Tin Shed we can make a Metal Physical Material and set its
* parent pointer to the Default Material.  And then for our Metal PhysicalMaterial
* we say:  Play SoundB for Pistols and Rifles.  Leaving everything else blank, our
* code can now traverse up the tree to the Default PhysicalMaterial and read the
* values out of that.
*
* This allows for very specific and interesting behavior that is for the most part
* completely in the hands of your content creators.
*
* A programmer is needed only to create the orig set of parameters and then it is
* all data driven parameterization!
*
**/
var(Parent) PhysicalMaterial Parent;

var(PhysicalProperties) export editinline PhysicalMaterialPropertyBase PhysicalMaterialProperty;



cpptext
{
	// UObject interface
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/**
     * This will fix any old PhysicalMaterials that were created in the PhysMaterial's outer instead
	 * of correctly inside the PhysMaterial.  This will allow "broken" PhysMaterials to be renamed.
	 **/
	virtual UBOOL Rename( const TCHAR* InName, UObject* NewOuter, ERenameFlags Flags );
}




/** finds a physical material property of the desired class, querying the parent if this material doesn't have it
 * @param DesiredClass the class of physical material property to search for
 * @return a PhysicalMaterialPropertyBase matching the desired class, or none if there isn't one
 */
function PhysicalMaterialPropertyBase GetPhysicalMaterialProperty(class<PhysicalMaterialPropertyBase> DesiredClass)
{
	if (PhysicalMaterialProperty != None && ClassIsChildOf(PhysicalMaterialProperty.Class, DesiredClass))
	{
		return PhysicalMaterialProperty;
	}
	else if (Parent != None)
	{
		return Parent.GetPhysicalMaterialProperty(DesiredClass);
	}
	else
	{
		return None;
	}
}

defaultproperties
{
	Friction=0.7
	Restitution=0.3

	Density=1.0
	AngularDamping=0.0
	LinearDamping=0.01
	MagneticResponse=0.0
	WindResponse=0.0
}
