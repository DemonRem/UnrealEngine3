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


#using <mscorlib.dll>
#include "StdAfx.h"
#include "ClientUserDelegate.h"

using namespace System::Runtime::InteropServices;
using namespace System::Collections;
using namespace System::Collections::Specialized;

using namespace p4dn;

ClientUserDelegate::ClientUserDelegate( gcroot<p4dn::ClientUser*> ManagedClientUser, gcroot<System::Text::Encoding*> encoding )
{
	mcu = ManagedClientUser;
	_encoding = encoding;
	//m_SaveForm = false;
}

ClientUserDelegate::~ClientUserDelegate() 
{  
	delete mcu;
}

//void ClientUserDelegate::SaveForm()
//{
//	m_SaveForm = true;
//}

void ClientUserDelegate::InputData( StrBuf *strbuf, ::Error* err )
{    

	//Build a System::String to pass to DotNet
	p4dn::Error* e = new p4dn::Error( err, _encoding );

	//if (m_SaveForm)
	//{
	//	Hashtable* sd;
	//	System::String* mSpecDef;
	//	mcu->InputForm(&sd, &mSpecDef, e);

	//	StrBuf sSpecDef;
	//	P4String::StringToStrBuf(&sSpecDef, mSpecDef, _encoding);

	//	::Error err;
	//	//Build our Spec objects
	//	::SpecDataTable	specData;
	//	::Spec		    spec(sSpecDef.Text(), "",  &err);
	//	
	//	//Spin the managed dictionary
	//	IEnumerator* myEnum = sd->Keys->GetEnumerator();
	//	while (myEnum->MoveNext())
	//	{
	//		// Convert the key and values
	//		System::String* key = static_cast<System::String*>(myEnum->Current);
	//		System::String* val = static_cast<System::String*>(sd->get_Item(key));

	//		StrBuf sKey; StrBuf sVal;
	//		P4String::StringToStrBuf(&sKey, key, _encoding);
	//		P4String::StringToStrBuf(&sVal, val, _encoding);
	//		
	//		// Create the unmanaged Dict entry
	//		specData.Dict()->SetVar(sKey, sVal);
	//	}


	//	spec.Format(&specData, strbuf);

	//}
	//else
	//{
		System::String* s; //P4String::StrPtrToString(strbuf, _encoding);
		mcu->InputData(&s, e );
		P4String::StringToStrBuf(strbuf, s, _encoding);
	//	
	//}

	e->Dispose();

}

void ClientUserDelegate::HandleError( ::Error *err )
{ 
    p4dn::Error* e = new p4dn::Error( err, _encoding);
    mcu->HandleError( e );
	e->Dispose();
}

void ClientUserDelegate::Message( ::Error *err )
{        
    p4dn::Error* e = new p4dn::Error( err , _encoding);
    mcu->Message( e );
	e->Dispose();
    
}

void ClientUserDelegate::OutputError( const_char *errBuf )
{
	System::String* s = P4String::CharArrToString(errBuf, _encoding);
    mcu->OutputError( s );    
}

void ClientUserDelegate::OutputInfo( char level, const_char *data )
{
    System::String* s = P4String::CharArrToString(data, _encoding);
    mcu->OutputInfo( level, s );    
}

void ClientUserDelegate::OutputBinary( const_char *data, int length )
{
	System::Byte b[] = new System::Byte[length];
	Marshal::Copy(IntPtr((void*)data), b, 0, length);
	mcu->OutputBinary( b );
}

void ClientUserDelegate::OutputText( const_char *data, int length )
{
	if (mcu->PrintBinary()) 
	{
		ClientUserDelegate::OutputBinary(data, length );
	}
	else
	{
		System::String *s = P4String::CharArrToString(data, _encoding);
		mcu->OutputText( s );
	}
}

/*
In this method, I deviate from the C++ api in form.
If this is a form, I call OutputForm, otherwise OutputStat.
This should be a lot easier in handling the Spec classes in managed code
and still provide full functionality.
*/
void ClientUserDelegate::OutputStat( StrDict *varList )
{

	System::Collections::Hashtable* dict; 
	::SpecDataTable specData;
	
	StrPtr* data = varList->GetVar("data");
	StrPtr* specdef = varList->GetVar("specdef");
		
	// I'm not sure what the intention of the specFormatted field is
	// can't we just look for the "data" field?
	// StrPtr* specformatted = varList->GetVar("specFormatted");

	StrDict* Dict;

	int i = 0;
	::StrRef var, val;   

	dict = new System::Collections::Hashtable();


	if (specdef)
	{
		// Send the SpecDef to the ClientUser so it can save it if it wants
		System::String* ManagedSpecDef = P4String::StrPtrToString(specdef, _encoding);
		mcu->SetSpecDef(ManagedSpecDef);
	}

	if (data)
	{
		// We have a form, not pre-parsed (i.e. pre-2005.2 server version)
		::Error e;
		::Spec s(specdef->Text(), "", &e);
		s.ParseNoValid(data->Text(), &specData, &e);
		Dict = specData.Dict();

	}
	else
	{
		// No form, just use the raw dictionary
		Dict = varList;
	}
	while (Dict->GetVar(i,var,val) != -0) 
	{   
		if (!( var == "specdef" || var == "func" || var == "specFormatted" ))
		{
			System::String* key = P4String::CharArrToString(var.Text(), _encoding);
			System::String* value = P4String::CharArrToString(val.Text(), _encoding);

			dict->Add( key, value );
			
		}
		i++;
	}


	mcu->OutputStat( dict );
}

void ClientUserDelegate::Prompt( const StrPtr& msg, StrBuf& rsp, int noEcho, ::Error *err )
{
    String* response;
	String* message = P4String::CharArrToString(msg.Text(), _encoding);
    bool bEcho = ( noEcho != 0 );
    p4dn::Error* e = new p4dn::Error( err, _encoding );

    mcu->Prompt( message, &response, bEcho, e );
    
   	P4String::StringToStrBuf(&rsp, response, _encoding);
	e->Dispose();    
}

void ClientUserDelegate::ErrorPause( char *errBuf, ::Error *err )
{
    System::String* s = P4String::CharArrToString(errBuf, _encoding);
    p4dn::Error* e = new p4dn::Error( err, _encoding );
    mcu->ErrorPause( s, e ); 
	e->Dispose();

}

void ClientUserDelegate::Edit( FileSys *f1, ::Error *err )
{    
    p4dn::Error* e = new p4dn::Error( err , _encoding);
    System::String* name = P4String::CharArrToString(f1->Name(), _encoding);
    System::IO::FileInfo* info = new System::IO::FileInfo( name );
    mcu->Edit( info, e );
	e->Dispose();
}

void ClientUserDelegate::Diff( FileSys *f1, FileSys *f2, int doPage, char *diffFlags, ::Error *e )
{
    p4dn::Error* err = new p4dn::Error( e , _encoding);
	System::String* sF1 = P4String::CharArrToString(f1->Name(), _encoding);
	System::IO::FileInfo* fsiF1 = new System::IO::FileInfo(sF1);

	System::String* sF2 = P4String::CharArrToString(f2->Name(), _encoding);
	System::IO::FileInfo* fsiF2 = new System::IO::FileInfo(sF2);

	System::String* sdiffFlags = P4String::CharArrToString(diffFlags, _encoding);

	mcu->Diff(fsiF1, fsiF2, doPage, sdiffFlags, err);
	
	err->Dispose();
}

void ClientUserDelegate::Merge( FileSys *base, FileSys *leg1, FileSys *leg2, FileSys *result, ::Error *e )
{
	if (mcu->RunMergeTool)
	{
		ClientUser::Merge(base, leg1, leg2, result, e);
		StrBuf s;
		e->Fmt(&s);
		Console::WriteLine(s.Text());
	}
	else
	{
		p4dn::Error* err = new p4dn::Error( e , _encoding);
		System::String* sbase = P4String::CharArrToString(base->Name(), _encoding);
		System::String* sleg1 = P4String::CharArrToString(leg1->Name(), _encoding);
		System::String* sleg2 = P4String::CharArrToString(leg2->Name(), _encoding);
		System::String* sresult = P4String::CharArrToString(result->Name(), _encoding);
		
		System::IO::FileInfo* fbase = new System::IO::FileInfo(sbase);
		System::IO::FileInfo* fleg1 = new System::IO::FileInfo(sleg1);
		System::IO::FileInfo* fleg2 = new System::IO::FileInfo(sleg2);
		System::IO::FileInfo* fresult = new System::IO::FileInfo(sresult);
	    
		mcu->Merge(fbase, fleg1, fleg2, fresult, err);
		
		err->Dispose();
	}
}


void ClientUserDelegate::Help( const_char *const *help )
{
    System::String* s = P4String::CharArrToString(*help, _encoding);
    mcu->Help( s );    
}

FileSys* ClientUserDelegate::File( FileSysType type )
{        
    return FileSys::Create( type );    
}

void ClientUserDelegate::Finished() 
{    
    mcu->Finished();    
}