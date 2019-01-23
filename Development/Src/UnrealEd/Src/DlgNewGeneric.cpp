/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "Factories.h"
#include "..\..\Launch\Resources\resource.h"
#include "Properties.h"
#include "GenericBrowser.h"

/*-----------------------------------------------------------------------------
	WxDlgNewGeneric.
-----------------------------------------------------------------------------*/
/**
 * Adds the specific font controls to the dialog
 */
void WxDlgNewGeneric::AppendFontControls(void)
{
	FontSizer = new wxBoxSizer(wxHORIZONTAL);
	FactorySizer->Add(FontSizer, 0, wxGROW|wxALL, 5);

	ChooseFontButton = new wxButton( this, ID_CHOOSE_FONT, TEXT("ChooseFont"), wxDefaultPosition, wxDefaultSize, 0 );
	FontSizer->Add(ChooseFontButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	FontStaticText = new wxStaticText( this, wxID_STATIC, TEXT("This is some sample text"), wxDefaultPosition, wxDefaultSize, 0 );
	FontSizer->Add(FontStaticText, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

	FontSizer->Layout();
	FactorySizer->Layout();
	// Add the size to the overall dialog size
	wxSize FontSize = FontSizer->GetSize();
	wxSize OurSize = GetSize();
	SetSize(wxSize(OurSize.GetWidth(),OurSize.GetHeight() + FontSize.GetHeight()));
}

/**
 * Removes the font controls from the dialog
 */
void WxDlgNewGeneric::RemoveFontControls(void)
{
	if (FontSizer != NULL)
	{
		// Shrink the dialog size first
		wxSize FontSize = FontSizer->GetSize();
		wxSize OurSize = GetSize();
		SetSize(wxSize(OurSize.GetWidth(),OurSize.GetHeight() - FontSize.GetHeight()));
		// Now remove us from the dialog
		FactorySizer->Detach(FontSizer);
		delete FontStaticText;
		FontStaticText = NULL;
		delete ChooseFontButton;
		ChooseFontButton = NULL;
		delete FontSizer;
		FontSizer = NULL;
		// Force a layout
		FactorySizer->Layout();
	}
}

/**
 * Lets the user select the font that is going to be imported
 */
void WxDlgNewGeneric::OnChooseFont(wxCommandEvent& In)
{
	wxFontDialog Dialog(this);
	// Show the dialog to let them choose the font
	if (Dialog.ShowModal() == wxID_OK)
	{
		wxFontData RetData = Dialog.GetFontData();
		// Get the font they selected
		wxFont Font = RetData.GetChosenFont();
		// Set the static text to use this font
		FontStaticText->SetFont(Font);
		// Force a visual refresh
		FontSizer->Layout();
		// Now update the factory with the latest values
		UTrueTypeFontFactory* TTFactory = Cast<UTrueTypeFontFactory>(Factory);
		if (TTFactory != NULL)
		{
			TTFactory->FontName = (const TCHAR*)Font.GetFaceName();
			TTFactory->Height = Font.GetPointSize();
			TTFactory->Underline = Font.GetUnderlined();
			TTFactory->Italic = Font.GetStyle() == wxITALIC;
			switch (Font.GetWeight())
			{
				case wxNORMAL:
					TTFactory->Style = 500;
					break;
				case wxLIGHT:
					TTFactory->Style = 300;
					break;
				case wxBOLD:
					TTFactory->Style = 700;
					break;
			}
		}
		// Update the changed properties
		PropertyWindow->SetObject( Factory, 1,1,0 );
		PropertyWindow->Raise();
	}
}

void WxDlgNewGeneric::OnFactorySelChange(wxCommandEvent& In)
{
	UClass* Class = (UClass*)FactoryCombo->GetClientData( FactoryCombo->GetSelection() );

	Factory = ConstructObject<UFactory>( Class );

	PropertyWindow->SetObject( Factory, 1,1,0 );
	PropertyWindow->Raise();
	// Append our font controls if this is a font factory
	if (Factory && Factory->GetClass()->IsChildOf(UFontFactory::StaticClass()))
	{
		AppendFontControls();
	}
	else
	{
		RemoveFontControls();
	}
	Refresh();
}

BEGIN_EVENT_TABLE(WxDlgNewGeneric, wxDialog)
	EVT_COMBOBOX( IDCB_FACTORY, WxDlgNewGeneric::OnFactorySelChange )
	EVT_BUTTON( ID_CHOOSE_FONT, WxDlgNewGeneric::OnChooseFont )
END_EVENT_TABLE()

/**
 * Creates the dialog box for choosing a factory and creating a new resource
 */
WxDlgNewGeneric::WxDlgNewGeneric() :
	wxDialog(NULL,-1,wxString(TEXT("New")))
{
	FontStaticText = NULL;
	ChooseFontButton = NULL;
	FontSizer = NULL;
	// Set our initial size
	SetSize(500,500);
    wxFlexGridSizer* ItemFlexGridSizer2 = new wxFlexGridSizer(1, 2, 0, 0);
    ItemFlexGridSizer2->AddGrowableRow(0);
    ItemFlexGridSizer2->AddGrowableCol(0);
    SetSizer(ItemFlexGridSizer2);

    wxBoxSizer* ItemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    ItemFlexGridSizer2->Add(ItemBoxSizer3, 1, wxGROW|wxGROW|wxALL, 5);

    wxStaticBox* ItemStaticBoxSizer4Static = new wxStaticBox(this, wxID_ANY, TEXT("Info"));
    wxStaticBoxSizer* ItemStaticBoxSizer4 = new wxStaticBoxSizer(ItemStaticBoxSizer4Static, wxHORIZONTAL);
    ItemBoxSizer3->Add(ItemStaticBoxSizer4, 0, wxGROW|wxALL, 5);

	PGNSizer = new wxBoxSizer(wxHORIZONTAL);
	PGNCtrl = new WxPkgGrpNameCtrl( this, ID_PKGGRPNAME, PGNSizer );
	PGNCtrl->SetSizer(PGNSizer);
	PGNCtrl->Show();
	PGNCtrl->SetAutoLayout(true);
    ItemStaticBoxSizer4->Add(PGNCtrl, 1, wxGROW, 5);

    wxStaticBox* ItemStaticBoxSizer6Static = new wxStaticBox(this, wxID_ANY, TEXT("Factory"));
    FactorySizer = new wxStaticBoxSizer(ItemStaticBoxSizer6Static, wxVERTICAL);
    ItemBoxSizer3->Add(FactorySizer, 1, wxGROW|wxALL, 5);

    wxBoxSizer* ItemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    FactorySizer->Add(ItemBoxSizer7, 0, wxGROW|wxALL, 5);

    wxStaticText* ItemStaticText8 = new wxStaticText( this, wxID_STATIC, TEXT("Factory"), wxDefaultPosition, wxDefaultSize, 0 );
    ItemBoxSizer7->Add(ItemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* ItemComboBox9Strings = NULL;
	FactoryCombo = new WxComboBox( this, IDCB_FACTORY, TEXT(""), wxDefaultPosition, wxSize(200, -1), 0, ItemComboBox9Strings, wxCB_READONLY );
    ItemBoxSizer7->Add(FactoryCombo, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* ItemStaticBoxSizer10Static = new wxStaticBox(this, wxID_ANY, TEXT("Options"));
    wxStaticBoxSizer* ItemStaticBoxSizer10 = new wxStaticBoxSizer(ItemStaticBoxSizer10Static, wxVERTICAL);
    FactorySizer->Add(ItemStaticBoxSizer10, 1, wxGROW|wxALL, 5);

	// Insert the property window inside the options
	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( this, GUnrealEd );

    ItemStaticBoxSizer10->Add(PropertyWindow, 1, wxGROW|wxALL, 5);

    wxBoxSizer* ItemBoxSizer15 = new wxBoxSizer(wxVERTICAL);
    ItemFlexGridSizer2->Add(ItemBoxSizer15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* ItemButton16 = new wxButton( this, wxID_OK, TEXT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    ItemButton16->SetDefault();
    ItemBoxSizer15->Add(ItemButton16, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* ItemButton17 = new wxButton( this, wxID_CANCEL, TEXT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    ItemBoxSizer15->Add(ItemButton17, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

	Factory = NULL;

	FLocalizeWindow( this );
}

// DefaultFactoryClass is an optional parameter indicating the default factory to select in combo box
int WxDlgNewGeneric::ShowModal(const FString& InPackage, const FString& InGroup, UClass* DefaultFactoryClass )
{
	Package = InPackage;
	Group = InGroup;

	PGNCtrl->PkgCombo->SetValue( *Package );
	PGNCtrl->GrpEdit->SetValue( *Group );

	// Find the factories that create new objects and add to combo box

	INT i=0;
	INT DefIndex = INDEX_NONE;
	for( TObjectIterator<UClass> It ; It ; ++It )
	{
		if( It->IsChildOf(UFactory::StaticClass()) && !(It->ClassFlags & CLASS_Abstract) )
		{
			UClass* FactoryClass = *It;
			Factory = ConstructObject<UFactory>( FactoryClass );
			UFactory* DefFactory = (UFactory*)FactoryClass->GetDefaultObject();
			if( Factory->bCreateNew )
			{
				FactoryCombo->Append( *DefFactory->Description, FactoryClass );

				// If this is the item we want as the default, remember the index.
				if(FactoryClass && DefaultFactoryClass == FactoryClass)
				{
					DefIndex = i;
				}

				i++;
			}
		}
	}

	Factory = NULL;

	// If no default was specified, we just select the first one.
	if(DefIndex == INDEX_NONE)
	{
		DefIndex = 0;
	}

	FactoryCombo->SetSelection( DefIndex );	

	wxCommandEvent DummyEvent;
	OnFactorySelChange( DummyEvent );

	PGNCtrl->NameEdit->SetFocus();

	return wxDialog::ShowModal();
}

// Same as UnObjBas.h::INVALID_PACKAGENAME_CHARACTERS but with '.' removed.
// We allow '.' because multiple groups could be chained together in the single
// text entry field that the "New" dialog presents the user.
#define DLGNEW_INVALID_GROUP_CHARACTERS	TEXT("\"' ,/")

bool WxDlgNewGeneric::Validate()
{
	Package = PGNCtrl->PkgCombo->GetValue();
	Group	= PGNCtrl->GrpEdit->GetValue();
	Name	= PGNCtrl->NameEdit->GetValue();

	FString	QualifiedName;
	if( Group.Len() )
	{
		QualifiedName = Package + TEXT(".") + Group + TEXT(".") + Name;
	}
	else
	{
		QualifiedName = Package + TEXT(".") + Name;
	}

	FString Reason;
	if( !FIsValidObjectName( *Name, Reason ) || !FIsValidObjectName( *Package, Reason ) || !FIsValidXName( *Group, Reason, DLGNEW_INVALID_GROUP_CHARACTERS ) || !FIsUniqueObjectName(*QualifiedName, ANY_PACKAGE, Reason) )
	{
		appMsgf( AMT_OK, *Reason );
		return 0;
	}

	UPackage* Pkg = UObject::CreatePackage(NULL,*Package);
	if( Group.Len() )
	{
		Pkg = GEngine->CreatePackage(Pkg,*Group);
	}
	UPackage* OutermostPkg = Pkg->GetOutermost();


	// Handle fully loading packages before creating new objects.
	WxGenericBrowser* GenericBrowser = GUnrealEd->GetBrowser<WxGenericBrowser>( TEXT("GenericBrowser") );
	check(GenericBrowser);
	TArray<UPackage*> TopLevelPackages;
	TopLevelPackages.AddItem( OutermostPkg );
	if( !GenericBrowser->HandleFullyLoadingPackages( TopLevelPackages, TEXT("CreateANewObject") ) )
	{
		// User aborted.
		return 0;
	}

	// We need to test again after fully loading.
	if( !FIsValidObjectName( *Name, Reason ) || !FIsValidObjectName( *Package, Reason ) || !FIsValidXName( *Group, Reason, DLGNEW_INVALID_GROUP_CHARACTERS ) || !FIsUniqueObjectName(*QualifiedName, ANY_PACKAGE, Reason) )
	{
		appMsgf( AMT_OK, *Reason );
		return 0;
	}

	OutermostPkg->SetDirtyFlag(TRUE);

	UObject* NewObj = Factory->FactoryCreateNew( Factory->GetSupportedClass(), Pkg, FName( *Name ), RF_Public|RF_Standalone, NULL, GWarn );
	if( NewObj )
	{
		// Set the new objects as the sole selection.
		USelection* SelectionSet = GEditor->GetSelectedObjects();
		SelectionSet->DeselectAll();
		SelectionSet->Select( NewObj );
	}

	return 1;
}

/**
 * Since we create a UFactory object, it needs to be serialized
 *
 * @param Ar The archive to serialize with
 */
void WxDlgNewGeneric::Serialize(FArchive& Ar)
{
	Ar << Factory;
}
