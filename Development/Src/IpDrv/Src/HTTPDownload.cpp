/*=============================================================================
	HTTPDownload.cpp: HTTP downloading support functions
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnIpDrv.h"
#include "HTTPDownload.h"

#if !XBOX

/*-----------------------------------------------------------------------------
	UHTTPDownload implementation.
-----------------------------------------------------------------------------*/
UHTTPDownload::UHTTPDownload()
{
	// Init.
	ServerSocket = NULL;
	ResolveInfo = NULL;
	HTTPState = HTTP_Initialized;
	bDownloadSendsFileSizeInData = FALSE;

}

void UHTTPDownload::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << ReceivedData << Headers << DownloadURL;
}

void UHTTPDownload::StaticConstructor()
{
	// Config.
	new(GetClass(),TEXT("ProxyServerHost"),		RF_Public)UStrProperty(CPP_PROPERTY(ProxyServerHost		), TEXT("Settings"), CPF_Config );
	new(GetClass(),TEXT("ProxyServerPort"),		RF_Public)UIntProperty(CPP_PROPERTY(ProxyServerPort		), TEXT("Settings"), CPF_Config );
	new(GetClass(),TEXT("RedirectToURL"),		RF_Public)UStrProperty(CPP_PROPERTY(DownloadParams		), TEXT("Settings"), CPF_Config );
	new(GetClass(),TEXT("UseCompression"),		RF_Public)UBoolProperty(CPP_PROPERTY(UseCompression		), TEXT("Settings"), CPF_Config );
	new(GetClass(),TEXT("MaxRedirection"),		RF_Public)UBoolProperty(CPP_PROPERTY(MaxRedirection		), TEXT("Settings"), CPF_Config );
}

static FString FormatFilename( const TCHAR* InFilename )
{
	FString Result;
	for( INT i=0;InFilename[i];i++ )
	{
		if( (InFilename[i]>='a'&&InFilename[i]<='z') || 
			(InFilename[i]>='A'&&InFilename[i]<='Z') || 
			(InFilename[i]>='0'&&InFilename[i]<='9') || 
			InFilename[i]=='/' || InFilename[i]=='?' || 
			InFilename[i]=='&' || InFilename[i]=='.' || 
			InFilename[i]=='=' || InFilename[i]=='-'
		  )
			Result = Result + FString::Printf(TEXT("%c"),InFilename[i]);
		else
			Result = Result + FString::Printf(TEXT("%%%02x"),InFilename[i]);
	}
	return Result;
}

void UHTTPDownload::ReceiveFile( UNetConnection* InConnection, INT InPackageIndex, const TCHAR *Params, UBOOL InCompression )
{
	UDownload::ReceiveFile( InConnection, InPackageIndex, Params, InCompression);

	// we need a params string
	if( !*Params )
	{
		return;
	}

	// remember if it is compressed
	IsCompressed = InCompression;

	// HTTP 301/302 support
	CurRedirectLevel = 0;
	if (MaxRedirection < 1)
	{
		MaxRedirection = 5; // default to 5
	}

	FPackageInfo& Info = Connection->PackageMap->List( PackageIndex );
	FURL Base(NULL, TEXT(""), TRAVEL_Absolute);
	Base.Port = 80;

	FString File = Info.PackageName.ToString() + TEXT(".") + Info.Extension;

	// this adds an extension unlike normal with no extension
	if( IsCompressed )
	{
		File = File + COMPRESSED_EXTENSION;
	}

	// patch up any URL variables with their values
	FString StringURL = Params;
	StringURL = StringURL.Replace(TEXT("%guid%"), *Info.Guid.String());

	StringURL = StringURL.Replace(TEXT("%file%"), *File);
	StringURL = StringURL.Replace(TEXT("%lcfile%"), *File.ToLower());
	StringURL = StringURL.Replace(TEXT("%ucfile%"), *File.ToUpper());

	StringURL = StringURL.Replace(TEXT("%ext%"), *Info.Extension);
	StringURL = StringURL.Replace(TEXT("%lcext%"), *Info.Extension.ToLower());
	StringURL = StringURL.Replace(TEXT("%ucext%"), *Info.Extension.ToUpper());


	// if we didn't substitute anything, just take the filename on the end.
	if (StringURL == Params)
	{
		StringURL = StringURL + File;
	}

	DownloadURL = FURL(&Base, *StringURL, TRAVEL_Relative);


	// use the proxy server if specified, otherwise, just the server in the URL
	const TCHAR* HostName = ProxyServerHost.Len() ? *ProxyServerHost : *DownloadURL.Host;

	// try to make a valid IP addre from the string
	UBOOL bNoResolvingNeeded;
	ServerAddr.SetIp(HostName, bNoResolvingNeeded);

	// if the IP address was valid we don't need to resolve
	if (bNoResolvingNeeded)
	{
		HTTPState = HTTP_Resolved;	
		debugf(TEXT("Decoded IP %s (%s)"), HostName, *ServerAddr.ToString(FALSE));
	}
	else
	{
		ResolveInfo = new FResolveInfo(HostName);
		HTTPState = HTTP_Resolving;
	}

	// Show progress while connecting.
	// @todo: This is not really a good way to show progress, as the function does too much other stuff
	//ReceiveData( NULL, 0 );
}

/**
 * Handle resolving an address
 */
void UHTTPDownload::StateResolving()
{
	check(ResolveInfo);
	// If destination address isn't resolved yet, send nowhere.
	if (!ResolveInfo->Resolved())
	{
		// Host name still resolving.
		return;
	}
	else if( ResolveInfo->GetError() )
	{
		// Host name resolution just now failed.
		DownloadError( *FString::Printf( *LocalizeError( TEXT("InvalidUrl"), TEXT("Engine") ), (*(*ProxyServerHost)) ? *ProxyServerHost : *DownloadURL.Host ) );
		HTTPState  = HTTP_Closed;
		delete ResolveInfo;
		ResolveInfo = NULL;
	}
	else
	{
		// Host name resolution just now succeeded.
		debugf( TEXT("Resolved %s (%s)"), *ResolveInfo->GetHostName(), *ResolveInfo->GetAddr().ToString(FALSE));

		ServerAddr	= ResolveInfo->GetAddr();
		delete ResolveInfo;
		ResolveInfo = NULL;
		HTTPState = HTTP_Resolved;
	}
}

/**
 * Handle when the address was resolved
 */
void UHTTPDownload::StateResolved()
{
	// use the port of the proxy server if it was specified
	ServerAddr.SetPort(ProxyServerHost.Len() ? ProxyServerPort : DownloadURL.Port);

	// Connect	
	ServerSocket = GSocketSubsystem->CreateStreamSocket();
	if (ServerSocket == NULL)
	{
		DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
		HTTPState  = HTTP_Closed;
		return;
	}

	// set some socket options
	ServerSocket->SetReuseAddr(TRUE);
	// Don't do this; breaks on Linux 2.4+. --ryan.
	//ServerSocket->SetLinger(TRUE);
	ServerSocket->SetNonBlocking(TRUE);

	if (ServerSocket->Connect(ServerAddr) == FALSE)
	{
		debugf( NAME_DevNet, TEXT("HTTPDownload: Connect() failed [%s]"), GSocketSubsystem->GetSocketError() );
		DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
		HTTPState  = HTTP_Closed;
		return;
	}

	ConnectStartTime = appSeconds();
	HTTPState  = HTTP_Connecting;
}

/**
 * Handle connecting to a server
 */
void UHTTPDownload::StateConnecting()
{
	// get state of connection
	ESocketConnectionState State = ServerSocket->GetConnectionState();

	// we've connected!
	if (State == SCS_Connected)
	{
		FString GetString = FormatFilename(*(FString(TEXT("/")) + DownloadURL.Map));

		// @todo: Reimplement OptionString(), as this won't work with proxy!! (has the host in it)
		// GetString = (ProxyServerHost.Len() > 0) ? *FormatFilename(*DownloadURL.String()) : *(FString(TEXT("/"))+FormatFilename(*(DownloadURL.Map+DownloadURL.OptionString()))),

		// make standard GET request
		FString Request = FString::Printf(TEXT("GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Unreal\r\nConnection: close\r\n\r\n"), *GetString, *ServerAddr.ToString(TRUE));

		// send request
		INT BytesSent;
		if (ServerSocket->Send((BYTE*)TCHAR_TO_ANSI(*Request), Request.Len(), BytesSent) == FALSE)
		{
			debugf( NAME_DevNet, TEXT("HTTPDownload: initial send failed") );
			DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
			HTTPState  = HTTP_Closed;
		}

		// now we're waiting to get all the headers
		HTTPState = HTTP_ReceivingHeader;
	}
	else if (State == SCS_NotConnected)
	{
		// timeout after 30 seconds
		// @todo: Make this an ini option!
		if (appSeconds() - ConnectStartTime > 30)
		{
			debugf( NAME_DevNet, TEXT("HTTPDownload: connection timed out") );
			DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
			HTTPState  = HTTP_Closed;
		}
	}
	else if (State == SCS_ConnectionError)
	{
		debugf( NAME_DevNet, TEXT("HTTPDownload: Connection failed") );
		DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
		HTTPState  = HTTP_Closed;
		return;
	}
}

/**
 * Handle receiving the header after connecting
 */
void UHTTPDownload::StateReceivingHeader()
{
	// get some header data
	FetchData();

	// parse it
	for (INT HeaderOffset = 0; HeaderOffset < ReceivedData.Num(); HeaderOffset++)
	{
		// look for CR
		if (ReceivedData(HeaderOffset) == 13 || ReceivedData(HeaderOffset) == 10)
		{	
			// we are at the end of the header if the first char is a CR
			UBOOL EOH = HeaderOffset == 0;
			// terminate string up to this point
			ReceivedData(HeaderOffset) = 0;

			// add the header string up to this CR
			Headers(Headers.AddZeroed()) = ANSI_TO_TCHAR((ANSICHAR*)ReceivedData.GetData());

			// if the next char is another CR, skip over it
			if (HeaderOffset<ReceivedData.Num() - 1 && (ReceivedData(HeaderOffset+1)==13 || ReceivedData(HeaderOffset+1)==10))
			{
				HeaderOffset++;					
			}

			// remove all data up to and including the CR
			ReceivedData.Remove(0, HeaderOffset + 1);

			// are we at the end of all headers
			if (EOH)
			{
				UBOOL bNeedRedir = false;

				// get the number code from the header
				FString Code = Headers(0).Mid(Headers(0).InStr(TEXT(" ")), 5);

				if (Code == TEXT(" 400 "))
				{
					debugf( NAME_DevNet, TEXT("HTTPDownload: Got code 400!") );
					DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
					HTTPState  = HTTP_Closed;
					return;
				}

				// received a redirect header, want "Location:" header and restart
				if ((Code == TEXT(" 301 ") || Code == TEXT(" 302 "))
					&& (CurRedirectLevel < MaxRedirection))
				{
					bNeedRedir = true;
					CurRedirectLevel++;
				}
				// check we got a 200 OK.
				else if (Code != TEXT(" 200 "))
				{
					DownloadError(*FString::Printf(*LocalizeError(TEXT("InvalidUrl"), TEXT("Engine")), *DownloadURL.String()));
					HTTPState  = HTTP_Closed;
					return;
				}
				// loop over the headers we have
				for (INT HeaderIndex = 0; HeaderIndex < Headers.Num(); HeaderIndex++)
				{
					FString& Header = Headers(HeaderIndex);

					if (Header.Left(16) == TEXT("CONTENT-LENGTH: ") )
					{								
						// parse the download size
						FileSize = appAtoi(*Header.Mid(16));

// we no longer get size from server, due to seekfree issues. Will it ever be needed?
/*
						// if it failed, use the size of the package we go from the server
						if (FileSize <= 0)
						{
							FileSize = Info->FileSize;
						}

						// validate the size matches the server size (if not compressed)
						if (!IsCompressed && FileSize != Info->FileSize)
						{
							DownloadError(*LocalizeError( TEXT("NetSize"), TEXT("Engine")) );
							HTTPState  = HTTP_Closed;
							return;
						}
*/
						if (FileSize <= 0)
						{
							DownloadError(*LocalizeError( TEXT("NetSize"), TEXT("Engine")) );
							HTTPState  = HTTP_Closed;
							return;
						}
					}
					// support for HTTP redirection
					else if( Header.Left(10) == TEXT("LOCATION: ") && bNeedRedir)
					{
						// generate a new URL for the redirection
						FURL Base(&DownloadURL, TEXT(""), TRAVEL_Relative);
						Base.Port = 80;
						DownloadURL = FURL(&Base, *(Header.Mid(10)), TRAVEL_Relative);
						debugf(TEXT("Download redirection to: %s"), *DownloadURL.String());

						// close the connection to the server
						delete ServerSocket;
						ServerSocket = NULL;
						// empty out all data
						ReceivedData.Empty();
						Headers.Empty();

						// use the proxy server if specified, otherwise, just the server in the URL
						const TCHAR* HostName = ProxyServerHost.Len() ? *ProxyServerHost : *DownloadURL.Host;

						UBOOL bNoResolvingNeeded;
						// set the address in the ServerAddr
						ServerAddr.SetIp(HostName, bNoResolvingNeeded);

						// if the IP address was valid we don't need to resolve
						if (bNoResolvingNeeded)
						{
							HTTPState = HTTP_Resolved;	
							debugf(TEXT("Decoded IP %s (%s)"), HostName, *ServerAddr.ToString(FALSE));
						}
						else
						{
							ResolveInfo = new FResolveInfo(HostName);
							HTTPState = HTTP_Resolving;
						}

						return;
					}
				}

				// if we didn't receive a redirection location, but needed one, error out
				if (bNeedRedir) 
				{
					DownloadError(*FString::Printf(*LocalizeError(TEXT("InvalidUrl"), TEXT("Engine")), *DownloadURL.String()));
					HTTPState  = HTTP_Closed;
					return;
				}

				// at this point, we're parsed all the headers
				HTTPState = HTTP_ReceivingData;
				// so break out of the loop parsing the header data
				break;
			}

			// fetch more header data
			FetchData();

			// and restart the loop
			HeaderOffset = -1;
		}
	}
}

/**
 * Handle receiving data
 */
void UHTTPDownload::StateReceivingData()
{
	// handle receiving file data
	while (ReceivedData.Num() || FetchData())
	{
		if (ReceivedData.Num() > 0)
		{
			// clamp amount of received data up to the size left that we want to download
			INT Count = Transfered + ReceivedData.Num() > FileSize ? FileSize - Transfered : ReceivedData.Num(); //!! ?? this doesn't look correct for compressed files.
			if (Count > 0)
			{
				// process the data
				ReceiveData(ReceivedData.GetTypedData(), Count);
			}

			// if no more to receive, then go on to the closed state
			if (Transfered == FileSize)
			{
				HTTPState = HTTP_Closed;
			}

			// empty the received data
			ReceivedData.Empty();
		}
	}

}

/**
 * Handle the session being closed
 */
void UHTTPDownload::StateClosed()
{
	// notify that we have finished downloading the package
	DownloadDone();

}

/**
 * Tick the connection
 */
void UHTTPDownload::Tick(void)
{
	// tick the state machine
	switch (HTTPState)
	{
		case HTTP_Resolving:
			StateResolving();
			break;

		case HTTP_Resolved:
			StateResolved();
			break;

		case HTTP_Connecting:
			StateConnecting();
			break;

		case HTTP_ReceivingHeader:
			StateReceivingHeader();
			break;

		case HTTP_ReceivingData:
			StateReceivingData();
			break;

		case HTTP_Closed:
			StateClosed();
			break;
	}
}

/**
 * Receive some data from the connection, and store it in a buffer
 */
UBOOL UHTTPDownload::FetchData()
{
	BYTE Buffer[1024];
	INT BytesRead = 0;
	UBOOL bReceivedData = FALSE;

	if (ServerSocket->GetConnectionState() == SCS_NotConnected)
	{
		// end of the stream (close)
		HTTPState = HTTP_Closed;
	}
	else
	{
		if (ServerSocket->Recv(Buffer, ARRAY_COUNT(Buffer), BytesRead) == FALSE)
		{
			// maybe this should be GetConnectionState()? 
			if (GSocketSubsystem->GetLastErrorCode() != SE_EWOULDBLOCK)
			{
				debugf( NAME_DevNet, TEXT("HTTPDownload: Receive() failed") );
				DownloadError( *LocalizeError( TEXT("ConnectionFailed"), TEXT("Engine") ) );
				HTTPState  = HTTP_Closed;
			}
		}
		else if (BytesRead > 0)
		{
			// append the data that was read to the buffer
			appMemcpy(&ReceivedData(ReceivedData.Add(BytesRead)), Buffer, BytesRead);
			bReceivedData = TRUE;
		}
	}

	return bReceivedData;
}

UBOOL UHTTPDownload::TrySkipFile()
{
	if( Super::TrySkipFile() )
	{
		HTTPState = HTTP_Closed;
		Connection->Logf( TEXT("SKIP GUID=%s"), *Info->Guid.String() );
		return 1;
	}
	return 0;
}

void UHTTPDownload::FinishDestroy()
{
	delete ServerSocket;
	ServerSocket = NULL;

	Super::FinishDestroy();
}

#endif

IMPLEMENT_CLASS(UHTTPDownload)
