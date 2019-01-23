/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __GENERICBROWSERCONTEXTMENUS_H__
#define __GENERICBROWSERCONTEXTMENUS_H__

/**
 * Baseclass for all generic browser type context menus.
 */
class WxMBGenericBrowserContext : public wxMenu
{
public:
	void AppendObjectMenu();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Object
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Object : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Object();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Archetype.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Archetype : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Archetype();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Material.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Material : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Material();
};

/*-----------------------------------------------------------------------------
WxMBGenericBrowserContext_MaterialInstanceConstant
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_MaterialInstanceConstant : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_MaterialInstanceConstant();
};


/*-----------------------------------------------------------------------------
WxMBGenericBrowserContext_MaterialInstanceTimeVarying
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_MaterialInstanceTimeVarying : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_MaterialInstanceTimeVarying();
};



/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Texture.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Texture : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Texture();
};

/*-----------------------------------------------------------------------------
WxMBGenericBrowserContext_RenderTexture
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_RenderTexture : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_RenderTexture();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_StaticMesh.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_StaticMesh : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_StaticMesh();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Sound.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Sound : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Sound();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_SoundCue.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_SoundCue : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_SoundCue();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_SpeechRecognition.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_SpeechRecognition : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_SpeechRecognition();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_PhysicsAsset.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_PhysicsAsset : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_PhysicsAsset();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_FaceFXAsset.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_FaceFXAsset : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_FaceFXAsset();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_FaceFXAnimSet.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_FaceFXAnimSet : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_FaceFXAnimSet();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_Skeletal.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_Skeletal : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_Skeletal();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_LensFlare
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_LensFlare : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_LensFlare();
};


/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_ParticleSystem.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_ParticleSystem : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_ParticleSystem();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_AnimSet.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_AnimSet : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_AnimSet();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_MorphTargetSet.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_MorphTargetSet : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_MorphTargetSet();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_AnimTree.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_AnimTree : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_AnimTree();
};

/*-----------------------------------------------------------------------------
	WxMBGenericBrowserContext_PostProcess.
-----------------------------------------------------------------------------*/

class WxMBGenericBrowserContext_PostProcess : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_PostProcess();
};

#endif // __GENERICBROWSERCONTEXTMENUS_H__
