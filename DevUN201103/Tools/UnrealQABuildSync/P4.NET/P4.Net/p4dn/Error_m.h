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
#include "StdAfx.h"
#include <vcclr.h>

template < typename T >  //  T is a managed class here
struct auto_dispose : gcroot< T * >  //  gcroot<> is an unmanaged class that wraps a pointer to a managed class
{
     explicit auto_dispose( T * inner )
         : base_type( inner )
     {}

     ~auto_dispose()
     {
         ( *this )->Dispose();
     }
};


namespace p4dn {

	__gc public class Error : public System::IDisposable 
    {

    public:

		Error( System::Text::Encoding* encoding);

		/*
			From error.h:
			E_EMPTY = 0,	// nothing yet
			E_INFO = 1,	// something good happened
			E_WARN = 2,	// something not good happened
			E_FAILED = 3,	// user did somthing wrong
			E_FATAL = 4	// system broken -- nothing can continue

		*/
		__value enum ErrorSeverity {
			Empty = 0,
			Info = 1,
			Warning = 2,
			Failed = 3,
			Fatal = 4
		};

		~Error();
		void Dispose(); // : System::IDisposable::Dispose();
        void Clear();
				
        bool Test();
        bool IsInfo();
        bool IsWarning();
		bool IsFailed();
        bool IsFatal();
        
        System::String* Fmt();

        __property ErrorSeverity get_Severity() {
             return static_cast< ErrorSeverity >( _err->GetSeverity() );
        }

    private:          
        ::Error* _err;
        bool _requiresFree;
		bool _Disposed;
		System::String* _Instance;
		System::Text::Encoding* _encoding;
		void CleanUp();

    public private:        
        Error( ::Error* e, System::Text::Encoding* encoding);
		__property ::Error* get_InternalError();
    };

} // end namespace




