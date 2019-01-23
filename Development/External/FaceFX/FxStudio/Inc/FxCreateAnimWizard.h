//------------------------------------------------------------------------------
// A wizard for creating new animations.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxCreateAnimWizard_H__
#define FxCreateAnimWizard_H__

#include "FxPlatform.h"
#include "FxArray.h"

#ifdef __UNREAL__
	#include "XWindow.h"
#else
	#include "wx/wizard.h"
#endif

namespace OC3Ent
{
namespace Face
{

// Runs the animation creation wizard.  Returns true on success, or false on failure.
FxBool FxRunCreateAnimWizard(wxWindow* parent, const FxString& destGroup);

//------------------------------------------------------------------------------
// Type Selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectType : public wxWizardPage
{
public:
	enum FxAnimType
	{
		AT_AnalysisBased = 0,
		AT_NonAnalysisBased
	};

	FxAnimWizardPageSelectType(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next);

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxAnimType GetAnimType() const;
	FxBool GetImportAudio() const;

protected:
	DECLARE_EVENT_TABLE()

	void OnAnimTypeChange(wxCommandEvent&);
	
	wxRadioBox*		_animTypeRadio;
	wxCheckBox*		_importAudioCheck;
	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;
};

//------------------------------------------------------------------------------
// Audio Selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectAudio : public wxWizardPage
{
public:
	FxAnimWizardPageSelectAudio(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, FxAnimWizardPageSelectType* typeSelectPage);

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxString GetAudioFilename() const;

protected:
	DECLARE_EVENT_TABLE()

	void OnBrowseButton(wxCommandEvent&);
	void OnPageChanged(wxWizardEvent&);
	void OnPageChanging(wxWizardEvent&);
	
	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;
	FxAnimWizardPageSelectType* _typeSelectPage;

	wxString _audioFilename;
};

#ifdef __UNREAL__

//------------------------------------------------------------------------------
// USoundNodeWave selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectSoundNodeWave : public wxWizardPage
{
public:
	FxAnimWizardPageSelectSoundNodeWave(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, FxAnimWizardPageSelectType* typeSelectPage);

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxString GetSelectedSoundNodeWave() const;

protected:
	DECLARE_EVENT_TABLE()

	void OnPageChanging(wxWizardEvent& event);
	void OnPageChanged(wxWizardEvent&);

	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;
	FxAnimWizardPageSelectType* _typeSelectPage;

	wxListBox* _soundNodeWaveListBox;

	wxString _selectedSoundNodeWave;
};

#endif

//------------------------------------------------------------------------------
// Text selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectText : public wxWizardPage
{
public:
	FxAnimWizardPageSelectText(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next);

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxWString GetAnalysisText() const;

protected:
	DECLARE_EVENT_TABLE()

	void OnBrowseButton(wxCommandEvent&);
	void OnPageChanging(wxWizardEvent&);
	void OnPageChanged(wxWizardEvent&);

	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;

	wxString		_analysisText;
};

//------------------------------------------------------------------------------
// Options selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectOptions : public wxWizardPage
{
public:
	FxAnimWizardPageSelectOptions(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next);

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxString GetLanguage();
	FxString GetGestureConfig();
	FxString GetCoarticulationConfig();

protected:
	DECLARE_EVENT_TABLE()

	void OnLanguageChoiceChange(wxCommandEvent&);
	void OnSelectConfigButtons(wxCommandEvent&);

	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;
	wxChoice*		_languageChoice;
	wxStaticText*	_gestureConfigLabel;
	wxStaticText*	_coarticulationConfigLabel;

	FxString		_language;
	FxString		_gestureConfig;
	FxString		_coarticulationConfig;

	FxString		_defaultLanguage;
};

//------------------------------------------------------------------------------
// Name selection
//------------------------------------------------------------------------------

class FxAnimWizardPageSelectName : public wxWizardPage
{
public:
	FxAnimWizardPageSelectName(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, FxAnimWizardPageSelectType* typeSelectPage, const FxString& animGroup );

	void SetPrev(wxWizardPage* prev);
	void SetNext(wxWizardPage* next);
	virtual wxWizardPage* GetPrev() const;
	virtual wxWizardPage* GetNext() const;

	FxString GetAnimName() const;

protected:
	DECLARE_EVENT_TABLE()

	void OnPageChanging(wxWizardEvent&);
	void OnPageChanged(wxWizardEvent&);

	wxWizardPage*	_prevPage;
	wxWizardPage*	_nextPage;
	FxAnimWizardPageSelectType* _typeSelectPage;

	FxString		_animGroup;
	wxString		_animName;
};

} // namespace Face
} // namespace OC3Ent

#endif