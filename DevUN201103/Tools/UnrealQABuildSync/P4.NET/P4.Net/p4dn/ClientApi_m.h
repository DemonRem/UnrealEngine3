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


#pragma once
#using <mscorlib.dll>

#include "Error_m.h"
#include "ClientUser_m.h"
#include "KeepAlive_m.h"
#include "ClientUserDelegate.h"
#include "Spec_m.h"

using namespace System::Runtime::InteropServices;

namespace p4dn {

	__gc public class ClientApi : public System::IDisposable 
    {

    public:

        ClientApi();
        ~ClientApi();

        void SetTrans( int output, int content, int fnames, int dialog );
		void SetProtocol( System::String *p, System::String *v );
        void SetProtocolV( System::String *p );
        String*	GetProtocol( System::String *v );

        void Init( p4dn::Error *e );
        void Run(System::String *func, p4dn::ClientUser *ui );
		//void SaveForm(System::String *func, p4dn::ClientUser *ui );
		void Dispose();
        int Final(p4dn::Error *e );
        int	Dropped();
		p4dn::Error* CreateError();
		p4dn::Spec*  CreateSpec(System::String* specDef);

        //void RunTag( String* func, p4dn::ClientUser *ui );
        //void WaitTag( p4dn::ClientUser *ui );

        void SetCharset( System::String* c );
        void SetClient( System::String* c );
        void SetCwd( System::String* c );
        void SetHost( System::String* c );
        void SetLanguage( System::String* c );
        void SetPassword( System::String* c );
        void SetPort( System::String* c );
		void SetProg( System::String* c );
		void SetVersion( System::String* c );
		void SetTicketFile( System::String* c );
        void SetUser( System::String* c );
        void SetArgv( System::String* args[] );
        void SetBreak( p4dn::KeepAlive *keepAlive );

		void SetMaxResults(int maxResults);
		void SetMaxScanRows(int maxScanRows);
		void SetMaxLockTime(int maxLockTime);

        void DefineCharset( System::String* c, p4dn::Error* e );
        void DefineClient( System::String* c, p4dn::Error* e );
        void DefineHost( System::String* c, p4dn::Error* e );
        void DefineLanguage( System::String* c, p4dn::Error* e );
        void DefinePassword( System::String* c, p4dn::Error* e );
        void DefinePort( System::String* c, p4dn::Error* e );
        void DefineUser( System::String* c, p4dn::Error* e );        

        __property System::String* get_Charset()  { return _clientApi->GetCharset().Text(); }
        __property System::String* get_Client()   { return _clientApi->GetClient().Text(); }
        __property System::String* get_Cwd()      { return _clientApi->GetCwd().Text(); }
        __property System::String* get_Host()     { return _clientApi->GetHost().Text(); }
        __property System::String* get_Language() { return _clientApi->GetLanguage().Text(); }
        __property System::String* get_Os()       { return _clientApi->GetOs().Text(); }
        __property System::String* get_Password() { return _clientApi->GetPassword().Text(); }
        __property System::String* get_Port()     { return _clientApi->GetPort().Text(); }
        __property System::String* get_User()     { return _clientApi->GetUser().Text(); }

		__property System::Text::Encoding* get_Encoding()     { return _encoding; }

    private:
        ::ClientApi* _clientApi;
		::ClientApi* getClientApi();
		System::Text::Encoding* _encoding;
		void CleanUp();
		bool _Disposed;

    private:
        //================================================================
        // 
        // class for callbacks into the KeepAlive interfaces
        //
		__nogc class KeepAliveDelegate : public ::KeepAlive 
		{
        private:
            // handle to managed KeepAlive class
            //int _keepAliveHandle;        
			gcroot<p4dn::KeepAlive*> _KeepAlive;
        public:
            KeepAliveDelegate(gcroot<p4dn::KeepAlive*> mKeepAlive) 
			{
				 _KeepAlive = mKeepAlive;
			}
            ~KeepAliveDelegate()
			{
				// Free the GC refererence
				delete _KeepAlive;
			}
            
			int IsAlive() {
                bool b = _KeepAlive->IsAlive();
                return ( b ? 1 : 0 );
            }
        };
        KeepAliveDelegate* _keepAliveDelegate;
    };
}
