/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Credits scene for UT3.
 */
class UTUIFrontEnd_Credits extends UTUIFrontEnd
	native(UIFrontEnd);

struct native CreditsImageSet
{
	var transient Surface  TexImage;
	var string			   TexImageName;
	var TextureCoordinates TexCoords;
	var array<string>	   LabelMarkup;
};

/** How long to show the scene for. */
var() float	SceneTimeInSec;

/** How much to scroll the credits text by. */
var() float ScrollAmount;

/** How much of a delay to have before starting to show gears photos. */
var() float DelayBeforePictures;

/** How much of a delay to have after pictures end. */
var() float DelayAfterPictures;

/** Record when we started displaying. */
var transient float StartTime;

/** A set of images and labels to cycle between. */
var	transient array<CreditsImageSet>	ImageSets;

/** Which object set to use for fading. */
var transient int CurrentObjectOffset;

/** Which image set we are currently on. */
var transient int CurrentImageSet;

/** Labels that hold quotes for each person. */
var transient UILabel QuoteLabels[6];

/** Photos of each person. */
var transient UIImage PhotoImage[2];

/** Labels to hold the scrolling credits text. */
var transient UILabel TextLabels[3];

/** Which text set we are currently on. */
var transient int CurrentTextSet;

/** Scrolling offset to start from. */
var transient float StartOffset[3];

/** Sets of text used for the scrolling credits. */
var transient array<string>	 TextSets;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );

	/** 
	 * Changes the datastore bindings for widgets to the next image set.
	 *
	 * @param bFront Whether we are updating front or back widgets.
	 */
	void UpdateWidgets(UBOOL bFront);

	/** 
	 * Updates the position and text for credits widgets.
	 */
	void UpdateCreditsText();

	/**
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();
}

/** Sets up the credits scene and retrieves references to widgets. */
native function SetupScene();

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	local int ImageIdx;

	Super.PostInitialize();

	// Retrieve references to all of the portrait images.
	for(ImageIdx=0; ImageIdx<ImageSets.length; ImageIdx++)
	{
		ImageSets[ImageIdx].TexImage = Surface(DynamicLoadObject(ImageSets[ImageIdx].TexImageName, class'Surface'));

		if(ImageSets[ImageIdx].TexImage == None)
		{
			`Log("UTUIFrontEnd_Credits - Couldn't find image "$ImageSets[ImageIdx].TexImageName);
		}
		else
		{
			`Log("UTUIFrontEnd_Credits - DLO Image: "$ImageSets[ImageIdx].TexImageName);
		}
	}

	SetupScene();
}

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
}

/** Callback for when the user wants to back out of this screen. */
function OnBack()
{
	CloseScene(self);
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();

	return true;
}

/**
 * Provides a hook for unrealscript to respond to input using actual input key names (i.e. Left, Tab, etc.)
 *
 * Called when an input key event is received which this widget responds to and is in the correct state to process.  The
 * keys and states widgets receive input for is managed through the UI editor's key binding dialog (F8).
 *
 * This delegate is called BEFORE kismet is given a chance to process the input.
 *
 * @param	EventParms	information about the input event.
 *
 * @return	TRUE to indicate that this input key was processed; no further processing will occur on this input key event.
 */
function bool HandleInputKey( const out InputEventParameters EventParms )
{
	local bool bResult;

	bResult=false;

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}



defaultproperties
{
	SceneTimeInSec = 240;
	ScrollAmount = -10000;
	DelayBeforePictures = 10;
	DelayAfterPictures = 20;

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_01", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG01_01>", "<strings:UTGameCredits.Quotes.IMG01_02>", "<strings:UTGameCredits.Quotes.IMG01_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_01", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG01_04>", "<strings:UTGameCredits.Quotes.IMG01_05>", "<strings:UTGameCredits.Quotes.IMG01_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_01", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG01_07>", "<strings:UTGameCredits.Quotes.IMG01_08>", "<strings:UTGameCredits.Quotes.IMG01_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_01", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG01_10>", "<strings:UTGameCredits.Quotes.IMG01_11>", "<strings:UTGameCredits.Quotes.IMG01_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_02", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG02_01>", "<strings:UTGameCredits.Quotes.IMG02_02>", "<strings:UTGameCredits.Quotes.IMG02_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_02", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG02_04>", "<strings:UTGameCredits.Quotes.IMG02_05>", "<strings:UTGameCredits.Quotes.IMG02_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_02", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG02_07>", "<strings:UTGameCredits.Quotes.IMG02_08>", "<strings:UTGameCredits.Quotes.IMG02_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_02", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG02_10>", "<strings:UTGameCredits.Quotes.IMG02_11>", "<strings:UTGameCredits.Quotes.IMG02_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_03", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG03_01>", "<strings:UTGameCredits.Quotes.IMG03_02>", "<strings:UTGameCredits.Quotes.IMG03_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_03", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG03_04>", "<strings:UTGameCredits.Quotes.IMG03_05>", "<strings:UTGameCredits.Quotes.IMG03_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_03", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG03_07>", "<strings:UTGameCredits.Quotes.IMG03_08>", "<strings:UTGameCredits.Quotes.IMG03_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_03", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG03_10>", "<strings:UTGameCredits.Quotes.IMG03_11>", "<strings:UTGameCredits.Quotes.IMG03_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_04", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG04_01>", "<strings:UTGameCredits.Quotes.IMG04_02>", "<strings:UTGameCredits.Quotes.IMG04_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_04", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG04_04>", "<strings:UTGameCredits.Quotes.IMG04_05>", "<strings:UTGameCredits.Quotes.IMG04_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_04", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG04_07>", "<strings:UTGameCredits.Quotes.IMG04_08>", "<strings:UTGameCredits.Quotes.IMG04_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_04", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG04_10>", "<strings:UTGameCredits.Quotes.IMG04_11>", "<strings:UTGameCredits.Quotes.IMG04_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_05", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG05_01>", "<strings:UTGameCredits.Quotes.IMG05_02>", "<strings:UTGameCredits.Quotes.IMG05_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_05", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG05_04>", "<strings:UTGameCredits.Quotes.IMG05_05>", "<strings:UTGameCredits.Quotes.IMG05_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_05", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG05_07>", "<strings:UTGameCredits.Quotes.IMG05_08>", "<strings:UTGameCredits.Quotes.IMG05_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_05", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG05_10>", "<strings:UTGameCredits.Quotes.IMG05_11>", "<strings:UTGameCredits.Quotes.IMG05_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_06", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG06_01>", "<strings:UTGameCredits.Quotes.IMG06_02>", "<strings:UTGameCredits.Quotes.IMG06_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_06", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG06_04>", "<strings:UTGameCredits.Quotes.IMG06_05>", "<strings:UTGameCredits.Quotes.IMG06_06>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_06", TexCoords=(U=256,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG06_07>", "<strings:UTGameCredits.Quotes.IMG06_08>", "<strings:UTGameCredits.Quotes.IMG06_09>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_06", TexCoords=(U=384,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG06_10>", "<strings:UTGameCredits.Quotes.IMG06_11>", "<strings:UTGameCredits.Quotes.IMG06_12>")))

	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_07", TexCoords=(U=0,V=0,UL=128,VL=512),   LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG07_01>", "<strings:UTGameCredits.Quotes.IMG07_02>", "<strings:UTGameCredits.Quotes.IMG07_03>")))
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_07", TexCoords=(U=128,V=0,UL=128,VL=512), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG07_04>", "<strings:UTGameCredits.Quotes.IMG07_05>", "<strings:UTGameCredits.Quotes.IMG07_06>")))
	
	ImageSets.Add((TexImageName="UI_FrontEnd_Art.Credits.Credits_Port_08", TexCoords=(U=0,V=0,UL=128,VL=128), LabelMarkup=("<Strings:UTGameCredits.Quotes.IMG08_01>")))

	TextSets.Add("<Strings:UTGameCredits.Credits.01>");
	TextSets.Add("<Strings:UTGameCredits.Credits.02>");
	TextSets.Add("<Strings:UTGameCredits.Credits.03>");
	TextSets.Add("<Strings:UTGameCredits.Credits.04>");
	TextSets.Add("<Strings:UTGameCredits.Credits.05>");
	TextSets.Add("<Strings:UTGameCredits.Credits.06>");
	TextSets.Add("<Strings:UTGameCredits.Credits.07>");
	TextSets.Add("<Strings:UTGameCredits.Credits.08>");
	TextSets.Add("<Strings:UTGameCredits.Credits.09>");
	TextSets.Add("<Strings:UTGameCredits.Credits.10>");
	TextSets.Add("<Strings:UTGameCredits.Credits.11>");
	TextSets.Add("<Strings:UTGameCredits.Credits.12>");
	TextSets.Add("<Strings:UTGameCredits.Credits.13>");
	TextSets.Add("<Strings:UTGameCredits.Credits.14>");
	TextSets.Add("<Strings:UTGameCredits.Credits.15>");
	TextSets.Add("<Strings:UTGameCredits.Credits.16>");
	TextSets.Add("<Strings:UTGameCredits.Credits.17>");
	TextSets.Add("<Strings:UTGameCredits.Credits.18>");
}

