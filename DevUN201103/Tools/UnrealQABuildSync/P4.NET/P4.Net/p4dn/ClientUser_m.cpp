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
#include "clientuser_m.h"

using namespace System::Runtime::InteropServices;


p4dn::ClientUser::ClientUser()
{        
}


void p4dn::ClientUser::InputData( String** s, p4dn::Error* err )
{     
}
 
void p4dn::ClientUser::InputForm(Hashtable** varList, String** specdef, p4dn::Error* err)
{     
}

void p4dn::ClientUser::HandleError( p4dn::Error* err )
{
  
}


void p4dn::ClientUser::Message( p4dn::Error* err )
{                
    
}

void p4dn::ClientUser::OutputError( String* s )
{        
   
}

void p4dn::ClientUser::SetSpecDef(String* SpecDef)
{        
   
}
void p4dn::ClientUser::OutputInfo( Char level, String* s )
{     
    
}


void p4dn::ClientUser::OutputBinary( System::Byte b[] )
{

}

void p4dn::ClientUser::OutputText( String *s )
{    
   
}

void p4dn::ClientUser::OutputStat( Hashtable* varList )
{ 
}


void p4dn::ClientUser::Prompt( const String* msg, 
                               String** rsp, 
                               bool noEcho, 
                               p4dn::Error* err )
{
    
   
}

void p4dn::ClientUser::ErrorPause(  System::String* errBuf, p4dn::Error* err )
{    

}

void p4dn::ClientUser::Edit( System::IO::FileInfo* info, p4dn::Error *err )
{       
    
}

void p4dn::ClientUser::Diff( System::IO::FileInfo* f1, 
                             System::IO::FileInfo* f2, 
                             int doPage, 
                             System::String* diffFlags, 
                             p4dn::Error *err )
{


}

void p4dn::ClientUser::Merge( System::IO::FileInfo* base, 
                              System::IO::FileInfo* leg1, 
                              System::IO::FileInfo* leg2, 
                              System::IO::FileInfo* result, 
                              p4dn::Error* err )
{

}

void p4dn::ClientUser::Help( System::String* help )
{
    
}

//void p4dn::ClientUser::RunCmd( System::String* command, 
//                               System::String* arg1, 
//                               System::String* arg2, 
//                               System::String* arg3, 
//                               System::String* arg4, 
//							   System::String* arg5, 
//                               System::String* pager,
//							   p4dn::Error* err )	
//{    
//    char* cmd = (char *)(void *) Marshal::StringToHGlobalAnsi( command ); 
//    char* a1  = (char *)(void *) Marshal::StringToHGlobalAnsi( arg1 );     
//    char* a2  = (char *)(void *) Marshal::StringToHGlobalAnsi( arg2 );     
//    char* a3  = (char *)(void *) Marshal::StringToHGlobalAnsi( arg3 );     
//    char* a4  = (char *)(void *) Marshal::StringToHGlobalAnsi( arg4 );     
//    char* pgr = (char *)(void *) Marshal::StringToHGlobalAnsi( pager );     
//
//	//Static method call
//    ::ClientUser::RunCmd( cmd, a1, a2, a3, a4, pgr, err->get_InternalError() );
//
//    Marshal::FreeHGlobal( cmd );
//    Marshal::FreeHGlobal( a1 );
//    Marshal::FreeHGlobal( a2 );
//    Marshal::FreeHGlobal( a3 );
//    Marshal::FreeHGlobal( a4 );
//	Marshal::FreeHGlobal( a5 );
//    Marshal::FreeHGlobal( pgr );
//}
