class UTUIFrontEnd_WeaponReplacementMenu extends UTUIFrontEnd
	dependson(UTMutator_WeaponReplacement);

var transient UIList WeaponList;
var transient UTUIComboBox ReplacementCombo;

var transient array<ReplacementInfo> WeaponsToReplace;
var transient array<ReplacementInfo> AmmoToReplace;

event SceneActivated(bool bInitialActivation)
{
	local int i, NumItems;
	local string ClassPath;

	Super.SceneActivated(bInitialActivation);

	if (bInitialActivation)
	{
		WeaponList = UIList(FindChild('lstWeapons', true));
		WeaponList.OnValueChanged = OnWeaponList_ValueChanged;

		ReplacementCombo = UTUIComboBox(FindChild('cmbReplacement', true));
		ReplacementCombo.OnValueChanged = OnReplacementCombo_ValueChanged;

		// load current options
		WeaponsToReplace = class'UTMutator_WeaponReplacement'.default.WeaponsToReplace;
		AmmoToReplace = class'UTMutator_WeaponReplacement'.default.AmmoToReplace;
	}

	// on console, we can't swap things in UTGameContent, so remove them from the list
	if (class'WorldInfo'.static.IsConsoleBuild())
	{
		NumItems = WeaponList.GetItemCount();
		for (i = 0; i < NumItems; i++)
		{
			if ( class'UTUIMenuList'.static.GetCellFieldString(WeaponList, 'ClassName', i, ClassPath) &&
				Left(ClassPath, 14) ~= "UTGameContent." )
			{
				WeaponList.RemoveElement(i);
				ReplacementCombo.ComboList.RemoveElement(i);
			}
		}
	}

	// update replacement combo for starting list item
	OnWeaponList_ValueChanged(None, 0);
}

function OnWeaponList_ValueChanged(UIObject Sender, int PlayerIndex)
{
	local string OldClassPath, ReplacementClassPath;
	local int SelectedItem, ReplacementIndex;
	local name WeaponClassName;

	SelectedItem = WeaponList.GetCurrentItem();

	if (class'UTUIMenuList'.static.GetCellFieldString(WeaponList, 'ClassName', SelectedItem, OldClassPath))
	{
		WeaponClassName = name(Right(OldClassPath, Len(OldClassPath) - InStr(OldClassPath, ".") - 1));
		ReplacementIndex = WeaponsToReplace.Find('OldClassName', WeaponClassName);
		ReplacementClassPath = (ReplacementIndex != INDEX_NONE) ? WeaponsToReplace[ReplacementIndex].NewClassPath : OldClassPath;
		// temporarily disable the delegate so we don't try to write a new replacement while we're in the middle of looking one up
		ReplacementCombo.OnValueChanged = None;
		ReplacementCombo.ComboList.SetIndex(class'UTUIMenuList'.static.FindCellFieldString(WeaponList, 'ClassName', ReplacementClassPath));
		ReplacementCombo.OnValueChanged = OnReplacementCombo_ValueChanged;
	}
}

function OnReplacementCombo_ValueChanged(UIObject Sender, int PlayerIndex)
{
	local int WeaponListIndex, ReplacementComboIndex, ReplacementIndex;
	local string OldClassPath, NewClassPath;
	local name WeaponClassName, AmmoClassName;

	WeaponListIndex = WeaponList.GetCurrentItem();

	if (class'UTUIMenuList'.static.GetCellFieldString(WeaponList, 'ClassName', WeaponListIndex, OldClassPath))
	{
		ReplacementComboIndex = ReplacementCombo.ComboList.GetCurrentItem();
		if (class'UTUIMenuList'.static.GetCellFieldString(ReplacementCombo.ComboList, 'ClassName', ReplacementComboIndex, NewClassPath))
		{
			// add weapon replacement info to array
			WeaponClassName = name(Right(OldClassPath, Len(OldClassPath) - InStr(OldClassPath, ".") - 1));
			ReplacementIndex = WeaponsToReplace.Find('OldClassName', WeaponClassName);
			if (ReplacementIndex == INDEX_NONE)
			{
				ReplacementIndex = WeaponsToReplace.length;
				WeaponsToReplace.length = WeaponsToReplace.length + 1;
				WeaponsToReplace[ReplacementIndex].OldClassName = WeaponClassName;
			}
			WeaponsToReplace[ReplacementIndex].NewClassPath = NewClassPath;
			// now add ammo info
			if (class'UTUIMenuList'.static.GetCellFieldString(WeaponList, 'AmmoClassPath', WeaponListIndex, OldClassPath) && OldClassPath != "")
			{
				AmmoClassName = name(Right(OldClassPath, Len(OldClassPath) - InStr(OldClassPath, ".") - 1));
				ReplacementIndex = AmmoToReplace.Find('OldClassName', AmmoClassName);
				if (ReplacementIndex == INDEX_NONE)
				{
					ReplacementIndex = AmmoToReplace.length;
					AmmoToReplace.length = AmmoToReplace.length + 1;
					AmmoToReplace[ReplacementIndex].OldClassName = AmmoClassName;
				}
				if (class'UTUIMenuList'.static.GetCellFieldString(ReplacementCombo.ComboList, 'AmmoClassPath', ReplacementComboIndex, NewClassPath))
				{
					AmmoToReplace[ReplacementIndex].NewClassPath = NewClassPath;
				}
				else
				{
					// new weapon has no ammo class
					AmmoToReplace[ReplacementIndex].NewClassPath = "";
				}
			}
		}
	}
}

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
}

/** Callback for when the user wants to back out of this screen. */
function OnBack()
{
	class'UTMutator_WeaponReplacement'.default.WeaponsToReplace = WeaponsToReplace;
	class'UTMutator_WeaponReplacement'.default.AmmoToReplace = AmmoToReplace;
	class'UTMutator_WeaponReplacement'.static.StaticSaveConfig();

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

