/*=============================================================================
	FacebookIntegration.cpp: Cross platform portion of Facebook integration.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UFacebookIntegration);


/**
 * Call through to subclass's native implementations function
 */
UBOOL UFacebookIntegration::Init()
{
	return NativeInit();
}
UBOOL UFacebookIntegration::Authorize()
{
	return NativeAuthorize();
}
UBOOL UFacebookIntegration::IsAuthorized()
{
	return NativeIsAuthorized();
}
void UFacebookIntegration::Disconnect()
{
	NativeDisconnect();
}
void UFacebookIntegration::WebRequest(const FString& URL,const FString& POSTPayload)
{
 	NativeWebRequest(URL, POSTPayload);
}
void UFacebookIntegration::FacebookRequest(const FString& GraphRequest)
{
 	NativeFacebookRequest(GraphRequest);
}


/**
 * Called by platform when authorization has completed
 */
void UFacebookIntegration::OnAuthorizationComplete(UBOOL bSucceeded)
{
	// call the delegates
	const TArray<FScriptDelegate> Delegates = AuthorizationDelegates;
	FacebookIntegration_eventOnAuthorizationComplete_Parms Parms(EC_EventParm);
	Parms.bSucceeded = bSucceeded;

	// Iterate through the delegate list
	for (INT Index = 0; Index < Delegates.Num(); Index++)
	{
		// Make sure the pointer if valid before processing
		const FScriptDelegate* ScriptDelegate = &Delegates(Index);
		if (ScriptDelegate != NULL)
		{
			// Send the notification of completion
			ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
		}
	}
}

/**
 * Called by platform when FB request has completed
 */
void UFacebookIntegration::OnFacebookRequestComplete(const FString& JsonString)
{
	// call the delegates
	const TArray<FScriptDelegate> Delegates = FacebookRequestCompleteDelegates;
	FacebookIntegration_eventOnFacebookRequestComplete_Parms Parms(EC_EventParm);
	Parms.JsonString = JsonString;

	// Iterate through the delegate list
	for (INT Index = 0; Index < Delegates.Num(); Index++)
	{
		// Make sure the pointer if valid before processing
		const FScriptDelegate* ScriptDelegate = &Delegates(Index);
		if (ScriptDelegate != NULL)
		{
			// Send the notification of completion
			ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
		}
	}
}

/**
 * Called by platform when web request has completed
 */
void UFacebookIntegration::OnWebRequestComplete(const FString& Response)
{
debugf(TEXT("WebRequestComplete:"));
debugf(TEXT("Response: %s"), *Response);
	// call the delegates
	const TArray<FScriptDelegate> Delegates = WebRequestCompleteDelegates;
	FacebookIntegration_eventOnWebRequestComplete_Parms Parms(EC_EventParm);
	Parms.Response = Response;

	// Iterate through the delegate list
	for (INT Index = 0; Index < Delegates.Num(); Index++)
	{
		// Make sure the pointer if valid before processing
		const FScriptDelegate* ScriptDelegate = &Delegates(Index);
		if (ScriptDelegate != NULL)
		{
			// Send the notification of completion
			ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
		}
	}
}




IMPLEMENT_CLASS(UJsonObject);

/**
 *  @return the JsonObject associated with the given key, or NULL if it doesn't exist
 */
UJsonObject* UJsonObject::GetObject(const FString& Key)
{
	return ObjectMap.FindRef(Key);
}

/**
 *  @return the value (string) associated with the given key, or "" if it doesn't exist
 */
FString UJsonObject::GetStringValue(const FString& Key)
{
	FString* Result = ValueMap.Find(Key);
	return Result ? *Result : FString(TEXT(""));
}


void UJsonObject::SetObject(const FString& Key, UJsonObject* Object)
{
	ObjectMap.Set(Key, Object);
}

void UJsonObject::SetStringValue(const FString& Key, const FString& Value)
{
	ValueMap.Set(Key, Value);
}

/**
 * Encodes an object hierarchy to a string suitable for sending over the web
 *
 * This code written from scratch, with only the spec as a guide. No
 * source code was referenced.
 *
 * @param Root The toplevel object in the hierarchy
 *
 * @return A well-formatted Json string
 */
FString UJsonObject::EncodeJson(UJsonObject* Root)
{
	// an objewct can't be an dictionary and an array
	if (Root->ValueMap.Num() + Root->ObjectMap.Num() > 0 && 
		Root->ValueArray.Num() + Root->ObjectArray.Num() > 0)
	{
		debugf(TEXT("A JsonObject can't have map values and array values"));
		return TEXT("ERROR!");
	}

	UBOOL bIsArray = Root->ValueArray.Num() + Root->ObjectArray.Num() > 0;

	// object starts with a {, unless it's an array
	FString Out = bIsArray ? TEXT("[") : TEXT("{");

	UBOOL bIsFirst = TRUE;

	if (bIsArray)
	{
		for (INT Index = 0; Index < Root->ValueArray.Num(); Index++)
		{
			// comma between elements
			if (!bIsFirst)
			{
				Out += TEXT(",");
			}
			else
			{
				bIsFirst = FALSE;
			}

			// output the key/value pair
			if (Root->ValueArray(Index).StartsWith(TEXT("\\#")))
			{
				// if we have a \#, then export it without the quotes, as it's a number, not a string
				Out += FString::Printf(TEXT("%s"), (*Root->ValueArray(Index)) + 2);
			}
			else
			{
				Out += FString::Printf(TEXT("\"%s\""), *Root->ValueArray(Index));
			}
		}
		for (INT Index = 0; Index < Root->ObjectArray.Num(); Index++)
		{
			// comma between elements
			if (!bIsFirst)
			{
				Out += TEXT(",");
			}
			else
			{
				bIsFirst = FALSE;
			}

			// output the key/value pair
			Out += FString::Printf(TEXT("%s"), *EncodeJson(Root->ObjectArray(Index)));
		}
	}
	else
	{
		for (TMap<FString, FString>::TIterator It(Root->ValueMap); It; ++It)
		{
			// comma between elements
			if (!bIsFirst)
			{
				Out += TEXT(",");
			}
			else
			{
				bIsFirst = FALSE;
			}

			// output the key/value pair
			if (It.Value().StartsWith(TEXT("\\#")))
			{
				// if we have a \#, then export it without the quotes, as it's a number, not a string
				Out += FString::Printf(TEXT("\"%s\":%s"), *It.Key(), (*It.Value()) + 2);
			}
			else
			{
				const FString& K = It.Key();
				const FString& V = It.Value();
				Out += FString::Printf(TEXT("\"%s\":\"%s\""), *It.Key(), *It.Value());
			}
		}

		for (TMap<FString, UJsonObject*>::TIterator It(Root->ObjectMap); It; ++It)
		{
			// comma between elements
			if (!bIsFirst)
			{
				Out += TEXT(",");
			}
			else
			{
				bIsFirst = FALSE;
			}

			// output the key/value pair
			Out += FString::Printf(TEXT("\"%s\":%s"), *It.Key(), *EncodeJson(It.Value()));
		}
	}

	Out += bIsArray ? TEXT("]") : TEXT("}");
	return Out;
}

/**
 * Parse a string from a Json string, starting at the given location
 */
static FString ParseString(const FString& InStr, INT& Index)
{
	const TCHAR* Str = *InStr;
	// skip the opening quote
	check(Str[Index] == '\"');
	Index++;

	// loop over the string until a non-quoted close quote is found
	UBOOL bDone = FALSE;
	FString Result;
	while (!bDone)
	{
		// handle a quoted character
		if (Str[Index] == '\\')
		{
			// skip the quote, and deal with it depending on next character
			Index++;
			switch (Str[Index])
			{
				case '\"':
				case '\\':
				case '/':
					Result += Str[Index]; break;
				case 'f':
					Result += '\f'; break;
				case 'r':
					Result += '\r'; break;
				case 'n':
					Result += '\n'; break;
				case 'b':
					Result += '\b'; break;
				case 't':
					Result += '\t'; break;
				case 'u':
					// 4 hex digits, like \uAB23, which is a 16 bit number that we would usually see as 0xAB23
					{
						TCHAR Hex[7];
						Hex[0] = '0'; Hex[1] = 'x'; 
						Hex[2] = Str[Index + 1]; 
						Hex[3] = Str[Index + 2]; 
						Hex[4] = Str[Index + 3]; 
						Hex[5] = Str[Index + 4]; 
						Hex[6] = 0;
						Index += 4;
						TCHAR UniChar = (TCHAR)appAtoi(Hex);
						Result += UniChar;
					}
					break;
				default:
					appErrorf(TEXT("Bad Json escaped char at %s"), Str[Index]);
					break;
			}
		}
		// handle end of the string
		else if (Str[Index] == '\"')
		{
			bDone = TRUE;
		}
		// otherwise, just drop it in to the output
		else
		{
			Result += Str[Index];
		}

		// always skip to next character, even in the bDone case
		Index++;
	}

	// and we are done!
	return Result;
}

/**
 * Skip over any whitespace at current location
 */
static void SkipWhiteSpace(const FString& InStr, INT& Index)
{
	// skip over the white space
	while (appIsWhitespace(InStr[Index]))
	{
		Index++;
	}
}

/**
 * Decodes a Json string into an object hierarchy (all needed objects will be created)
 *
 * This code written from scratch, with only the spec as a guide. No
 * source code was referenced.
 *
 * @param Str A Json string (probably received from the web)
 *
 * @return The root object of the resulting hierarchy
 */
UJsonObject* UJsonObject::DecodeJson(const FString& InJsonString)
{
	// strip off any white space
	FString JsonString = InJsonString;
	JsonString.Trim();
	JsonString.TrimTrailing();

	// get a pointer to the data
	const TCHAR* Str = *JsonString;

	// object or array start is required here
	if (Str[0] != '{' && Str[0] != '[' &&
		Str[JsonString.Len() - 1] != '}' && 
		Str[JsonString.Len() - 1] != ']')
	{
		debugf(TEXT("Poorly formed Json string at %s"), Str);
		return NULL;
	}

	// make a new object to hold the contents
	UJsonObject* NewObj = ConstructObject<UJsonObject>(UJsonObject::StaticClass());

	UBOOL bIsArray = Str[0] == '[';

	// parse the data, skipping over the opening bracket/brace
	INT Index = 1;
	while (Index < JsonString.Len() - 1)
	{
		SkipWhiteSpace(JsonString, Index);

		// if this is an dictionary, parse the key
		FString Key;
		if (!bIsArray)
		{
			Key = ParseString(JsonString, Index);
			SkipWhiteSpace(JsonString, Index);

			// this needs to be a :, skip over it
			check(Str[Index] == ':');
			Index++;

			SkipWhiteSpace(JsonString, Index);
		}

		// now read the value (for arrays and dictionaries, we are now pointing to a value)
		if (Str[Index] == '{' || Str[Index] == '[')
		{
			// look for the end
			INT EndOfSubString = Index;
			INT ObjectAndArrayStack = 0;
			// look for the end of the sub object/array 
			do 
			{
				// the start of an object or array increments the stack count
				if (Str[EndOfSubString] == '{' || Str[EndOfSubString] == '[')
				{
					ObjectAndArrayStack++;
				}
				// the end of an object or array decrements the stack count
				else if (Str[EndOfSubString] == '}' || Str[EndOfSubString] == ']')
				{
					ObjectAndArrayStack--;
				}
				EndOfSubString++;

			// go until the stack is finalized to 0
			} while (ObjectAndArrayStack > 0);

			// now we have the end of an object, recurse on that string
			FString SubString = JsonString.Mid(Index, EndOfSubString - Index);
			UJsonObject* SubObject = DecodeJson(SubString);

			// put the object into the arry or dictionary
			if (bIsArray)
			{
				NewObj->ObjectArray.AddItem(SubObject);
			}
			else
			{
				NewObj->ObjectMap.Set(Key, SubObject);
			}

			// skip past the object and carry on
			Index = EndOfSubString;
		}
		// if we aren't a subojbect, then we are a value (string or number)
		else
		{
			FString Value;
			if (Str[Index] == '\"')
			{
				Value = ParseString(JsonString, Index);
			}
			else
			{
				// look for the end of the number (first non-valid number character)
				INT EndOfNumber = Index;
				UBOOL bDone = FALSE;
				while (!bDone)
				{
					TCHAR Digit = Str[EndOfNumber];
					// possible number values, according to standard
					if ((Digit >= '0' && Digit <= '9') || 
						Digit == 'E' || Digit == 'e' || 
						Digit == '-' ||	Digit == '+' ||
						Digit == '.')
					{
						EndOfNumber++;
					}
					else
					{
						bDone = TRUE;
					}
				}
				// get the string of all the number parts, prepending \#, since how we denote numbers with unrealscript
				Value = FString("\\#") + JsonString.Mid(Index, EndOfNumber - Index);
				// skip past it
				Index = EndOfNumber;
			}

			// put the object into the arry or dictionary
			if (bIsArray)
			{
				NewObj->ValueArray.AddItem(Value);
			}
			else
			{
				NewObj->ValueMap.Set(Key, Value);
			}
		}

		// skip any whitespace after the value
		SkipWhiteSpace(JsonString, Index);

		// continue if not at the end yet
		if (Index < JsonString.Len() - 1)
		{
			// this needs to be a comma
			checkf(Str[Index] == ',', TEXT("Should be a , but it's %s"), Str + Index);
			Index++;

			// skip whitespace after a comma
			SkipWhiteSpace(JsonString, Index);
		}
	}

	return NewObj;
}

