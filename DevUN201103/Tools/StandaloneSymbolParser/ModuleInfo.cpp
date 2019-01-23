/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #include "Stdafx.h"
#include "ModuleInfo.h"

using namespace System::Text;

namespace StandaloneSymbolParser
{
	/**
	 * Constructor.
	 *
	 * @param	BinaryStream	The stream that module deserializes itself from.
	 */
	ModuleInfo::ModuleInfo(BinaryReader ^BinaryStream)
	{
		BaseOfImage = BinaryStream->ReadUInt64();
		ImageSize = BinaryStream->ReadUInt32();
		TimeDateStamp = BinaryStream->ReadUInt32();
		PdbSig = BinaryStream->ReadUInt32();
		PdbAge = BinaryStream->ReadUInt32();

		array<Byte> ^Bits = nullptr;

		PdbSig70 = Guid(BinaryStream->ReadInt32(), BinaryStream->ReadInt16(), BinaryStream->ReadInt16(), BinaryStream->ReadByte(), BinaryStream->ReadByte(), BinaryStream->ReadByte(),
			BinaryStream->ReadByte(), BinaryStream->ReadByte(), BinaryStream->ReadByte(), BinaryStream->ReadByte(), BinaryStream->ReadByte());

		int StrLen = BinaryStream->ReadInt32();
		Bits = BinaryStream->ReadBytes(StrLen);

		ModuleName = Encoding::UTF8->GetString(Bits);

		StrLen = BinaryStream->ReadInt32();
		Bits = BinaryStream->ReadBytes(StrLen);

		ImageName = Encoding::UTF8->GetString(Bits);

		StrLen = BinaryStream->ReadInt32();
		Bits = BinaryStream->ReadBytes(StrLen);

		LoadedImageName = Encoding::UTF8->GetString(Bits);
	}

	ModuleInfo::ModuleInfo(unsigned __int64 BaseAddr, unsigned int ImgSize, unsigned int Age, Guid Signature, String ^Path)
		: BaseOfImage(BaseAddr), ImageSize(ImgSize), PdbAge(Age), PdbSig70(Signature), ImageName(Path), LoadedImageName(Path)
	{
		if(Path == nullptr)
		{
			throw gcnew ArgumentNullException(L"Path");
		}

		ModuleName = Path::GetFileName(Path);
	}
}
