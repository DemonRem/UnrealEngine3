//------------------------------------------------------------------------------
// Extended information about the phonemes.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxArray.h"
#include "FxPhonemeInfo.h"

namespace OC3Ent
{

namespace Face
{

#define MAKE_PHONEINFO( phon, cls, tb, sampa, ipa, sample ) \
	FxPhonExtendedInfo phon##Info((phon), (cls), (tb), (sampa), (ipa), (sample));\
	phonInfo.PushBack( phon##Info );

// Gets information about a phoneme
const FxPhonExtendedInfo& FxGetPhonemeInfo( const FxPhoneme& phon )
{
	FxAssert( PHONEME_FIRST <= phon && phon <= PHONEME_LAST );
	static FxArray<FxPhonExtendedInfo> phonInfo;
	if( phonInfo.Length() == 0 )
	{
		MAKE_PHONEINFO( PHONEME_IY, PHONCLASS_VOWEL,	  wxT("IY"), wxT("i:"), wxT("i"),                                    wxT("trEE") );
		MAKE_PHONEINFO( PHONEME_IH, PHONCLASS_VOWEL,	  wxT("IH"), wxT("I"),  wxString((wxChar)618),                       wxT("Insect") );
		MAKE_PHONEINFO( PHONEME_EH, PHONCLASS_VOWEL,	  wxT("EH"), wxT("E"),  wxString((wxChar)603),                       wxT("bEt") );
		MAKE_PHONEINFO( PHONEME_EY, PHONCLASS_DIPHTHONG,  wxT("EY"), wxT("eI"), wxString((wxChar)603)+wxString((wxChar)618), wxT("plAY") );
		MAKE_PHONEINFO( PHONEME_AE, PHONCLASS_VOWEL,	  wxT("AE"), wxT("{"),  wxString((wxChar)230),                       wxT("bAt") );
		MAKE_PHONEINFO( PHONEME_AA, PHONCLASS_VOWEL,	  wxT("AA"), wxT("A"),  wxString((wxChar)593),                       wxT("Arm") );
		MAKE_PHONEINFO( PHONEME_AW, PHONCLASS_DIPHTHONG,  wxT("AW"), wxT("AU"), wxString(wxT('a'))+wxString((wxChar)650),    wxT("abOUt") );
		MAKE_PHONEINFO( PHONEME_AY, PHONCLASS_DIPHTHONG,  wxT("AY"), wxT("aI"), wxString(wxT('a'))+wxString((wxChar)618),    wxT("bIte") );
		MAKE_PHONEINFO( PHONEME_AH, PHONCLASS_VOWEL,	  wxT("AH"), wxT("V"),  wxString((wxChar)652),                       wxT("rUn") );
		MAKE_PHONEINFO( PHONEME_AO, PHONCLASS_VOWEL,	  wxT("AO"), wxT("O"),  wxString((wxChar)596),                       wxT("cAUght") );
		MAKE_PHONEINFO( PHONEME_OY, PHONCLASS_DIPHTHONG,  wxT("OY"), wxT("OI"), wxString((wxChar)596)+wxString((wxChar)618), wxT("bOY") );
		MAKE_PHONEINFO( PHONEME_OW, PHONCLASS_DIPHTHONG,  wxT("OW"), wxT("@U"), wxString((wxChar)601)+wxString((wxChar)650), wxT("bOAt") );
		MAKE_PHONEINFO( PHONEME_UH, PHONCLASS_VOWEL,	  wxT("UH"), wxT("U"),  wxString((wxChar)650),       				 wxT("pUt") );
		MAKE_PHONEINFO( PHONEME_UW, PHONCLASS_VOWEL,	  wxT("UW"), wxT("u:"), wxString(wxT('u')),          				 wxT("sOOn") );
		MAKE_PHONEINFO( PHONEME_ER, PHONCLASS_VOWEL,	  wxT("ER"), wxT("3"),  wxString((wxChar)604),       				 wxT("bIRd") );
		MAKE_PHONEINFO( PHONEME_AX, PHONCLASS_VOWEL,	  wxT("AX"), wxT("@"),  wxString((wxChar)601),       				 wxT("About") );
		MAKE_PHONEINFO( PHONEME_S,  PHONCLASS_FRICATIVE,  wxT("S"),  wxT("s"),  wxString(wxT('s')),          				 wxT("Sea") );
		MAKE_PHONEINFO( PHONEME_SH, PHONCLASS_FRICATIVE,  wxT("SH"), wxT("S"),  wxString((wxChar)643),       				 wxT("SHe") );
		MAKE_PHONEINFO( PHONEME_Z,  PHONCLASS_FRICATIVE,  wxT("Z"),  wxT("z"),  wxString(wxT('z')),          				 wxT("Zone") );
		MAKE_PHONEINFO( PHONEME_ZH, PHONCLASS_FRICATIVE,  wxT("ZH"), wxT("Z"),  wxString((wxChar)658),       				 wxT("pleaSure") );
		MAKE_PHONEINFO( PHONEME_F,  PHONCLASS_FRICATIVE,  wxT("F"),  wxT("f"),  wxString((wxChar)wxT('f')),  				 wxT("Fat") );
		MAKE_PHONEINFO( PHONEME_TH, PHONCLASS_FRICATIVE,  wxT("TH"), wxT("T"),  wxString((wxChar)952),       				 wxT("THin") );
		MAKE_PHONEINFO( PHONEME_V,  PHONCLASS_FRICATIVE,  wxT("V"),  wxT("v"),  wxString((wxChar)wxT('v')),  				 wxT("Van") );
		MAKE_PHONEINFO( PHONEME_DH, PHONCLASS_FRICATIVE,  wxT("DH"), wxT("D"),  wxString((wxChar)240),       				 wxT("THen") );
		MAKE_PHONEINFO( PHONEME_M,  PHONCLASS_NASAL,	  wxT("M"),  wxT("m"),  wxString((wxChar)wxT('m')),  				 wxT("Man") );
		MAKE_PHONEINFO( PHONEME_N,  PHONCLASS_NASAL,	  wxT("N"),  wxT("n"),  wxString((wxChar)wxT('n')),  				 wxT("No") );
		MAKE_PHONEINFO( PHONEME_NG, PHONCLASS_NASAL,	  wxT("NG"), wxT("N"),  wxString((wxChar)331),       				 wxT("siNG") );
		MAKE_PHONEINFO( PHONEME_L,  PHONCLASS_APPROXIMATE,wxT("L"),  wxT("l"),  wxString((wxChar)wxT('l')),  				 wxT("Law") );
		MAKE_PHONEINFO( PHONEME_R,  PHONCLASS_APPROXIMATE,wxT("R"),  wxT("r"),  wxString((wxChar)635),       				 wxT("Red") );
		MAKE_PHONEINFO( PHONEME_W,  PHONCLASS_APPROXIMATE,wxT("W"),  wxT("w"),  wxString((wxChar)wxT('w')),  				 wxT("War") );
		MAKE_PHONEINFO( PHONEME_Y,  PHONCLASS_APPROXIMATE,wxT("Y"),  wxT("j"),  wxString((wxChar)wxT('j')),  				 wxT("Yes") );
		MAKE_PHONEINFO( PHONEME_HH, PHONCLASS_FRICATIVE,  wxT("HH"), wxT("h"),  wxString((wxChar)wxT('h')),  				 wxT("Hay") );
		MAKE_PHONEINFO( PHONEME_B,  PHONCLASS_STOP,		  wxT("B"),  wxT("b"),  wxString((wxChar)wxT('b')),  				 wxT("Bee") );
		MAKE_PHONEINFO( PHONEME_D,  PHONCLASS_STOP,		  wxT("D"),  wxT("d"),  wxString((wxChar)wxT('d')),  				 wxT("Day") );
		MAKE_PHONEINFO( PHONEME_JH, PHONCLASS_STOP,       wxT("JH"), wxT("dZ"), wxString((wxChar)676),       				 wxT("JuDGe") );
		MAKE_PHONEINFO( PHONEME_G,  PHONCLASS_STOP,		  wxT("G"),  wxT("g"),  wxString((wxChar)wxT('g')),  				 wxT("Game") );
		MAKE_PHONEINFO( PHONEME_P,  PHONCLASS_STOP,		  wxT("P"),  wxT("p"),  wxString((wxChar)wxT('p')),  				 wxT("Pet") );
		MAKE_PHONEINFO( PHONEME_T,  PHONCLASS_STOP,		  wxT("T"),  wxT("t"),  wxString((wxChar)wxT('t')),  				 wxT("TesT") );
		MAKE_PHONEINFO( PHONEME_K,  PHONCLASS_STOP,		  wxT("K"),  wxT("k"),  wxString((wxChar)wxT('k')),  				 wxT("Key") );
		MAKE_PHONEINFO( PHONEME_CH, PHONCLASS_STOP,       wxT("CH"), wxT("tS"), wxString((wxChar)679),       				 wxT("Chance") );
		MAKE_PHONEINFO( PHONEME_SIL,PHONCLASS_SILENCE,    wxT("SIL"),wxT("sil"),wxT("sil"),                  				 wxT("(silence)") );
		MAKE_PHONEINFO( PHONEME_SHORTSIL,PHONCLASS_SILENCE,wxT("SHORTSIL"),wxT("shortsil"), wxT("shortsil"), 				 wxT("(silence)") );
		MAKE_PHONEINFO( PHONEME_FLAP,PHONCLASS_STOP,       wxT("FLAP"),    wxT("flap"),     wxT("flap"),     				 wxT("waTer") );
	}
	return phonInfo[phon];
}

const FxPhonExtendedInfo& FxGetPhonemeInfo( FxSize phonIndex )
{
	return FxGetPhonemeInfo( static_cast<FxPhoneme>(phonIndex) );
}

} // namespace Face

} // namespace OC3Ent
