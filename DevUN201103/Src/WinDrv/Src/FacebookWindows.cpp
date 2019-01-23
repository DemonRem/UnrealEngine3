/*=============================================================================
	WindFacebook.cpp: Windows specific Facebook integration.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "Engine.h"
#include <wininet.h>

/**
 * Callback function that is called whenever anything happens on the network
 */
void CALLBACK InternetStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext,
									 DWORD dwInternetStatus, LPVOID lpvStatusInformation,
									 DWORD dwStatusInformationLength);


class UFacebookWindows : public UFacebookIntegration
{
	DECLARE_CLASS_INTRINSIC(UFacebookWindows, UFacebookIntegration, 0, WinDrv)

	virtual UBOOL NativeInit()
	{
		// setup net connection
		NetHandle = InternetOpen(TEXT("UWinFacebook"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);

		// set the callback
		InternetSetStatusCallback(NetHandle, InternetStatusCallback);

		return NetHandle != NULL;
	}

	virtual void BeginDestroy()
	{
		// shut down WinINet
		InternetCloseHandle(NetHandle);

		Super::BeginDestroy();
	}
	
	virtual UBOOL NativeAuthorize()
	{
		if (!HACK_IsAuthorized)
		{
			// temp hardcoded hack
			HACK_IsAuthorized = TRUE;
			UserName = TEXT("Fake FB User");

			OnAuthorizationComplete(TRUE);
		}

		return TRUE;
	}

	virtual UBOOL NativeIsAuthorized()
	{
		return HACK_IsAuthorized;
	}

	virtual void NativeDisconnect()
	{
	}

	void NativeWebRequest(const FString& URL, const FString& POSTPayload)
	{
		// crack open the URL into its component parts
		URL_COMPONENTS URLParts;
		TCHAR Host[1024];
		TCHAR Resource[1024];

		// fill out the URL crack request
		appMemzero(&URLParts, sizeof(URLParts));
		URLParts.dwStructSize = sizeof(URLParts);
		URLParts.lpszHostName = Host;
		URLParts.dwHostNameLength = 1024;
		URLParts.lpszUrlPath = Resource;
		URLParts.dwUrlPathLength = 1024;

		InternetCrackUrl(*URL, 0, 0, &URLParts);

		// is it a secure request?
		UBOOL bIsHTTPS = URLParts.nScheme == INTERNET_SCHEME_HTTPS;

		DWORD RequestFlags = 0;
		DWORD Port = INTERNET_DEFAULT_HTTP_PORT;
		if (bIsHTTPS)
		{
			RequestFlags |= INTERNET_FLAG_SECURE;
			Port = INTERNET_DEFAULT_HTTPS_PORT;
		}

		// did the URL specify an alternate port?
		if (URLParts.nPort)
		{
			Port = URLParts.nPort;
		}

		// open a connection to the URL
		ConnectionHandle = InternetConnect(NetHandle, Host, Port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)this);

		// create the right type of request, and send it
		if (POSTPayload.Len() > 0)
		{
			RequestHandle = HttpOpenRequest(ConnectionHandle, TEXT("POST"), Resource, NULL, NULL, NULL, RequestFlags, (DWORD_PTR)this);
			HttpSendRequest(RequestHandle, NULL, 0, TCHAR_TO_UTF8(*POSTPayload), POSTPayload.Len() + 1);
		}
		else
		{
			RequestHandle = HttpOpenRequest(ConnectionHandle, TEXT("GET"), Resource, NULL, NULL, NULL, RequestFlags, (DWORD_PTR)this);
			HttpSendRequest(RequestHandle, NULL, 0, NULL, 0);
		}

		// disble security checking for now
		if (bIsHTTPS)
		{
			// allow self-signed certs
			DWORD SecurityFlags;
			DWORD FlagsLen = sizeof(SecurityFlags);
			// get current options
			InternetQueryOption (RequestHandle, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&SecurityFlags, &FlagsLen);

			// add the ignore unknown CA option, and allow for whatever name
			SecurityFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_REVOCATION;
			InternetSetOption (RequestHandle, INTERNET_OPTION_SECURITY_FLAGS, &SecurityFlags, sizeof(SecurityFlags));
		}
	}

	void NativeFacebookRequest(const FString& GraphRequest)
	{
		// @todo
	}

	/**
	 * Pulls data off the network stream when a web request completes
	 *
	 * @param Result The result structure for the request (error info)
	 */
	void ProcessReceivedData(INTERNET_ASYNC_RESULT* Result)
	{
		// request has completed! did it error out?
		if (Result->dwResult == 0)
		{
			FScopeLock ScopeLock(&ReplySyncObject);
			WebReply = FString::Printf(TEXT("WebRequest failed with error %d"), Result->dwError);
		}
		else
		{
			FString FullResult;

			// read the data from the result stream
			ANSICHAR Buffer[2048];
			UBOOL bDone = FALSE;

			// loop until complete
			while (!bDone)
			{
				DWORD BytesToRead = 2047;
				DWORD BytesRead;
				InternetReadFile(RequestHandle, Buffer, BytesToRead, &BytesRead);
				if (BytesRead == 0)
				{
					bDone = true;
				}
				else
				{
					// dumpo out what we have read
					Buffer[BytesRead] = 0;
					FullResult += UTF8_TO_TCHAR(Buffer);
				}
			}

			// safely tell game thread there's something there
			{
				FScopeLock ScopeLock(&ReplySyncObject);
				WebReply = FullResult.Len() ? FullResult : TEXT("Complete");
			}
		}

		// shut down the whole request
		InternetCloseHandle(RequestHandle);
		InternetCloseHandle(ConnectionHandle);
	}

	/**
	 * Perform any per-frame actions
	 */
	virtual void Tick(FLOAT DeltaSeconds)
	{
		FScopeLock ScopeLock(&ReplySyncObject);
		
		if (WebReply.Len() != 0)
		{
			// tell game thread we are done!
			OnWebRequestComplete(WebReply);

			// and clear it
			WebReply = TEXT("");
		}
	}


protected:

	/** Handle to the WinINet network */
	HINTERNET NetHandle;

	/** Handle to net connection */
	HINTERNET ConnectionHandle;

	/** Handle to outstanding request */
	HINTERNET RequestHandle;

	/** Thread protection for replies (which come in on other threads) */
	FCriticalSection ReplySyncObject;

	/** The contents of a reply from the web */
	FString WebReply;


	UBOOL HACK_IsAuthorized;
};


IMPLEMENT_CLASS(UFacebookWindows);

void AutoInitializeRegistrantsWinFacebook( INT& Lookup )
{
	UFacebookWindows::StaticClass();
}

/**
 * Callback function that is called whenever anything happens on the network
 */
void CALLBACK InternetStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext,
									 DWORD dwInternetStatus, LPVOID lpvStatusInformation,
									 DWORD dwStatusInformationLength)
{
	// are we done with the request?
	if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
	{
		// get the FB object from the callback data
		UFacebookWindows* FBIntegration = (UFacebookWindows*)dwContext;

		// it will process the results
		INTERNET_ASYNC_RESULT* Result = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;
		FBIntegration->ProcessReceivedData(Result);
	}
}
