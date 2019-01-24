/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace MemoryProfiler2
{
	/**
	 * Helper class containing information shared for all snapshots of a given stream.
	 */
	public class FStreamInfo
	{
		/** File name associated with this stream.									*/
		public string FileName;

		/** Array of unique names. Code has fixed indexes into it.					*/
		public string[] NameArray;
		/** Array of unique call stacks. Code has fixed indexes into it.			*/
		public FCallStack[] CallStackArray;
		/** Array of unique call stack addresses. Code has fixed indexes into it.	*/
		public FCallStackAddress[] CallStackAddressArray;
		/** Array of modules. */
		public StandaloneSymbolParser.ModuleInfo[] ModuleInfo = new StandaloneSymbolParser.ModuleInfo[0];

		/** Global public stream info so we don't need to pass it around.			*/
		public static FStreamInfo GlobalInstance = null;

		/**
		 * Constructor, associating this stream info with a filename. 
		 * @param	InFileName	FileName to use for this stream
		 */
		public FStreamInfo(string InFileName)
		{
			FileName = InFileName;
		}

		/**
		 * Initializes and sizes the arrays. Size is known as soon as header is serialized.
		 */
		public void Initialize(uint NameArrayEntries,uint CallStackArrayEntries,uint CallStackAddressArrayEntries)
		{
			NameArray = new string[NameArrayEntries];
			CallStackArray = new FCallStack[CallStackArrayEntries];
			CallStackAddressArray = new FCallStackAddress[CallStackAddressArrayEntries];
		}
	};
}