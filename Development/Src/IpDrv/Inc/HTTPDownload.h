/*=============================================================================
	HTTPDownload.h: HTTP downloading support functions
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef UHTTPDownload_H
#define UHTTPDownload_H


enum EHTTPState
{
	HTTP_Initialized		=0,
	HTTP_Resolving			=1,
	HTTP_Resolved			=2,
	HTTP_Connecting			=3,
	HTTP_ReceivingHeader	=4,
	HTTP_ReceivingData		=5,
	HTTP_Closed				=6
};

class UHTTPDownload : public UDownload
{
	DECLARE_CLASS(UHTTPDownload,UDownload,CLASS_Transient|CLASS_Config|CLASS_Intrinsic,IpDrv);

#if !XBOX
	// Config.
	FStringNoInit	ProxyServerHost;
	INT				ProxyServerPort;
	INT				MaxRedirection;
	INT				CurRedirectLevel;

	// Variables.
	BYTE			HTTPState;
	FSocket*		ServerSocket;
	FInternetIpAddr LocalAddr;
	FInternetIpAddr ServerAddr;
	FResolveInfo*	ResolveInfo;
	FURL			DownloadURL;
	TArray<BYTE>	ReceivedData;
	TArray<FString>	Headers;
	DOUBLE			ConnectStartTime;

	// Constructors.
	void StaticConstructor();
	UHTTPDownload();

	// UObject interface.
	void FinishDestroy();
	void Serialize( FArchive& Ar );

	// UDownload Interface.
	virtual void ReceiveFile( UNetConnection* InConnection, INT PackageIndex, const TCHAR *Params, UBOOL InCompression );
	UBOOL TrySkipFile();

	/**
	 * Tick the connection
	 */
	void Tick(void);

	/**
	 * Receive some data from the connection, and store it in a buffer
	 */
	UBOOL FetchData();

private:
	/**
	 * Handle resolving an address
	 */
	void StateResolving();

	/**
	 * Handle when the address was resolved
	 */
	void StateResolved();

	/**
	 * Handle connecting to a server
	 */
	void StateConnecting();

	/**
	 * Handle receiving the header after connecting
	 */
	void StateReceivingHeader();

	/**
	 * Handle receiving data
	 */
	void StateReceivingData();

	/**
	 * Handle the session being closed
	 */
	void StateClosed();

#endif
};

#endif
