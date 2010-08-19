/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/

#ifndef ANH_UTILS_BSTRING_H
#define ANH_UTILS_BSTRING_H

#include <vector>
#include "Utils/typedefs.h"
#include "Utils/declspec.h"

class BString;

typedef std::vector<BString> BStringVector;

//======================================================================================================================

#define BSTRING_ALLOC_BLOCK_SIZE      64

enum BStringType
{
  BSTRType_ANSI,
  BSTRType_Unicode16,
  BSTRType_UTF8
};

//======================================================================================================================

class UTILS_API BString
{
public:
    BString();
    BString(BStringType type, uint16 length);
    ~BString();

    BString(const int8* data);
    BString(const uint16* data);
    BString(const wchar_t* data);
    BString(const BString& data);

    uint16 initRawBSTR(int8* data, BStringType type);

    // Operator overloads
    bool operator ==(char* string);
    bool operator ==(const char* data);
    bool operator ==(const BString& data);
    bool operator ==(BString data);

    BString& operator =(const int8* data);

    BString& operator =(int8* data);
    BString& operator =(const uint16* data);
    BString& operator =(const wchar_t* data);
    BString& operator =(const BString& data);

    BString& operator <<(const int8* data);
    BString& operator <<(const uint16* data);
    BString& operator <<(int8* data);
    BString& operator <<(uint16* data);

    // Accessor Methods
    int8* getAnsi();
    const int8* getAnsi() const;
   // uint16* getUnicode16();
    wchar_t* getUnicode16();
    const wchar_t* getUnicode16() const;
    int8*   getUTF8();

    void convert(BStringType type);
    void substring(BString& dest, uint16 start, uint16 end);

    int  split(BStringVector& retVec,char delimiter);
    void toLower();
    void toUpper();
    void toUpperFirst();
    void toLowerFirst();
    bool isNumber();

    void setLength(uint16 length);
    void setType(BStringType type);

    uint16 getLength() const;
    uint32 getDataLength() const;
    uint32 getCharacterWidth() const;

    BStringType getType() const;
    int8* getRawData() const;
    uint32 getAllocated() const;
    uint32 getCrc() const;

    static uint32 CRC(char* data);

private:
    void _allocate();

    int8* mString;          // Pointer to the allocated buffer
    BStringType mType;            // What format is the current string in.
    uint16 mAllocated;       // Length of the allocated buffer which should be a multiple of
    uint16 mCharacterWidth;  // Size of a single character in bytes
    uint16 mLength;          // Length of the string itself.  BStrings are NOT null terminated

    static uint32 mCrcTable[256];
};

#endif //MMOSERVER_UTILS_BSTRING_H


