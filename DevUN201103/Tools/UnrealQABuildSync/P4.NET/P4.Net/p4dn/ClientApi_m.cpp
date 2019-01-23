/*
 * P4.Net *
Copyright (c) 2007 Shawn Hladky

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


#include "StdAfx.h"
#using <mscorlib.dll>
#include "clientapi_m.h"
#include "i18napi.h"

using namespace System::Runtime::InteropServices;

p4dn::ClientApi::ClientApi()
{
	_Disposed = false;
    _clientApi = new ::ClientApi();
    _keepAliveDelegate = NULL;
	
	// default is the OS default encoding.
	_encoding = System::Text::Encoding::Default;
}

p4dn::ClientApi::~ClientApi()
{
	CleanUp();
}
::ClientApi* p4dn::ClientApi::getClientApi()
{
	// Oops someone called dispose too early!!!
	if (_Disposed)
	{
		//Have to throw an error here.
		throw new System::Exception("Error! ClientApi has been Disposed!");
	}
	return _clientApi;
}
void p4dn::ClientApi::CleanUp()
{
	if (_clientApi != NULL) delete _clientApi;
	if (_keepAliveDelegate != NULL) delete _keepAliveDelegate;
	_clientApi = NULL;
	_keepAliveDelegate = NULL;

}

void p4dn::ClientApi::Dispose()
{
	System::GC::SuppressFinalize(this);
	CleanUp();
	_Disposed = true;
}

void p4dn::ClientApi::SetMaxResults(int maxResults)
{
	if( maxResults  )	getClientApi()->SetVar( "maxResults",  maxResults  );
}
void p4dn::ClientApi::SetMaxScanRows(int maxScanRows)
{
	if( maxScanRows )	getClientApi()->SetVar( "maxScanRows", maxScanRows );
}
void p4dn::ClientApi::SetMaxLockTime(int maxLockTime)
{
	if( maxLockTime )	getClientApi()->SetVar( "maxLockTime", maxLockTime );
}
void p4dn::ClientApi::SetTrans( int output, int content, int fnames, int dialog )
{
    getClientApi()->SetTrans( output, content, fnames, dialog );
}

void p4dn::ClientApi::SetProtocol( String *p, String *v )
{   
	StrBuf str_p; StrBuf str_v;
	P4String::StringToStrBuf(&str_p, p, _encoding);
    P4String::StringToStrBuf(&str_v, v , _encoding);
    getClientApi()->SetProtocol( str_p.Text(), str_v.Text() );    
}

void p4dn::ClientApi::SetArgv( String* args[] )
{  
	StrBuf s;
	for (int i = 0; i < args->Length; ++i) {
		s.Clear();
		P4String::StringToStrBuf(&s, args[i], _encoding);
        getClientApi()->SetVar( "", &s );
    }
}


void p4dn::ClientApi::SetProtocolV( String* p ) 
{ 
	StrBuf s;
	P4String::StringToStrBuf(&s, p, _encoding);
    getClientApi()->SetProtocolV( s.Text() );
}


String* p4dn::ClientApi::GetProtocol( String *v ) 
{ 
    StrBuf s;
	P4String::StringToStrBuf(&s, v, _encoding);
    StrPtr* ptr = getClientApi()->GetProtocol( s.Text() );
	String* s2 = P4String::StrPtrToString(ptr, _encoding);
    return s2;
}

 void p4dn::ClientApi::Init( p4dn::Error* e ) 
 { 
	if(getClientApi()->GetCharset().Length() > 0)
	{
		// unicode server use UTF-8
		_encoding = new System::Text::UTF8Encoding();

		// set the translations (use UTF-8 for everything but content).
		CharSetApi::CharSet content = CharSetApi::Lookup(getClientApi()->GetCharset().Text());
		getClientApi()->SetTrans(CharSetApi::CharSet::UTF_8, content, 
			CharSetApi::CharSet::UTF_8, CharSetApi::CharSet::UTF_8);
	}
	else
	{
		// To use the OS default encoding.
		_encoding = System::Text::Encoding::Default;
	}
    getClientApi()->Init( e->get_InternalError() );
 }

 void p4dn::ClientApi::Run( String* func, p4dn::ClientUser *ui ) 
 {
     StrBuf cmd;
	 P4String::StringToStrBuf(&cmd, func, _encoding);
	 ClientUserDelegate cud = ClientUserDelegate(ui, _encoding);
     getClientApi()->Run(cmd.Text(), &cud);              
 }

 p4dn::Error* p4dn::ClientApi::CreateError()
 {
	 return new p4dn::Error(_encoding);
 }

 p4dn::Spec*  p4dn::ClientApi::CreateSpec(System::String* specDef)
 {
	 return new p4dn::Spec(specDef, _encoding);
 }

 int p4dn::ClientApi::Final( p4dn::Error *e ) 
 {      
    return getClientApi()->Final( e->get_InternalError() );
 }

 int p4dn::ClientApi::Dropped() 
 { 
     return getClientApi()->Dropped();
 }

 //
 // These functions are disabled.  There will be a lot of work to enable
 // asynchronous execution and ensure that all of references are released
 // so managed objects are garbage collected
 //
 //void p4dn::ClientApi::RunTag( String* func, p4dn::ClientUser *ui ) 
 //{      
 //   char* f = (char *)(void *) Marshal::StringToHGlobalAnsi( func );  
	//ClientUserDelegate __nogc* cud = new ClientUserDelegate(ui);
 //   _clientApi->RunTag( f, cud);
	//delete cud;
 //   Marshal::FreeHGlobal( f );
 //}


 //void p4dn::ClientApi::WaitTag( p4dn::ClientUser *ui )
 //{ 
	//ClientUserDelegate __nogc* cud = new ClientUserDelegate(ui);
 //   _clientApi->WaitTag( cud );
	//delete cud;
 //}

 void p4dn::ClientApi::SetCharset( String* c ) 
 { 
     StrBuf f;
	 P4String::StringToStrBuf(&f, c, _encoding);
     getClientApi()->SetCharset( f.Text() );
 }

 void p4dn::ClientApi::SetClient( String* c ) 
 { 
     StrBuf f;
	 P4String::StringToStrBuf(&f, c, _encoding);
     getClientApi()->SetClient( f.Text() );
 }

 void p4dn::ClientApi::SetCwd( String* c ) 
 {
     StrBuf f;
	 P4String::StringToStrBuf(&f, c, _encoding);
     getClientApi()->SetCwd( f.Text() );
 }

 void p4dn::ClientApi::SetHost( String* c ) 
 {
     StrBuf f;
	 P4String::StringToStrBuf(&f, c, _encoding);
     getClientApi()->SetHost( f.Text() );
 }

 void p4dn::ClientApi::SetLanguage( String* c ) 
 {
     StrBuf f;
	 P4String::StringToStrBuf(&f, c, _encoding);
     getClientApi()->SetLanguage( f.Text() );
 }

 void p4dn::ClientApi::SetPassword( String* c ) 
 {
	StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	getClientApi()->SetPassword( f.Text() );
 }

 void p4dn::ClientApi::SetPort( String* c ) 
 {
	StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	getClientApi()->SetPort( f.Text() );
 }

 void p4dn::ClientApi::SetProg( String* c ) 
 {
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	 getClientApi()->SetProg( f.Text() );
 }

 void p4dn::ClientApi::SetVersion( String* c ) 
 {
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	 getClientApi()->SetVersion( f.Text() );
 }

 void p4dn::ClientApi::SetTicketFile( String* c ) 
 {
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	getClientApi()->SetTicketFile( f.Text() );
 }

 void p4dn::ClientApi::SetUser( String* c ) 
 {
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
	 getClientApi()->SetUser( f.Text() );
 }

 void p4dn::ClientApi::SetBreak( p4dn::KeepAlive *keepAlive )
 {
     if (_keepAliveDelegate != NULL) {
         return;
     }
     _keepAliveDelegate = new KeepAliveDelegate( keepAlive );
	 getClientApi()->SetBreak(_keepAliveDelegate);
 }

 void p4dn::ClientApi::DefineCharset( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefineCharset( f.Text(), e->get_InternalError() );

 }

 void p4dn::ClientApi::DefineClient( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefineClient( f.Text(), e->get_InternalError() );
 }

 void p4dn::ClientApi::DefineHost( String* c, p4dn::Error* e ) 
 {
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefineHost( f.Text(), e->get_InternalError() );
 }

 void p4dn::ClientApi::DefineLanguage( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefineLanguage( f.Text(), e->get_InternalError() );
 }

 void p4dn::ClientApi::DefinePassword( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefinePassword( f.Text(), e->get_InternalError() );
 }

 void p4dn::ClientApi::DefinePort( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefinePort( f.Text(), e->get_InternalError() );
 }

 void p4dn::ClientApi::DefineUser( String* c, p4dn::Error* e ) 
 { 
    StrBuf f;
	P4String::StringToStrBuf(&f, c, _encoding);    
     getClientApi()->DefineUser(f.Text(), e->get_InternalError() );
 }
