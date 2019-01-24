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
#include "Error_m.h"
#include <stdlib.h>

p4dn::Error::Error( ::Error* e, System::Text::Encoding* encoding)
{
	_encoding = encoding;
	_err = e;
    _requiresFree = false;
	_Disposed = false;
}

p4dn::Error::Error( System::Text::Encoding* encoding )
{   
	_err = new ::Error();
	_encoding = encoding;
	_requiresFree = true;
	_Disposed = false;
}

p4dn::Error::~Error( void )
{
	CleanUp();
}
void p4dn::Error::Dispose()
{
	System::GC::SuppressFinalize(this);
	_Disposed = true;
	CleanUp();
}
void p4dn::Error::CleanUp()
{
	if (_requiresFree && _err != NULL ) delete _err;
	_err = NULL;
	_requiresFree = false;
}

::Error* p4dn::Error::get_InternalError()
{
	// Silly programer called Dispose() too early
	// I will be nice to them and reset the object
	if(_Disposed)
	{
		_err = new ::Error();
		_Disposed = false;
		System::GC::ReRegisterForFinalize(this);
	}
	return _err;
}

 void p4dn::Error::Clear()
 {
    get_InternalError()->Clear();
 }

 bool p4dn::Error::Test() 
 {
    return get_InternalError()->Test();
 }

 bool p4dn::Error::IsInfo()
 {
     return get_InternalError()->GetSeverity() == E_INFO;   
 }

 bool p4dn::Error::IsWarning()
 {
     return get_InternalError()->GetSeverity() == E_WARN;     
 }

 bool p4dn::Error::IsFailed()
 {
     return get_InternalError()->GetSeverity() == E_FAILED;     
 }
 

 bool p4dn::Error::IsFatal()
 {
     return get_InternalError()->GetSeverity() == E_FATAL;     
 }

 System::String* p4dn::Error::Fmt()
 {     
     ::StrBuf buf;
     get_InternalError()->Fmt( &buf ); 
	 System::String* temp = P4String::StrPtrToString(&buf, _encoding);
     return temp->Trim();
 }

