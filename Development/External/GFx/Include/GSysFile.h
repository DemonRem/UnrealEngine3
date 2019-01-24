/**********************************************************************

Filename    :   GSysFile.h
Content     :   Header for all internal file management-
                functions and structures to be inherited by OS
                specific subclasses.
Created     :   July 29, 1998
Authors     :   Michael Antonov, Brendan Iribe, Andrew Reisse

Notes       :   errno may not be preserved across use of GBaseFile member functions
            :   Directories cannot be deleted while files opened from them are in use
                (For the GetFullName function)

History     :   4/04/1999   :   Changed GBaseFile to support directory
                1/20/1999   :   Updated to follow new style
                8/25/2001   :   MA - Restructured file, directory, added streams,
                                    item directories & paths

Copyright   :   (c) 1998-2003 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysFile_H
#define INC_GSysFile_H

#include "GFile.h"

// ***** Declared classes
class   GSysFile;

// *** File Statistics

// This class contents are similar to _stat, providing
// creation, modify and other information about the file.
struct GFileStat
{
    // No change or create time because they are not available on most systems
    SInt64  ModifyTime;
    SInt64  AccessTime;
    SInt64  FileSize;

    bool operator== (const GFileStat& stat) const
    {
        return ( (ModifyTime == stat.ModifyTime) &&
                 (AccessTime == stat.AccessTime) &&
                 (FileSize == stat.FileSize) );
    }
};

// *** System File

// System file is created to access objects on file system directly
// This file can refer directly to path.
// System file can be open & closed several times; however, such use is not recommended
// This class is realy a wrapper around an implementation of GFile interface for a 
// particular platform.

class GSysFile : public GDelegatedFile
{
protected:
  GSysFile(const GSysFile &source) : GDelegatedFile () { GUNUSED(source); }
public:

    // ** Constructor
    GEXPORT GSysFile();
    // Opens a file
    // For some platforms (PS2,PSP,Wii) buffering will be used even if Open_Buffered flag is not set because 
    // of these platforms' file system limitation.
    GEXPORT GSysFile(const GString& path, SInt flags = Open_Read|Open_Buffered, SInt mode = Mode_ReadWrite); 

    // ** Open & management 
    // Will fail if file's already open
    // For some platforms (PS2,PSP,Wii) buffering will be used even if Open_Buffered flag is not set because 
    // of these platforms' file system limitation.
    GEXPORT bool    Open(const GString& path, SInt flags = Open_Read|Open_Buffered, SInt mode = Mode_ReadWrite);
        
    GINLINE bool    Create(const GString& path, SInt mode = Mode_ReadWrite)
        {   return Open(path, Open_ReadWrite|Open_Create, mode); }

    // Helper function: obtain file statistics information. In GFx, this is used to detect file changes.
    // Return 0 if function failed, most likely because the file doesn't exist.
    GEXPORT static bool GCDECL GetFileStat(GFileStat* pfileStats, const GString& path);
    
    // ** Overrides
    // Overridden to provide re-open support
    GEXPORT virtual SInt    GetErrorCode();

    GEXPORT virtual bool    IsValid();

    GEXPORT virtual bool    Close();
    //GEXPORT   virtual bool    CloseCancel();
};

#endif
