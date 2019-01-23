/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "EditorPrivate.h"
#include "UnScrPrecom.h"

/**
 * Process the buffer pointed to by Begin and End into Buffer. Buffer will contain
 * a copy of the text between Begin and End with all UnrealScript-style (C++-style)
 * comments removed. Single line comments (// style) are replaced with the end of
 * line characters from the end of the line and multi-line comments (/* to * /) are
 * replaced with a single space OR all internal end-of-line characters, whichever is
 * longer. This keeps line counts the same in the input and output buffers
 * 
 * @param	Begin	pointer to the start of the source file's text
 * @param	End		pointer to the end of the source file's text
 * @param	Result	[out] buffer that will contain the post processed version of the text
 */
void FCommentStrippingFilter::Process(const TCHAR* Begin, const TCHAR* End, FString& Result)
{
	TCHAR ch;
	TCHAR matchQuote = ' ';
	UBOOL bQuoted = false, 
		bSingleLineComment = false, 
		bMultiLineComment = false,
		bWhitespaceGenerated = false, 
		bEscaped = false;

	SourceBuffer.Add(Begin, End);

	while (SourceBuffer) 
	{
		ch = SourceBuffer.GetChar();

		if (bMultiLineComment) 
		{
			if ((ch == TCHAR('*')) && SourceBuffer && (SourceBuffer.PeekChar() == TCHAR('/'))) 
			{
				SourceBuffer.GetChar(); // consume second character (of 2 character sequence)
				if (!bWhitespaceGenerated)
					Result.AppendChar(' '); // make sure a multiline comment generates _some_ white space
				bMultiLineComment = false;
			}
			else if ((ch == TCHAR('\r') || (ch == TCHAR('\n')))) 
			{
				Result += ch;
				if (SourceBuffer && (SourceBuffer.PeekChar() == TCHAR('\n'))) 
				{
					ch = SourceBuffer.GetChar(); // consume second character (of 2 character sequence)
					Result += ch;
				}
				bWhitespaceGenerated = true;
			}
		}
		else if (bSingleLineComment) 
		{
			if ((ch == TCHAR('\r') || (ch == TCHAR('\n')))) 
			{
				Result += ch;
				if (SourceBuffer && (SourceBuffer.PeekChar() == TCHAR('\n'))) 
				{
					ch = SourceBuffer.GetChar(); // consume second character (of 2 character sequence)
					Result += ch;
				}
				bSingleLineComment = false;
			}
		}
		else 
		{
			if (bQuoted)
			{
				Result += ch;
				if (!bEscaped) 
				{
					if (ch == TCHAR('\\')) 
					{
						bEscaped = true;
					}
					else if (ch == matchQuote) 
						bQuoted = false;
				}
				else 
				{
					bEscaped = false;
				}
			}
			else if ((ch == TCHAR('/')) && SourceBuffer) 
			{
				if (SourceBuffer.PeekChar() == TCHAR('/')) 
				{
					SourceBuffer.GetChar(); // consume second character (of 2 character sequence)
					bSingleLineComment = true;
				}
				else if (SourceBuffer.PeekChar() == TCHAR('*')) 
				{
					SourceBuffer.GetChar(); // consume second character (of 2 character sequence)
					bWhitespaceGenerated = false;
					bMultiLineComment = true;
				}
				else 
				{
					Result += ch;
				}
			}
			else if ((ch == TCHAR('"')) || (ch == TCHAR('\''))) 
			{
				Result += ch;
				bQuoted = true;
				matchQuote = ch;
			}
			else 
			{
				Result += ch;
			}
		}
	}
}


/**
* This class processes a buffer of text, expanding macro expressions.
* Constructed with the name of the package; this gives an initial search
* path for included files.
*
*/
FMacroProcessingFilter::FMacroProcessingFilter(const TCHAR * pName, const FString & fileName, FFeedbackContext* inWarn) 
:  PackageName(pName), FTextFilter(fileName, inWarn), NestLevel_If(0)
{
	EnableOutputFlag.Push(true);
	InitSymbolTable();
}


/**
* Initializes the global macro table and adds all hard-coded macro names, such as "debug" and "final_release"
*
*/
void FMacroProcessingFilter::InitSymbolTable()
{
	if (ParseParam(appCmdLine(), TEXT("debug"))) 
	{
		GlobalSymbols.Set(NAME_Debug, TEXT("..."));

		// Add the logd macro
#if FINAL_RELEASE
		GlobalSymbols.Set(TEXT("logd"), TEXT(""));
#else
		TArray<FString> ParameterList;
		ParameterList.AddItem(TEXT("msg"));
		ParameterList.AddItem(TEXT("cond"));
		ParameterList.AddItem(TEXT("tag"));

		GlobalSymbols.Set(TEXT("logd"), FScriptMacroDefinition(ParameterList, TEXT("`if(`cond)if (`cond) `{endif}LogInternal(`msg`if(`tag),`tag`endif)")));
#endif
	}
	else
	{
		GlobalSymbols.Set(TEXT("logd"), TEXT(""));
	}


#if FINAL_RELEASE
	GlobalSymbols.Set(FName(TEXT("FINAL_RELEASE")), TEXT("..."));
	GlobalSymbols.Set(NAME_Log, TEXT(""));
	GlobalSymbols.Set(TEXT("Warn"), TEXT(""));
	GlobalSymbols.Set(NAME_Assert, TEXT(""));
#else
	if ( ParseParam(appCmdLine(), TEXT("FINAL_RELEASE")) )
	{
		GlobalSymbols.Set(FName(TEXT("FINAL_RELEASE")), TEXT("..."));
		GlobalSymbols.Set(NAME_Log, TEXT(""));
		GlobalSymbols.Set(TEXT("Warn"), TEXT(""));
		GlobalSymbols.Set(NAME_Assert, TEXT(""));
	}
	else
	{
		// add the log macro
		TArray<FString> ParameterList;
		ParameterList.AddItem(TEXT("msg"));
		ParameterList.AddItem(TEXT("cond"));
		ParameterList.AddItem(TEXT("tag"));

		GlobalSymbols.Set(NAME_Log, FScriptMacroDefinition(ParameterList, TEXT("`if(`cond)if (`cond) `{endif}LogInternal(`msg`if(`tag),`tag`endif)")));

		// Warn doesn't require [or allow] a tag to be specified, so remove it from the parameter list
		ParameterList.Pop();
		GlobalSymbols.Set(TEXT("Warn"), FScriptMacroDefinition(ParameterList, TEXT("`if(`cond)if (`cond) `{endif}WarnInternal(`msg)")));

		// Add the assert macro
		ParameterList.Empty();
		ParameterList.AddItem(TEXT("cond"));
		GlobalSymbols.Set(NAME_Assert, FScriptMacroDefinition(ParameterList, TEXT("Assert(`cond)")));
	}
#endif

	// game specific symbols we need in the UnrealScript Compiler.  
    // We have do this at runtime as GAMENAME is not a valid #define in this module. 
	// @todo look to make an Engine virtual function to call instead of this hardcodedness
	if( FString(GGameName) == TEXT("War") )
	{
		if ( ParseParam(appCmdLine(), TEXT("MGS_AUTOMATION")) == TRUE )
		{
			GlobalSymbols.Set(FName(TEXT("_VINCE_")), TEXT("..."));
			GlobalSymbols.Set(FName(TEXT("_MGSTEST_")), TEXT("..."));
		}		
	}
}


FString FMacroProcessingFilter::GetContext()
{
	FString Path = appBaseDir(), Filepath;

	// remove the trailing path separator from the base directory
	if( Path.Right(1)==PATH_SEPARATOR )
		Path = Path.LeftChop(1);

	// remove the "Binaries" part from the base directory, so that we can build an absolute path
	if ( Path.Len() && Path.Right(1) != PATH_SEPARATOR )
	{
		INT pos = Path.InStr(PATH_SEPARATOR,true);
		Path = Path.Left(pos);
	}

	Filepath = SourceBuffer.GetCurrentProvider();
	if ( Filepath.Len() == 0 )
	{
		// if we don't have a valid provider, build a path for our filename 
		// if the doesn't contain the input directory, add that part
		if ( Filename.InStr(*GEditor->EditPackagesInPath) == INDEX_NONE )
			Filepath = GEditor->EditPackagesInPath * PackageName * TEXT("Classes");

		Filepath *= Filename;
	}

	// if the file has a relative path name, remove the relative part
	if ( Filepath.Left(1) == TEXT(".") )
	{
		INT pos = Filepath.InStr(PATH_SEPARATOR);
		if ( pos != INDEX_NONE )
		{
			Filepath = Filepath.Mid(pos+1);
		}
	}

	Path *= Filepath;
	return FString::Printf(TEXT("%s(%i)"),*Path,SourceBuffer.GetCurrentLineNumber());
}

/**
 * Searches through the input buffer, replacing all macro references with the expanded version.
 * 
 * @param	Begin	pointer to the start of the source file's text
 * @param	End		pointer to the end of the source file's text
 * @param	Result	[out] buffer that will contain the post processed text
 */
void FMacroProcessingFilter::Process( const TCHAR* Begin, const TCHAR* End, FString& Result )
{
	TCHAR ch;
	if ( Filename.InStr(TEXT("..")) == INDEX_NONE && Filename.InStr(TEXT("\\")) == INDEX_NONE && Filename.InStr(TEXT("/")) == INDEX_NONE )
	{
		// no path information specified in the filename - so add it
		Filename = GEditor->EditPackagesInPath * PackageName * TEXT("Classes") * Filename;
	}

	SourceBuffer.AddFile(Begin, End, Filename);
	FString Unemitted;


	UBOOL InQuote = false,		// indicates that we're currently inside a literal string
		InComment = false;		// indicates that we're currently inside a single line comment (ignore everything except newline)
	INT MultilineCommentLevel=0;// indicates that we're currently inside a multi-line comment (ignore everything except additional multi-line command tokens)

	while (SourceBuffer) 
	{
		ch = SourceBuffer.GetChar();
		if ( ch == 0 )
			continue;

		if ( ch == TCHAR('\\') && InQuote )
		{
			// Emit escape characters only if output is enabled.
			if ( IsOutputEnabled() )
			{
				// if we're inside a literal string, and the current character
				// is the escape character, eat the next character, then continue
				Result += ch;
				Result += SourceBuffer.GetChar();
			}
			else
			{
				// Eat current and next char; store them in Unemitted for debugging purposes.
				Unemitted += ch;
				Unemitted += SourceBuffer.GetChar();
			}
			continue;
		}

		// if the current character is a quote, and we aren't inside a comment, flip the InQuote flag
		if ( ch == TCHAR('"') && !InComment && MultilineCommentLevel == 0 )
		{
			InQuote = !InQuote;
		}
		
		if ( ch != CALL_MACRO_CHAR || (!SourceBuffer.IsMacro() && (InQuote || InComment || MultilineCommentLevel > 0)) ) 
		{
			if ( IsOutputEnabled() || ch == TCHAR('\r') || ch == TCHAR('\n') ) 
			{
				if (Unemitted.Len() != 0) 
				{
					//debugf(TEXT("Unemitted characters = \"%s\""), *Unemitted);
					Unemitted = TEXT("");
				}

				Result += ch;

				if ( IsOutputEnabled() && !InQuote )
				{
					// now perform any special logic needed when we encounter certain characters

					// don't process any macros while inside comments
					TCHAR NextChar = SourceBuffer.PeekChar();

					// if we're currently inside a single line comment, but we're still allowed to start and end
					// multi-line comment blocks
					if ( InComment )
					{
						if ( ch == TCHAR('\n') )
						{
							// we've reached the end of the line
							InComment = false;
						}

						// multi-line comment block end sequence
						else if ( ch == TCHAR('*') && NextChar == TCHAR('/') )
						{
							if( --MultilineCommentLevel < 0 )
								appThrowf( TEXT("Unexpected '*/' outside of comment") );
	
							// eat the second character
							SourceBuffer.GetChar();
							Result += NextChar;
						}

						// we've encountered the multi-line comment start sequence
						else if ( ch == TCHAR('/') && NextChar == TCHAR('*') )
						{
							MultilineCommentLevel++;

							// eat this character
							SourceBuffer.GetChar();
							Result += NextChar;
						}
					}

					else if ( ch == TCHAR('*') && NextChar == TCHAR('/') )
					{
						// we've encountered a multi-line comment closing token
						// decrement the multi-line comment level
						if( --MultilineCommentLevel < 0 )
							appThrowf( TEXT("Unexpected '*/' outside of comment") );

						// eat the second character
						SourceBuffer.GetChar();
						Result += NextChar;
					}
					else if ( ch == TCHAR('/') )
					{
						if ( NextChar == TCHAR('*') )
						{
							// we've encountered the multi-line comment start sequence
							MultilineCommentLevel++;
							
							// eat this character
							SourceBuffer.GetChar();
							Result += NextChar;
					}
						else if ( NextChar == TCHAR('/') )
						{
							// we've encountered the single line comment start sequence
							InComment = 1;

							// eat this character
							SourceBuffer.GetChar();
							Result += NextChar;
						}
					}
				}
			}
			else 
			{
				// we just keep track of this for debugging purposes
				// these are the characters that won't exist in the post-processed version of the file.
				Unemitted += ch;
			}
		}
		else
		{
			FName macroName = FName(*ParseMacroName());
			if ( ShouldProcessMacro(macroName) )
				ExpandMacro(macroName);
		}
	}
}

/**
* Parses the name of the macro from the buffer.  SourceBuffer should be positioned at the first character 
* of the name, or at a BEGIN_MACRO_BLOCK_CHAR.  Advances the position of SourceBuffer past the macroname.
*
* @return  the name of the macro
*
*/
FString FMacroProcessingFilter::ParseMacroName() 
{
	UBOOL wrapped = false;
	TCHAR ch;
	FString retval;

	if ( SourceBuffer && SourceBuffer.PeekChar() == BEGIN_MACRO_BLOCK_CHAR) 
	{
		SourceBuffer.GetChar(); // advance past the wrapper
		wrapped = true;
	}
	while ( SourceBuffer ) 
	{
		ch = SourceBuffer.GetChar();

		// if this character is whitespace, we've reached the end of the macro name
		if ( appIsWhitespace(ch) ||

		// if this character is not an alphanumeric character and it isn't the built-in macro-name used for the macro parameter count, we've reached the end of the macro name
			(!appIsAlnum(ch) && ch != TCHAR('_') && ch!=MACRO_PARAMCOUNT_CHAR) )	
		{
			//@fixme ronp - this breaks using macros that do not have parameter lists because we eat all trailing whitespace, so if the macro was used to conditionally compile
			// a function keyword, for example:
			// native final `maybe_private function foo();
			//
			// when `maybe_private is defined to "private", it results in the following:
			// native final privatefunction foo();
			//
			// because we're eating the trailing whitespace
			while ( appIsWhitespace(ch) )
			{
				// eat all whitepsace between the macroname and the parameter list or definition
				ch = SourceBuffer.GetChar();
				check(SourceBuffer);	//@todo: handle this gracefully
			}

			// put the character back - it isn't part of the macro name
			SourceBuffer.UngetChar(ch);
			break;
		}

		if (ch != 0)
			retval += ch;
	}

	if ( wrapped ) 
	{
		ch = SourceBuffer.GetChar();
		if (ch != END_MACRO_BLOCK_CHAR) 
		{
			appThrowf(TEXT("Unterminated macro name"));
		}
	}
	return retval;
}

/**
* Parses the macro definition from the Buffer.  SourceBuffer should be positioned at the beginning of the macro definition.
*
* @param	NewMacroName	the name of macro this definition will be associated with.  currently only used for error message purposes.
* @return					the definition for the macro
*/
FString FMacroProcessingFilter::ParseMacroDefinition( const TCHAR* NewMacroName )
{
	FString Result;
	UBOOL bSpanNewLine = false;
	for ( TCHAR ch = SourceBuffer.GetChar(); ch != 0; ch = SourceBuffer.GetChar() )
	{
		if ( ch == TCHAR('\\') )
		{
			if ( SourceBuffer.PeekChar() == TCHAR('n') )
			{
				// we want to insert a manual line break

				// eat the "n"
				SourceBuffer.GetChar();

				// then insert a line break
				Result += LINE_TERMINATOR;
				continue;
			}

			// uncomment these lines to only allow backslashes that appear at the end
			// of the line to be considered a valid line-span request
//			TCHAR nextChar = SourceBuffer.PeekChar();
//			if ( nextChar == TCHAR('\r') || nextChar == TCHAR('\n') )
//			{
				bSpanNewLine = true;

				// don't add the backslash to the macro definition
				continue;
//			}
		}

		if ( ch == TCHAR('\n') ||
			(ch == TCHAR('\r') && SourceBuffer.PeekChar() == TCHAR('\n')) )
		{
			if ( bSpanNewLine )
			{
				// switch off bSpanNewLine when the current character is the \n character
				bSpanNewLine = ch == TCHAR('\r');
				if ( !bSpanNewLine )
				{
					// eat all leading whitespace on the next line
					while ( appIsWhitespace(ch = SourceBuffer.GetChar()) );
					SourceBuffer.UngetChar(ch);
				}

				// skip over new lines when expanding multi-line macro definitions
				// or the line number count will be off
				continue;
			}
			else
                break;
		}
		else if ( bSpanNewLine && !appIsWhitespace(ch) )
		{
			appThrowf(TEXT("Non whitespace character encountered after line-span character in definition for macro '%s'."), NewMacroName ? NewMacroName : TEXT("Unknown"));
		}

		Result += ch;
	}

	return Result;
}

/**
* Retrieves the next parameter from a macro expansion.  It is assumed that we are just past the opening
* left parenthesis or comma. This function will skip leading whitespace and assemble the parameter until
* it sees a  top-level (to it) comma or closing parenthesis. It will expand macros as they are encountered.
*
* @return  the next parameter for the current macro
*
*/
FString FMacroProcessingFilter::ParseNextParam()
{
	FString param;

	if ( !SourceBuffer )
		return FString();

	TCHAR ch = SourceBuffer.GetChar();

	// parameters skip leading blanks
	while ( appIsWhitespace(ch) )
	{
		if ( !SourceBuffer )
		{
			// this means we ran our of character while attempting to parse the next parameter
			appThrowf(TEXT("End of script encountered while attempting to parse macro parameter"));
		}

		ch = SourceBuffer.GetChar();
	}


	bool bQuoted = false;
	bool bEscaped = false;
	TCHAR closingQuote = TCHAR(' ');

	 // for the ( at the beginning of the parameters
	for (INT nestingLevel = 1; nestingLevel > 0; ch = SourceBuffer.GetChar()) 
	{
		if ( !SourceBuffer )
		{
			// this means we ran our of character while attempting to parse the next parameter
			appThrowf(TEXT("End of script encountered while attempting to parse macro parameter"));
		}

		if ( bQuoted )
		{
			if ( !bEscaped )
			{
				if ( ch == TCHAR('\\') )
				{
					bEscaped = true;
				}
				else if ( ch == closingQuote )
				{
					bQuoted = false;
				}
			}
			else
			{
				bEscaped = false;
			}
		}
		else if ( ch == TCHAR('"') || ch == TCHAR('\'') )
		{
			bQuoted = true;
			closingQuote = ch;
		}
		else
		{
			if (ch == '(') 
			{
				++nestingLevel;
			}

			else if ((ch == ')') || ((nestingLevel == 1) && (ch == ','))) 
			{
				--nestingLevel;
				if ( nestingLevel == 0 )
				{
					// we've reached the end of the parameter - remove any trailing spaces
					if ( param.Len() > 0 )
					{
						INT pos = param.Len();
						for ( ; pos > 0; pos-- )
						{
							if ( !appIsWhitespace(param[pos - 1]) )
							{
								break;
							}
						}
						if ( pos == 0 )
						{
							// entire parameter was whitespace
							param.Empty();
						}
						else if ( pos < param.Len() )
						{
							// trailing whitespace
							param = param.Left(pos);
						}
					}
					break;
				}
			}
		}

		if ( ch != CALL_MACRO_CHAR || bEscaped ) 
		{
			if (IsOutputEnabled() && ch != 0)
			{
				param += ch;
			}
		}
		else
		{
			FName macroName = FName(*ParseMacroName());
			if ( ShouldProcessMacro(macroName) )
			{
				ExpandMacro(macroName);
			}
		}
	}

	SourceBuffer.UngetChar(ch);
	return param;
}

/**
* Retrieves the parameters for the macro currently being parsed.
*
* @param   Params   [out]	the parameters listed in the text for the call to the macro
* 
* @return  the number of parameters parsed
*
*/
INT FMacroProcessingFilter::GetParamList(TArray<FString> & Params)
{
	// make sure we're positioned at the opening parenthesis
	if (SourceBuffer.PeekChar() != '(') 
		return 0;

	Params.Empty();
	while (SourceBuffer.PeekChar() != ')') 
	{
		// consume the opening parenthesis or comma
		SourceBuffer.GetChar();
		FString nextParameter = ParseNextParam();
		new (Params) FString(nextParameter);
	}

	// consume the closing parenthesis
	SourceBuffer.GetChar();
	return Params.Num();

}

/**
 * Replaces a macro with the expanded definition for that macro.
 * 
 * @param	macroName	name of the macro to expand
 * 
 */
void FMacroProcessingFilter::ExpandMacro(const FName& macroName)
{
	FScriptMacroDefinition* definition = NULL;
	TArray<FString> parameters;

	if ( macroName == NAME_If )
	{
		if ( (SourceBuffer.PeekChar() != '(') 		// if the next character isn't the opening parenthesis
			|| (GetParamList(parameters)  != 1))	// or we don't have exactly one parameter before the closing parenthesis
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 1, found %d."), *macroName.ToString(), parameters.Num());
		}
		else 
		{
			++NestLevel_If;
			if (NestLevel_If >= 1)
			{
				EnableOutputFlag.Push(parameters(0) != TEXT(""));
			}
		}
	}
	else if ( macroName == NAME_Else ) 
	{
		if ( (SourceBuffer.PeekChar() == '(')		// if the next character is an opening parenthesis
			&& (GetParamList(parameters)  != 0))	// and parameters were specified
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 0, found %d."), *macroName.ToString(), parameters.Num());
		}
		if ((NestLevel_If >= 1) && (EnableOutputFlag.Num() > 1)) 
		{
			UBOOL switchValue = !EnableOutputFlag.Pop();
			EnableOutputFlag.Push(switchValue);
		}
	}
	else if ( macroName == NAME_EndIf ) 
	{
		if ( (SourceBuffer.PeekChar() == '(') 
			&& (GetParamList(parameters)  != 0)) 
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 0, found %d."), *macroName.ToString(), parameters.Num());
		}
		--NestLevel_If;
		if ((NestLevel_If >= 0) && (EnableOutputFlag.Num() > 1)) 
		{
			EnableOutputFlag.Pop();
		}
	}
	else if ( macroName == NAME_Include )
	{
		if ( (SourceBuffer.PeekChar() != '(')	// if the next character isn't the opening parenthesis
			|| (GetParamList(parameters) != 1))	// or we don't have exactly one parameter before the closing parenthesis
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 1, found %d."), *macroName.ToString(), parameters.Num());
		}
		else 
		{
			const FString& origFileName = parameters(0);
			FString fileName = origFileName;
			FString fileContent=TEXT("");

			UBOOL bNoPathInfoSpecified = fileName.InStr(TEXT("..")) == INDEX_NONE && fileName.InStr(TEXT("\\")) == INDEX_NONE && fileName.InStr(TEXT("/")) == INDEX_NONE;
			if ( bNoPathInfoSpecified )
			{
				// No special characters in the filename -  attempt to search this package's directory; first look for it in the Classes directory
				fileName = GEditor->EditPackagesInPath * PackageName * TEXT("Classes") * origFileName;
				appLoadFileToString(fileContent, *fileName);

				if ( fileContent.Len() == 0 )
				{
					// wasn't found there....look in the package's base directory
					fileName = GEditor->EditPackagesInPath * PackageName * origFileName;
					appLoadFileToString(fileContent, *fileName);
				}
			}

			if ( fileContent.Len() == 0 )
			{
				fileName = GEditor->EditPackagesInPath * origFileName;
				appLoadFileToString(fileContent, *fileName);
			}

			if (fileContent.Len() != 0) 
			{
				// run the comment stripper on the newly included file
				FCommentStrippingFilter stripper(fileName, Warn);
				FString strippedFileContent;
				stripper.Process(*fileContent, *fileContent + fileContent.Len(), strippedFileContent);
				SourceBuffer.AddFile(*strippedFileContent, *strippedFileContent + strippedFileContent.Len(), fileName);
			}
			else 
			{
				FString Locations = fileName;
				if ( bNoPathInfoSpecified )
				{
					Locations = Locations + TEXT(", ") + (GEditor->EditPackagesInPath * PackageName * TEXT("Classes") * origFileName)
						+ TEXT(", or ") + (GEditor->EditPackagesInPath * PackageName * origFileName);
				}
				appThrowf(TEXT("Unable to read include file '%s' from %s"), *origFileName, *Locations);
			}
		}
	}
	else if ( macroName == NAME_Define )
	{
		TCHAR ch = SourceBuffer.GetChar();
		for ( ; ch != 0 && appIsWhitespace(ch); ch = SourceBuffer.GetChar() )
		{
			// eat up all whitespace between the word "define" and the name of the new macro
			//@todo no error handling here - i.e. if SourceBuffer runs our of characters
		}

		// ch == 0 if we don't have any more characters in the current file
		if ( ch != 0 )
		{
			// if this isn't an alpha numeric character
			if ( !appIsAlpha(ch) )
			{
				appThrowf(TEXT("Macro names must begin with an alpha numeric character."));
			}

			FString NewMacroName, NewMacroValue;
			while ( ch != 0 && !appIsWhitespace(ch)
				&& ch != TCHAR('(') && ch != TCHAR(')')
				&& ch != TCHAR('\r') && ch != TCHAR('\n') )
			{
				//@todo: handle cr/lf and zero
				NewMacroName += ch;
				ch = SourceBuffer.GetChar();
			}

			if ( ch == 0 )
			{
				appThrowf(TEXT("End of file encountered while parsing name for new macro definition."));
			}

			// we now have the new macro's name.  ch is pointing at the first whitespace
			// character after the name of the macro definition
			SourceBuffer.UngetChar(ch);

			// check for any illegal macro names
			if ( NewMacroName == TEXT("#") )
			{
				appThrowf(TEXT("# is a reserved macro name."));
			}

			// grab the optional macro parameter list
			TArray<FString> MacroParameters;
			INT NumParams = GetParamList(MacroParameters);

			// eat all whitespace between the macroname or parameter list and the macro definition
			do 
			{
				ch = SourceBuffer.GetChar();

				// if we encounter the span-line character, skip over it an
				if ( ch == TCHAR('\\') )
				{
					do
					{
						ch = SourceBuffer.GetChar();
					} 
					while( ch != 0 && ch != TCHAR('\n') );

					// change ch to point to the first character of the new line
					ch = SourceBuffer.GetChar();
				}
			} while ( ch != 0 && appIsWhitespace(ch) );

			// ch is now pointing at the first character of the macro definition
			// put that character back as it will be needed by ParseMacroDefinition
			SourceBuffer.UngetChar(ch);
			NewMacroValue = ParseMacroDefinition(*NewMacroName);

			GlobalSymbols.Set(FName(*NewMacroName), FScriptMacroDefinition(MacroParameters, *NewMacroValue));
		}
		else
		{
			appThrowf(TEXT("End of file encountered while searching for name of new macro definition."));
		}
	}
	else if ( macroName == NAME_IsDefined )
	{
		if ((SourceBuffer.PeekChar() != '(') || (GetParamList(parameters)  != 1)) 
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 1, found %d."), *macroName.ToString(), parameters.Num());
		}
		else 
		{
			const FString& defMacroName = parameters(0);
			if (Lookup(*defMacroName, definition))
			{
				// if this macro was found, inject a non-zero character into
				// the input stream so that the `if will resolve to true
				SourceBuffer.UngetChar('1');
			}
		}
	}
	else if ( macroName == NAME_NotDefined )
	{
		if ((SourceBuffer.PeekChar() != '(') || (GetParamList(parameters)  != 1)) 
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 1, found %d."), *macroName.ToString(), parameters.Num());
		}
		else 
		{
			const FString& defMacroName = parameters(0);
			if (!Lookup(*defMacroName, definition))
			{
				// if this macro wasn't found, inject a non-zero character into
				// the input stream so that the `if will resolve to true
				SourceBuffer.UngetChar('1');
			}
		}
	}
	else if ( macroName == NAME_Undefine )
	{
		if ((SourceBuffer.PeekChar() != '(') || (GetParamList(parameters)  != 1)) 
		{
			appThrowf(TEXT("Wrong number of parameters for '%s' macro. Expected 1, found %d."), *macroName.ToString(), parameters.Num());
		}
		else
		{
			FName undefMacroName = FName(*parameters(0));
			if ( Lookup(undefMacroName, definition) )
				GlobalSymbols.Remove(undefMacroName);
		}
	}

	// otherwise, it's a custom macro
	else if ( Lookup(macroName, definition)) 
	{
		if (SourceBuffer.PeekChar() == '(') 
		{
			GetParamList(parameters);
			FString& value = definition->MacroDefinition;
			SourceBuffer.AddMacro(*value, *value + value.Len(), macroName, definition, &parameters);
		}
		else 
		{
			// this macro has no parameters - in order for it to be correctly evaluated,
			// it needs to have the definition of the macro as its only parameter
			FString& value = definition->MacroDefinition;

			// insert the macro's name and value into its list of parameters
			definition->AddParameterName(*macroName.ToString(),0);
			parameters.InsertZeroed(0);
			parameters(0) = value;

			SourceBuffer.AddMacro(*value, *value + value.Len(), macroName, definition, &parameters);
		}
	}

	else
	{
		appThrowf(TEXT("Unknown macro '%s'."), *macroName.ToString());
	}
}

/**
* Determines whether the specified macro should be processed.
*
* @param macroName the name of the macro
*
* @return TRUE if the macro should be processed
*/
UBOOL FMacroProcessingFilter::ShouldProcessMacro(const FName& macroName)
{
	return IsOutputEnabled()	// if output is enabled, process the macro

		// always process the following macros, even inside of `if() blocks
		|| macroName == NAME_If
		|| macroName == NAME_Else
		|| macroName == NAME_EndIf;
}
