/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MeshComponent extends PrimitiveComponent
	native
	noexport
	abstract;

var(Rendering) const array<MaterialInterface>	Materials;


/**
 * @param ElementIndex - The element to access the material of.
 * @return the material used by the indexed element of this mesh.
 */
native function MaterialInterface GetMaterial(int ElementIndex);

/**
 * Changes the material applied to an element of the mesh.
 * @param ElementIndex - The element to access the material of.
 * @return the material used by the indexed element of this mesh.
 */
native function SetMaterial(int ElementIndex, MaterialInterface Material);

/** @return The total number of elements in the mesh. */
native function int GetNumElements();

/**
 * Creates a material instance for the specified element index.  The parent of the instance is set to the material being replaced.
 * @param ElementIndex - The index of the skin to replace the material for.
 */
function MaterialInstanceConstant CreateAndSetMaterialInstanceConstant(int ElementIndex)
{
	local MaterialInstanceConstant Instance;

	// Create the material instance.
	Instance = new(Outer) class'MaterialInstanceConstant';
	Instance.SetParent(GetMaterial(ElementIndex));

	// Assign it to the given mesh element.
	// This MUST be done after setting the parent; otherwise the component will use the default material in place of the invalid material instance.
	SetMaterial(ElementIndex,Instance);

	return Instance;
}

/**
* Creates a material instance for the specified element index.  The parent of the instance is set to the material being replaced.
* @param ElementIndex - The index of the skin to replace the material for.
*/
function MaterialInstanceTimeVarying CreateAndSetMaterialInstanceTimeVarying(int ElementIndex)
{
	local MaterialInstanceTimeVarying Instance;

	// Create the material instance.
	Instance = new(Outer) class'MaterialInstanceTimeVarying';
	Instance.SetParent(GetMaterial(ElementIndex));

	// Assign it to the given mesh element.
	SetMaterial(ElementIndex,Instance);

	return Instance;
}


defaultproperties
{
	CastShadow=TRUE
	bAcceptsLights=TRUE
	bUseAsOccluder=TRUE
}
