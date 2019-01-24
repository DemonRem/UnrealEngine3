/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #pragma once

namespace StandaloneSymbolParser
{
	using namespace System;
	using namespace System::IO;

	/**
	 * Contains information about a module loaded by a process.
	 */
	public ref class ModuleInfo
	{
	public:
		/** Base address the module is loaded at */
		unsigned __int64 BaseOfImage;
		/** Size of the image on disk */
		unsigned int ImageSize;
		/** Time/date stamp for the pdb */
		unsigned int TimeDateStamp;
		/** The name of the module */
		String ^ModuleName;
		/** The full image name */
		String ^ImageName;
		/** The full name of the image when loaded */
		String ^LoadedImageName;
		/** The old pdb signature (replaced by pdbsig70) */
		unsigned int PdbSig;
		/** The pdb signature */
		Guid PdbSig70;
		/** The age of the pdb */
		unsigned int PdbAge;

		/**
		 * Constructor.
		 *
		 * @param	BinaryStream	The stream that module deserializes itself from.
		 */
		ModuleInfo(BinaryReader ^BinaryStream);

		/**
		 * Constructor.
		 *
		 * @param	BaseAddr	The module base address.
		 * @param	ImgSize		The size of the module on disk.
		 * @param	Age			The age of the PDB.
		 * @param	Signature	The signature of the PDB.
		 * @param	Path		The location on disk of the module (according to its header).
		 */
		ModuleInfo(unsigned __int64 BaseAddr, unsigned int ImgSize, unsigned int Age, Guid Signature, String ^Path);
	};
}
