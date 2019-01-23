//------------------------------------------------------------------------------
// A text tag parser for UNICODE strings.
// 
// This parser changes the format from HTML to a simpler format.
// The format of the tag is as such:
// [curvename (type={quad|lt|ct||tt}) (duration=real) (timeshift=real)
//		(easein=real) (easeout=real) (v1=real) (v2=real) (v3=real) (v4=real)]
// To close a tag, simply use:
// [/curvename]
//
// Note: curvename must be quoted if it contains spaces.
// 
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxUnicodeTextTagParser.h"
#include "FxPhonWordList.h"

namespace OC3Ent
{

namespace Face
{

//------------------------------------------------------------------------------
// FxUnicodeTextTagInfo
//------------------------------------------------------------------------------

static const FxWChar FxTagOpen  = L'[';
static const FxWChar FxTagClose = L']';

// Default constructor.
FxUnicodeTextTagInfo::FxUnicodeTextTagInfo()
	: _originalTag(wxEmptyString)
	, _preprocessedTag(wxEmptyString)
	, _tagError(TE_None)
	, _curveName("")
	, _curveType(CT_Quadruplet)
	, _duration(FxInvalidValue)
	, _timeShift(0.0f)
	, _easeIn(0.2f)
	, _easeOut(0.2f)
	, _v1(0.0f)
	, _v2(1.0f)
	, _v3(1.0f)
	, _v4(0.0f)
	, _startWord(FxInvalidIndex)
	, _endWord(FxInvalidIndex)
	, _isOpeningTag(FxTrue)
{
}

// Constructs from a tag.
FxUnicodeTextTagInfo::FxUnicodeTextTagInfo(const FxWString& tag)
	: _originalTag(wxEmptyString)
	, _preprocessedTag(wxEmptyString)
	, _tagError(TE_None)
	, _curveName("")
	, _curveType(CT_Quadruplet)
	, _duration(FxInvalidValue)
	, _timeShift(0.0f)
	, _easeIn(0.2f)
	, _easeOut(0.2f)
	, _v1(0.0f)
	, _v2(1.0f)
	, _v3(1.0f)
	, _v4(0.0f)
	, _startWord(FxInvalidIndex)
	, _endWord(FxInvalidIndex)
	, _isOpeningTag(FxTrue)
{
	InitializeTag(tag);
}

// Destructor.
FxUnicodeTextTagInfo::~FxUnicodeTextTagInfo()
{
}

// Initializes the tag info from a tag.
void FxUnicodeTextTagInfo::InitializeTag(const FxWString& tag)
{
	_originalTag = tag;

	// Remove the beginning and ending brackets.
	FxWString tagBody = tag.StripWhitespace();
	// Make sure the beginning and end of the tag are brackets.
	if( tagBody.Length() > 2 )
	{
		if( tagBody.GetChar(0) == FxTagOpen && tagBody.GetChar(tagBody.Length()-1) == FxTagClose )
		{
			tagBody = tagBody.Substring(1, tagBody.Length() - 2);
		}
		else
		{
			_tagError = TE_MalformedTag;
			//FxWarn("Text Tagging: Malformed tag: %s", _originalTag.mb_str(wxConvLibc));
		}
	}
	else
	{
		_tagError = TE_MalformedTag;
		//FxWarn("Text Tagging: Malformed tag: %s", _originalTag.mb_str(wxConvLibc));
	}

	if( _tagError == TE_None )
	{
		PreprocessTag(tagBody);
	}
}

// Returns the error state of the tag.
FxUnicodeTextTagInfo::FxTagError FxUnicodeTextTagInfo::GetError() const
{
	return _tagError;
}

const FxString& FxUnicodeTextTagInfo::GetCurveName( void ) const
{
	return _curveName;
}

void FxUnicodeTextTagInfo::SetCurveName( const FxString& curveName )
{
	_curveName = curveName;
}

FxUnicodeTextTagInfo::FxCurveType FxUnicodeTextTagInfo::GetCurveType( void ) const
{
	return _curveType;
}

void FxUnicodeTextTagInfo::SetCurveType( FxUnicodeTextTagInfo::FxCurveType curveType )
{
	_curveType = curveType;
}

FxReal FxUnicodeTextTagInfo::GetDuration( void ) const
{
	return _duration;
}

void FxUnicodeTextTagInfo::SetDuration( FxReal duration )
{
	_duration = duration;
}

FxReal FxUnicodeTextTagInfo::GetTimeShift( void ) const
{
	return _timeShift;
}

void FxUnicodeTextTagInfo::SetTimeShift( FxReal timeShift )
{
	_timeShift = timeShift;
}

FxReal FxUnicodeTextTagInfo::GetEaseIn( void ) const
{
	return _easeIn;
}

void FxUnicodeTextTagInfo::SetEaseIn( FxReal easeIn )
{
	_easeIn = easeIn;
}

FxReal FxUnicodeTextTagInfo::GetEaseOut( void ) const
{
	return _easeOut;
}

void FxUnicodeTextTagInfo::SetEaseOut( FxReal easeOut )
{
	_easeOut = easeOut;
}

FxReal FxUnicodeTextTagInfo::GetV1( void ) const
{
	return _v1;
}

void FxUnicodeTextTagInfo::SetV1( FxReal v1 )
{
	_v1 = v1;
}

FxReal FxUnicodeTextTagInfo::GetV2( void ) const
{
	return _v2;
}

void FxUnicodeTextTagInfo::SetV2( FxReal v2 )
{
	_v2 = v2;
}

FxReal FxUnicodeTextTagInfo::GetV3( void ) const
{
	return _v3;
}

void FxUnicodeTextTagInfo::SetV3( FxReal v3 )
{
	_v3 = v3;
}

FxReal FxUnicodeTextTagInfo::GetV4( void ) const
{
	return _v4;
}

void FxUnicodeTextTagInfo::SetV4( FxReal v4 )
{
	_v4 = v4;
}

FxSize FxUnicodeTextTagInfo::GetStartWord( void ) const
{
	return _startWord;
}

void FxUnicodeTextTagInfo::SetStartWord( FxSize startWord )
{
	_startWord = startWord;
}

FxSize FxUnicodeTextTagInfo::GetEndWord( void ) const
{
	return _endWord;
}

void FxUnicodeTextTagInfo::SetEndWord( FxSize endWord )
{
	_endWord = endWord;
}

// Returns the original tag.
const FxWString& FxUnicodeTextTagInfo::GetOriginalTag() const
{
	return _originalTag;
}

// Returns true if this is an opening tag.
FxBool FxUnicodeTextTagInfo::IsOpeningTag() const
{
	return _isOpeningTag;
}

// Preprocesses the tag, removing unneeded spaces.
void FxUnicodeTextTagInfo::PreprocessTag(const FxWString& tagBody)
{
	_preprocessedTag = wxEmptyString;

	// Extract and strip the curve name.
	FxWString namelessTagBody = ExtractCurveName(tagBody.StripWhitespace());

	// Remove any leading/trailing whitespace, then tokenize the text.
	FxArray<FxWString> tokenArray = Tokenize(namelessTagBody.StripWhitespace());

	// Now concat together all the tokens into the preprocessed string.
	for( FxSize i = 0; i < tokenArray.Length(); ++i )
	{
		_preprocessedTag += tokenArray[i];
		if( i < tokenArray.Length()-1 && tokenArray[i+1].Length() > 0 && tokenArray[i+1].GetChar(0) == wxT('=') ||
			tokenArray[i].Length() > 0 && tokenArray[i].GetChar(tokenArray[i].Length()-1) == wxT('=') )
		{
			// If the next token begins with an '=' or the current token ends with a '=', do not insert a space.
			// Intentionally blank.  It was easier for me to think it through this way.
		}
		else
		{
			// otherwise, insert a space between the tokens.
			_preprocessedTag += wxT(" ");
		}
	}

	if( _tagError == TE_None )
	{
		ProcessTag(_preprocessedTag);
	}
}

// Extracts the curve name from a tag body.  Returns the body minus the name.
FxWString FxUnicodeTextTagInfo::ExtractCurveName(const FxWString& tagBody)
{
	FxWString retval;
	FxWString curveName;
	FxWString temp = tagBody;
	// Make sure something is in the string.
	if( tagBody.Length() > 0 )
	{
		// If the tag begins with a '/', it is a closing tag.  Mark it as such,
		// then strip the '/' so name extraction can proceed.
		if( temp.GetChar(0) == wxT('/') )
		{
			_isOpeningTag = FxFalse;
			temp = temp.AfterFirst(wxT('/')).StripWhitespace();
		}

		if( temp.GetChar(0) == wxT('"') )
		{
			// If the first character is a quote, we need to do quoted extraction.
			FxSize secondQuotePos = temp.FindFirst(L'"', 1);
			if( secondQuotePos == FxInvalidIndex )
			{
				_tagError = TE_MalformedTag;
				//FxWarn("Text Tagging: Malformed tag: %s", _originalTag.mb_str(wxConvLibc));
				_curveName = FxString(temp.GetData());
				return wxEmptyString;
			}
			curveName = temp.AfterFirst(wxT('"')).BeforeFirst(wxT('"'));
			retval = temp.Substring(secondQuotePos+1);
		}
		else
		{
			// Otherwise, pull out the name until the first space.
			curveName = temp.BeforeFirst(wxT(' '));
			retval = temp.AfterFirst(wxT(' '));
		}
	}

	// Record the curve name, and return the tag with the curve name removed.
	// That is, only the parameters should be present at this point.
	_curveName = FxString(curveName.GetData());
	if( 0 == _curveName.Length() )
	{
		_tagError = TE_MalformedTag;
		//FxWarn("Text Tagging: Empty curve name not allowed: %s", _originalTag.mb_str(wxConvLibc));
	}
	return retval;
}

// Process the tag, extracting the name and any parameters.
void FxUnicodeTextTagInfo::ProcessTag(const FxWString& tag)
{
	// The string should now be in the format
	// name=value name=value name=value.
	// Tokenize it, and process each name=value pair.
	FxArray<FxWString> tokens = Tokenize(tag);
	for( FxSize i = 0; i < tokens.Length(); ++i )
	{
		ProcessParameter(tokens[i]);
	}
}

// Handles a single tag parameter
void FxUnicodeTextTagInfo::ProcessParameter(const FxWString& parameter)
{
	// Split the parameter into the name and value.
	FxWString name(parameter.BeforeFirst(L'=').ToUpper());
	FxWString value(parameter.AfterFirst(L'=').ToUpper());

	// Try to convert the value to a real.
	FxBool goodValue = FxTrue; // Eh, this used to work, when we used wxString.
	FxReal valueAsReal = FxAtof(value.GetCstr());

	// Check if we know what to do with the name.
	if( name == wxT("TYPE") )
	{
		if( value == wxT("QUAD") )
		{
			_curveType = CT_Quadruplet;
		}
		else if( value == wxT("LT") )
		{
			_curveType = CT_LeadingTriplet;
			_v3 = 0.0f;
		}
		else if( value == wxT("CT") )
		{
			_curveType = CT_CenterTriplet;
			_v3 = 0.0f;
		}
		else if( value == wxT("TT") )
		{
			_curveType = CT_TrailingTriplet;
			_v3 = 0.0f;
		}
		else
		{
			_tagError = TE_UnknownParameter;
			//FxWarn("Text Tagging: Unknown curve type: %s. Parameter ignored.", value.mb_str(wxConvLibc));
		}
	}
	else if( name == wxT("DURATION") && goodValue )
	{
		_duration = valueAsReal;
	}
	else if( name == wxT("TIMESHIFT") && goodValue )
	{
		_timeShift = valueAsReal;
	}
	else if( name == wxT("EASEIN") && goodValue )
	{
		_easeIn = valueAsReal;
	}
	else if( name == wxT("EASEOUT") && goodValue )
	{
		_easeOut = valueAsReal;
	}
	else if( name == wxT("V1") && goodValue )
	{
		_v1 = valueAsReal;
	}
	else if( name == wxT("V2") && goodValue )
	{
		_v2 = valueAsReal;
	}
	else if( name == wxT("V3") && goodValue )
	{
		_v3 = valueAsReal;
	}
	else if( name == wxT("V4") && goodValue )
	{
		_v4 = valueAsReal;
	}
	else
	{
		_tagError = TE_UnknownParameter;
		//FxWarn("Text Tagging: Unknown parameter: %s. Parameter ignored.", name.mb_str(wxConvLibc));
	}
}

FxArray<FxWString> FxUnicodeTextTagInfo::Tokenize(FxWString str)
{
	FxArray<FxWString> retval;
	str = str.StripWhitespace();
	while( str.Length() )
	{
		// Find the next whitespace.
		FxSize nextWhitespace = str.FindFirst(FxWString::traits_type::SpaceCharacter);
		nextWhitespace = FxMin(nextWhitespace, str.FindFirst(FxWString::traits_type::TabCharacter));
		nextWhitespace = FxMin(nextWhitespace, str.FindFirst(FxWString::traits_type::ReturnCharacter));
		nextWhitespace = FxMin(nextWhitespace, str.FindFirst(FxWString::traits_type::NewlineCharacter));
		retval.PushBack(str.Substring(0, nextWhitespace));
		if( nextWhitespace != FxInvalidIndex )
		{
			str = str.Substring(nextWhitespace).StripWhitespace();
		}
		else
		{
			str.Clear();
		}
	}
	return retval;
}

//------------------------------------------------------------------------------
// FxUnicodeTextTagParser
//------------------------------------------------------------------------------

static const FxWChar FxTagDelimiter  = static_cast<FxWChar>(179);
static const FxWChar FxWordDelimiter = static_cast<FxWChar>(180);

// Constructor.
FxUnicodeTextTagParser::FxUnicodeTextTagParser()
	: _originalText(wxEmptyString)
	, _error(TTE_None)
{
}

// Constructs from a tagged analysis text.
FxUnicodeTextTagParser::FxUnicodeTextTagParser(const FxWString& analysisText)
	: _originalText(wxEmptyString)
	, _error(TTE_None)
{
	InitializeParser(analysisText);
}

// Destructor.
FxUnicodeTextTagParser::~FxUnicodeTextTagParser()
{
}

// Initializes the parser with a tagged analysis text.
FxBool FxUnicodeTextTagParser::InitializeParser(const FxWString& analysisText)
{
	_originalText = analysisText;
	StripPunctuation(_originalText);
	return ReplaceTextTagsWithMarkers(_noPuncText);
}

// Parses according to the words in the word list.
void FxUnicodeTextTagParser::Parse(const FxPhonWordList* pWordList)
{
	ReplaceWordsWithMarkers(_stageOne, pWordList);
	StripExtraneousCharacters(_stageTwo);

	// Now, set the word indices in the text tags.
	for( FxSize i = 0; i < _tagInfos.Length(); ++i )
	{
		// Find the start word marker
		FxWString tagMarker = MakeTagMarker(i);
		int pos = _stageThree.Find(tagMarker) + tagMarker.Length();
		if( pos < static_cast<int>(_stageThree.Length()) )
		{
			// Extract the next word position marker.
			int start = _stageThree.FindFirst(FxWordDelimiter, pos);
			int end   = _stageThree.FindFirst(FxWordDelimiter, start+1);
			FxWString wordMarker = _stageThree.Substring(start, end-start+1);
			// Read the word index and record it.
			_tagInfos[i].SetStartWord(ReadWordMarker(wordMarker));
			// Remove the marker from the string.
			_stageThree.Replace(tagMarker, L"");
		}

		// Find the end word marker.
		pos = _stageThree.Find(tagMarker) - 1;
		if( pos > 0 )
		{
			// Extract the previous word position marker.
			int end = _stageThree.FindLast(FxWordDelimiter, pos);
			int start = _stageThree.FindLast(FxWordDelimiter, end-1);
			FxWString wordMarker = _stageThree.Substring(start, end-start+1);
			// Read the word index and record it.
			_tagInfos[i].SetEndWord(ReadWordMarker(wordMarker));
			// Remove the marker from the string.
			_stageThree.Replace(tagMarker, L"");
		}

		// Ensure the start and end words are legitimate.
		if( _tagInfos[i].GetStartWord() == FxInvalidIndex )
		{
			_tagInfos[i].SetStartWord(0);
		}
		if( _tagInfos[i].GetEndWord() == FxInvalidIndex )
		{
			_tagInfos[i].SetEndWord(pWordList->GetNumWords()-1);
		}
	}
}

// Gets the string stripped of all its punctuation.
const FxWString& FxUnicodeTextTagParser::GetPartiallyStrippedText() const
{
	return _noPuncText;
}

// Gets the string stripped of all its text tags.
const FxWString& FxUnicodeTextTagParser::GetStrippedText() const
{
	return _strippedText;
}

// Returns the number of text tags.
FxSize FxUnicodeTextTagParser::GetNumTextTags( void ) const
{
	return _tagInfos.Length();
}

// Returns the text tag at index.
const FxUnicodeTextTagInfo& FxUnicodeTextTagParser::GetTextTag( const FxSize index ) const
{
	return _tagInfos[index];
}

// Returns the error that occurred during parsing.
FxUnicodeTextTagParser::FxTextTagError FxUnicodeTextTagParser::GetError() const
{
	return _error;
}

// Strips any unsupported punctuation from the string.
void FxUnicodeTextTagParser::StripPunctuation(const FxWString text)
{
	FxSize len = text.Length();
	_noPuncText.Clear();
	FxBool inTag = FxFalse;
	for( FxSize i = 0; i < len; ++i )
	{
		if( text[i] == FxTagOpen )
		{
			inTag = FxTrue;
		}

		if( !inTag )
		{
			// The only characters allowed in the string are alphanumeric, the
			// hyphen, the single quote, and a space.
			if( wxIsalnum(text[i]) || text[i] == wxT('-') || 
				text[i] == wxT('\'') || text[i] == wxT(' ') )
			{
				_noPuncText += text[i];
			}
		}
		else
		{
			_noPuncText += text[i];
		}

		if( text[i] == FxTagClose )
		{
			inTag = FxFalse;
		}
	}
}

// Replaces the text tags with markers.
FxBool FxUnicodeTextTagParser::ReplaceTextTagsWithMarkers(const FxWString& text)
{
	_stageOne = text;
	_strippedText = text;

	// Strip the tags and store the result.
	size_t startIndex = _strippedText.FindFirst(FxTagOpen);
	while( startIndex != FxInvalidIndex )
	{
		size_t endIndex = _strippedText.FindFirst(FxTagClose);
		// Flag mismatched brackets.
		if( endIndex == FxInvalidIndex) 
		{
			_error = TTE_UnmatchedBracket;
			//FxWarn("Text Tagging: Unmatched bracket: %s", _originalText.mb_str(wxConvLibc));
		}

		_strippedText.Replace(_strippedText.Substring(startIndex, endIndex-startIndex+1), L"");
		//_strippedText.Remove(startIndex, endIndex-startIndex+1);
		startIndex = _strippedText.FindFirst(FxTagOpen);
	}
	// Make sure there aren't any lone tag closing characters floating in the text.
	if( _strippedText.FindFirst(FxTagClose) != FxInvalidIndex )
	{
		_error = TTE_UnmatchedBracket;
		//FxWarn("Text Tagging: Unmatched bracket: %s", _originalText.mb_str(wxConvLibc));
	}

	FxBool keepGoing = FxTrue;
	while( keepGoing )
	{
		// Find the start and end of the next tag
		size_t startIndex = _stageOne.FindFirst(FxTagOpen);
		size_t endIndex   = _stageOne.FindFirst(FxTagClose);

		if( startIndex == FxInvalidIndex )
		{ 
			keepGoing = FxFalse;
			break;
		}
		
		// Extract the text tag and store it.
		FxUnicodeTextTagInfo tagInfo(_stageOne.Substring(startIndex, endIndex-startIndex+1));
		if( tagInfo.IsOpeningTag() && tagInfo.GetError() == FxUnicodeTextTagInfo::TE_None )
		{
			_tagInfos.PushBack(tagInfo);

			// Replace the tag with a marker.
			_stageOne.Replace(tagInfo.GetOriginalTag(), MakeTagMarker(_tagInfos.Length()-1));
		}
		else
		{
			// Find which tag we're closing and replace it with a marker.
			FxBool foundMatch = FxFalse;
			for( FxInt32 i = _tagInfos.Length()-1; i >= 0; --i )
			{
				if( _tagInfos[i].GetCurveName() == tagInfo.GetCurveName() )
				{
					foundMatch = FxTrue;
					_stageOne.Replace(tagInfo.GetOriginalTag(), MakeTagMarker(i));
				}
			}

			// Remove the ending tag if no start tag was found.
			if( !foundMatch )
			{
				_stageOne.Replace(tagInfo.GetOriginalTag(), wxEmptyString);
				_error = TTE_UnmatchedTag;
				//FxWarn("Text Tagging: Unmatched tag: %s", tagInfo.GetOriginalTag().mb_str(wxConvLibc));
			}
		}
	}
	return FxTrue;
}

// Replaces words with markers.
FxBool FxUnicodeTextTagParser::ReplaceWordsWithMarkers(const FxWString& text, const FxPhonWordList* pWordList)
{
	_stageTwo = text;
	FxSize numReplacements = 0;
	// Attempt to replace the words with no change in case.
	for( FxSize i = 0; i < pWordList->GetNumWords(); ++i )
	{
		numReplacements += _stageTwo.Replace(FxWString(pWordList->GetWordText(i).GetData()), MakeWordMarker(i));
	}

	// If we haven't found all the words, uppercase the search string and try again.
	if( numReplacements != pWordList->GetNumWords() )
	{
		_stageTwo = _stageTwo.ToUpper();
		for( FxSize i = 0; i < pWordList->GetNumWords(); ++i )
		{
			numReplacements += _stageTwo.Replace(FxWString(pWordList->GetWordText(i).GetData()), MakeWordMarker(i));
		}
	}
	
	// If we still haven't found all the words, flag an error.
	if( numReplacements != pWordList->GetNumWords() )
	{
		_error = TTE_UnmatchedWord;
		//FxWarn("Text Tagging: Unmatched word: %s", _originalText.mb_str(wxConvLibc));
	}

	return FxTrue;
}

// Strips all extraneous characters.
FxBool FxUnicodeTextTagParser::StripExtraneousCharacters(const FxWString& text)
{
	FxBool inMarker = FxFalse;
	// Run a simple state machine on the text.  Removes any character that isn't
	// a tag delimiter, word delimiter, or enclosed in either delimiter.
	for( FxSize i = 0; i < text.Length(); ++i )
	{
		if( text.GetChar(i) == FxTagDelimiter || text.GetChar(i) == FxWordDelimiter )
		{
			if( inMarker )
			{
				_stageThree += text.GetChar(i);
			}
			inMarker = !inMarker;
		}
		if( inMarker )
		{
			_stageThree += text.GetChar(i);
		}
	}
	return FxTrue;
}

// Makes a tag marker for the tag at index.
FxWString FxUnicodeTextTagParser::MakeTagMarker(FxSize index)
{
	FxString retval(static_cast<FxSize>(3));
	retval += FxTagDelimiter;
	retval << index;
	retval += FxTagDelimiter;
	return FxWString(retval.GetData());
}

// Makes a word marker for the word at index.
FxWString FxUnicodeTextTagParser::MakeWordMarker(FxSize index)
{
	FxString retval(static_cast<FxSize>(3));
	retval += FxWordDelimiter;
	retval << index;
	retval += FxWordDelimiter;
	return FxWString(retval.GetData());
}

// Returns the tag index for a tag marker
FxSize FxUnicodeTextTagParser::ReadTagMarker(const FxWString& tagMarker)
{
	FxWString temp = tagMarker.StripWhitespace().AfterFirst(FxTagDelimiter).BeforeLast(FxTagDelimiter);
	return FxAtoi(temp.GetCstr());
}

// Returns the word index for a word marker
FxSize FxUnicodeTextTagParser::ReadWordMarker(const FxWString& wordMarker)
{
	FxWString temp = wordMarker.StripWhitespace().AfterFirst(FxWordDelimiter).BeforeLast(FxWordDelimiter);
	return FxAtoi(temp.GetCstr());
}

} // namespace Face

} // namespace OC3Ent
