/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;
using StandaloneSymbolParser;
using System.ComponentModel;

namespace MemoryProfiler2
{	
	public static class FStreamParser
	{
        private static BinaryReader SwitchStreams(int PartIndex, string BaseFilename, bool bIsBigEndian, out FileStream ParserStream)
        {
            // Determine the effective filename for this part
            string CurrentFilename = BaseFilename;
            if (PartIndex > 0)
            {
                CurrentFilename = Path.ChangeExtension(BaseFilename, String.Format(".m{0}", PartIndex));
            }

            // Open the file
            ParserStream = File.OpenRead(CurrentFilename);

            // Create a reader of the correct endianness
			BinaryReader BinaryStream;            
            if (bIsBigEndian)
            {
				BinaryStream = new BinaryReaderBigEndian(ParserStream);
            }
            else
            {
                BinaryStream = new BinaryReader(ParserStream, System.Text.Encoding.ASCII);
            }
            return BinaryStream;
        }

		/**
		 * Parses the passed in token stream and returns list of snapshots.
		 */
		public static List<FStreamSnapshot> Parse(MainWindow MainMProfWindow, BackgroundWorker BGWorker)
		{
            string PrettyFilename = Path.GetFileNameWithoutExtension(FStreamInfo.GlobalInstance.FileName);
            BGWorker.ReportProgress(0, "Loading header information for " + PrettyFilename);

			// Create binary reader and file info object from filename.
            bool bIsBigEndian = false;
			FileStream ParserFileStream = File.OpenRead(FStreamInfo.GlobalInstance.FileName);
			BinaryReader BinaryStream = new BinaryReader(ParserFileStream,System.Text.Encoding.ASCII);

			// Serialize header.
			FProfileDataHeader Header = new FProfileDataHeader(BinaryStream);

			// Determine whether read file has magic header. If no, try again byteswapped.
			if(Header.Magic != FProfileDataHeader.ExpectedMagic)
			{
				// Seek back to beginning of stream before we retry.
				ParserFileStream.Seek(0,SeekOrigin.Begin);

				// Use big endian reader. It transparently endian swaps data on read.
				BinaryStream = new BinaryReaderBigEndian(ParserFileStream);
                bIsBigEndian = true;
				
				// Serialize header a second time.
				Header = new FProfileDataHeader(BinaryStream);
			}

			// At this point we should have a valid header. If no, throw an exception.
			if( Header.Magic != FProfileDataHeader.ExpectedMagic )
			{
				throw new InvalidDataException();
			}

            // Keep track of the current data file for multi-part recordings
            int NextDataFile = 1;

			// Initialize shared information across snapshots, namely names, callstacks and addresses.
			FStreamInfo.GlobalInstance.Initialize(Header.NameTableEntries, Header.CallStackTableEntries, Header.CallStackAddressTableEntries);

			// Keep track of current position as it's where the token stream starts.
			long TokenStreamOffset = ParserFileStream.Position;

			// Seek to name table and serialize it.
			ParserFileStream.Seek(Header.NameTableOffset,SeekOrigin.Begin);
			for(int NameIndex = 0;NameIndex < Header.NameTableEntries;NameIndex++)
			{
				UInt32 Length = BinaryStream.ReadUInt32();
				FStreamInfo.GlobalInstance.NameArray[NameIndex] = new string(BinaryStream.ReadChars((int)Length));
			}

			// Seek to callstack address array and serialize it.                
			ParserFileStream.Seek(Header.CallStackAddressTableOffset,SeekOrigin.Begin);
			for(int AddressIndex = 0;AddressIndex < Header.CallStackAddressTableEntries;AddressIndex++)
			{
				FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndex] = new FCallStackAddress(BinaryStream, Header.bShouldSerializeSymbolInfo);
			}

			// Seek to callstack array and serialize it.
			ParserFileStream.Seek(Header.CallStackTableOffset,SeekOrigin.Begin);
			for(int CallStackIndex = 0;CallStackIndex < Header.CallStackTableEntries;CallStackIndex++)
			{
				FStreamInfo.GlobalInstance.CallStackArray[CallStackIndex] = new FCallStack(BinaryStream);
			}

			if(Header.ModuleEntries > 0)
			{
				BinaryStream.BaseStream.Seek(Header.ModulesOffset, SeekOrigin.Begin);

				FStreamInfo.GlobalInstance.ModuleInfo = new ModuleInfo[Header.ModuleEntries];

				for(uint ModuleIndex = 0; ModuleIndex < FStreamInfo.GlobalInstance.ModuleInfo.Length; ++ModuleIndex)
				{
					FStreamInfo.GlobalInstance.ModuleInfo[ModuleIndex] = new ModuleInfo(BinaryStream);
				}
			}

			// We need to look up symbol information ourselves if it wasn't serialized.
			if( !Header.bShouldSerializeSymbolInfo )
			{
                LookupSymbols(Header, MainMProfWindow, BGWorker);
			}

			// Snapshot used for parsing. A copy will be made if a special token is encountered. Otherwise it
			// will be returned as the only snaphot at the end.
			FStreamSnapshot Snapshot = new FStreamSnapshot("End");
			List<FStreamSnapshot> SnapshotList = new List<FStreamSnapshot>();

			// Seek to beginning of token stream.
			ParserFileStream.Seek(TokenStreamOffset, SeekOrigin.Begin);

            // Figure out the progress scale
            long StartOfMetadata = Math.Min(Header.NameTableOffset, Header.CallStackAddressTableOffset);
            StartOfMetadata = Math.Min(StartOfMetadata, Header.CallStackTableOffset);
            StartOfMetadata = Math.Min(StartOfMetadata, Header.ModulesOffset);

            long ProgressInterval = (StartOfMetadata - TokenStreamOffset) / 100;
            double ProgressScaleFactor = 100.0f / (StartOfMetadata - TokenStreamOffset);
            long NextProgressUpdate = TokenStreamOffset;

			// Parse tokens till we reach the end of the stream.

			FStreamToken Token = new FStreamToken();
			while (Token.ReadNextToken(BinaryStream))
			{
                long CurrentStreamPos = ParserFileStream.Position;
                if (ParserFileStream.Position >= NextProgressUpdate)
                {
                    BGWorker.ReportProgress(
                        (int)((CurrentStreamPos - TokenStreamOffset) * ProgressScaleFactor),
                        String.Format("Parsing token stream for {0}, part {1} of {2}", PrettyFilename, NextDataFile, Header.NumDataFiles));
                    NextProgressUpdate += ProgressInterval;
                }

				switch (Token.Type)
				{
					// Malloc
					case EProfilingPayloadType.TYPE_Malloc:
						HandleMalloc( Token, ref Snapshot );
						break;

					// Free
					case EProfilingPayloadType.TYPE_Free:
						HandleFree( Token, ref Snapshot );
						break;

					// Realloc
					case EProfilingPayloadType.TYPE_Realloc:
						HandleFree( Token, ref Snapshot );
						Token.Pointer = Token.NewPointer;
						HandleMalloc( Token, ref Snapshot );
						break;

					// Status/ payload.
					case EProfilingPayloadType.TYPE_Other:
						switch(Token.SubType)
						{
							case EProfilingPayloadSubType.SUBTYPE_EndOfStreamMarker:
    							// Should never receive EOS marker as ReadNextToken should've returned false.
								throw new InvalidDataException();

                            case EProfilingPayloadSubType.SUBTYPE_EndOfFileMarker:
                                // Switch to the next file in the chain
                    			ParserFileStream.Close();
                                BinaryStream = SwitchStreams(NextDataFile, FStreamInfo.GlobalInstance.FileName, bIsBigEndian, out ParserFileStream);

                                // Update variables used for reporting progress
                                TokenStreamOffset = 0;
                                ProgressInterval = ParserFileStream.Length / 100;
                                ProgressScaleFactor = 100.0f / ParserFileStream.Length;
                                NextProgressUpdate = 0;

                                // Tick over to the next file, and make sure things are still ending as expected
                                NextDataFile++;
                                if (NextDataFile > Header.NumDataFiles)
                                {
                                    throw new InvalidDataException("Found an unexpected number of data files (more than indicated in the master file");
                                }
                                break;

							case EProfilingPayloadSubType.SUBTYPE_SnapshotMarker:
	    						// Create snapshot.
								FStreamSnapshot MarkerSnapshot = Snapshot.DeepCopy();

                                if (FStreamInfo.GlobalInstance.NameArray[Token.TextIndex] != "")
                                {
                                    MarkerSnapshot.Description = FStreamInfo.GlobalInstance.NameArray[Token.TextIndex];
                                    MarkerSnapshot.bHasUserDescription = true;
                                }

								SnapshotList.Add(MarkerSnapshot);
								break;


							case EProfilingPayloadSubType.SUBTYPE_TotalUsed:
								Snapshot.CurrentOSUsed = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_TotalAllocated:
								Snapshot.CurrentOSAllocated = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_CPUUsed:
								Snapshot.CurrentVirtualUsed = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_CPUSlack:
								Snapshot.CurrentVirtualSlack = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_CPUWaste:
								Snapshot.CurrentVirtualWaste = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_GPUUsed:
								Snapshot.CurrentPhysicalUsed = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_GPUSlack:
								Snapshot.CurrentPhysicalSlack = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_GPUWaste:
								Snapshot.CurrentPhysicalWaste = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_OSOverhead:
								Snapshot.CurrentOSOverhead = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_ImageSizeMarker:
								Snapshot.CurrentImageSize = Token.Payload;
								break;

							case EProfilingPayloadSubType.SUBTYPE_FrameTimeMarker:
							case EProfilingPayloadSubType.SUBTYPE_TextMarker:
								break;

							// Unhandled.
							default:
								throw new InvalidDataException();
						}
						break;

					// Unhandled.
					default:
						throw new InvalidDataException();
				}
			}

			// Closes the file so it can potentially be opened for writing.
			ParserFileStream.Close();

			foreach( FCallStack CallStack in FStreamInfo.GlobalInstance.CallStackArray )
			{
				// Find the first non templated entry in each callstack
				CallStack.EvaluateFirstNonContainer();

				// Find the group this callstack belongs to
				CallStack.EvaluateGroup(MainMProfWindow.Options.ClassGroups);
			}

			// Add snapshot in end state to the list and return it.
			SnapshotList.Add(Snapshot);

            BGWorker.ReportProgress(100, "Finalizing snapshots for " + PrettyFilename);

			// Finalize snapshots. This entails creating the sorted snapshot list.
			foreach( FStreamSnapshot SnapshotToFinalize in SnapshotList )
			{
				SnapshotToFinalize.FinalizeSnapshot(null);
			}

			return SnapshotList;
		}

		/** Updates internal state with allocation. */
		private static void HandleMalloc( FStreamToken Token, ref FStreamSnapshot Snapshot )
		{
		    // Disregard requests that didn't result in an allocation.
			if( Token.Pointer != 0 && Token.Size > 0 )
			{
				// Keep track of size associated with pointer and also current callstack.
                FCallStackAllocationInfo PointerInfo = new FCallStackAllocationInfo( Token.Size, Token.CallStackIndex, 1 );
				Snapshot.PointerToPointerInfoMap.Add( Token.Pointer, PointerInfo );
                // Add size to lifetime churn tracking.
                Snapshot.LifetimeCallStackList[Token.CallStackIndex] = Snapshot.LifetimeCallStackList[Token.CallStackIndex].Add( Token.Size, 1 );

				// Maintain timeline view
				Snapshot.AllocationSize += PointerInfo.Size;
				Snapshot.AllocationCount++;

				if( Snapshot.AllocationSize > Snapshot.AllocationMaxSize )
				{
					Snapshot.AllocationMaxSize = Snapshot.AllocationSize;
				}

				if( Snapshot.AllocationCount % 5000 == 0 )
				{
					FMemorySlice Slice = new FMemorySlice( Snapshot );
					Snapshot.OverallMemorySlice.Add( Slice );
					Snapshot.AllocationMaxSize = 0;
				}
			}
		}

		/** Updates internal state with free. */
		private static void HandleFree( FStreamToken Token, ref FStreamSnapshot Snapshot )
		{
			// @todo: We currently seem to free/ realloc pointers we didn't allocate, which seems to be a bug that needs to 
			// be fixed in the engine. The below gracefully handles freeing pointers that either never have been allocated 
			// or are already freed.
			if( Snapshot.PointerToPointerInfoMap.ContainsKey( Token.Pointer ) )
			{
				FCallStackAllocationInfo PointerInfo = Snapshot.PointerToPointerInfoMap[Token.Pointer];

				// Maintain timeline view
				Snapshot.AllocationSize -= PointerInfo.Size;
				Snapshot.AllocationCount++;
				if( Snapshot.AllocationCount % 5000 == 0 )
				{
					FMemorySlice Slice = new FMemorySlice( Snapshot );
					Snapshot.OverallMemorySlice.Add( Slice );
				}

				// Remove freed pointer if it is in the array.
				Snapshot.PointerToPointerInfoMap.Remove(Token.Pointer);
			}
		}

		/** Lookup symbols for callstack addresses in passed in FStreamInfo.GlobalInstance. */
		private static void LookupSymbols( FProfileDataHeader Header, MainWindow MainMProfWindow, BackgroundWorker BGWorker )
		{
            string PrettyFilename = Path.GetFileNameWithoutExtension(FStreamInfo.GlobalInstance.FileName);
            BGWorker.ReportProgress(0, "Loading symbols for " + PrettyFilename);
            
            // Proper Symbol parser will be created based on platform.
			SymbolParser ConsoleSymbolParser = null;

            // Search for symbols in the same directory as the mprof first to allow packaging of previous results
            // PS3 /*PLATFORM_PS3 in UnFile.h*/
            if (Header.Platform == 0x00000008)
            {
                ConsoleSymbolParser = new PS3SymbolParser();
                int LastPathSeparator = MainMProfWindow.CurrentFilename.LastIndexOf('\\');
                if (LastPathSeparator != -1)
                {
                    string CurrentPath = MainMProfWindow.CurrentFilename.Substring(0, LastPathSeparator);
                    if (!ConsoleSymbolParser.LoadSymbols(CurrentPath + "\\" + Header.ExecutableName + "xelf", FStreamInfo.GlobalInstance.ModuleInfo))
                    {
                        ConsoleSymbolParser = null;
                    }
                }
            }
            // Xbox 360 /*PLATFORM_Xenon in UnFile.h*/
            else if (Header.Platform == 0x00000004)
            {
                Directory.SetCurrentDirectory(Path.GetDirectoryName(MainMProfWindow.CurrentFilename));

                ConsoleSymbolParser = new XenonSymbolParser();
                int LastPathSeparator = MainMProfWindow.CurrentFilename.LastIndexOf('\\');
                if (LastPathSeparator != -1)
                {
                    string CurrentPath = MainMProfWindow.CurrentFilename.Substring(0, LastPathSeparator);
                    if (!ConsoleSymbolParser.LoadSymbols(CurrentPath + "\\" + Header.ExecutableName + ".xex", FStreamInfo.GlobalInstance.ModuleInfo))
                    {
                        ConsoleSymbolParser = null;
                    }
                }
            }
            // Win32 /*PLATFORM_Windows, PLATFORM_WindowsConsole, PLATFORM_WindowsServer in UnFile.h*/
			else if( Header.Platform == 0x00000001 ||
                     Header.Platform == 0x00000002 ||
                     Header.Platform == 0x00000040 )
			{
				// unsupported - application evaluates symbols at runtime
			}

            // If symbols weren't found in the same directory as the mprof, search the intermediate pdb locations
            if (ConsoleSymbolParser == null)
            {
                // PS3 /*PLATFORM_PS3 in UnFile.h*/
                if (Header.Platform == 0x00000008)
                {
                    ConsoleSymbolParser = new PS3SymbolParser();
					if(!ConsoleSymbolParser.LoadSymbols(Header.ExecutableName + "xelf", FStreamInfo.GlobalInstance.ModuleInfo))
                    {
                        ConsoleSymbolParser = null;
                    }
                }
                // Xbox 360 /*PLATFORM_Xenon in UnFile.h*/
                else if (Header.Platform == 0x00000004)
                {
                    ConsoleSymbolParser = new XenonSymbolParser();
					if(!ConsoleSymbolParser.LoadSymbols("..\\" + Header.ExecutableName + ".xex", FStreamInfo.GlobalInstance.ModuleInfo))
                    {
                        ConsoleSymbolParser = null;
                    }
                }
                // Win32 /*PLATFORM_Windows, PLATFORM_WindowsConsole, PLATFORM_WindowsServer in UnFile.h*/
                else if (Header.Platform == 0x00000001 ||
                         Header.Platform == 0x00000002 ||
                         Header.Platform == 0x00000040)
				{
					// unsupported - application evaluates symbols at runtime
				}
            }

			// If console parses is null it means that either the platform is not supported or the symbols couldn't
			// be loaded.
			if( ConsoleSymbolParser == null )
			{
				MessageBox.Show("Failed to load symbols for " + Header.ExecutableName);
				throw new InvalidDataException();
			}

			// Create mapping from string to index in name array.
			Dictionary<string, int> NameToIndexMap = new Dictionary<string,int>();

			// Propagate existing name entries to map.
			for( int NameIndex=0; NameIndex<FStreamInfo.GlobalInstance.NameArray.Length; NameIndex++ )
			{
				NameToIndexMap.Add( FStreamInfo.GlobalInstance.NameArray[NameIndex], NameIndex );
			}

			// Current index is incremented whenever a new name is added.
			int CurrentNameIndex = FStreamInfo.GlobalInstance.NameArray.Length;

			// Iterate over all addresses and look up symbol information.
            int NumAddressesPerTick = FStreamInfo.GlobalInstance.CallStackAddressArray.Length / 100;
            int AddressIndex = 0;
            string ProgressString = "Resolving symbols for " + PrettyFilename;

			foreach( FCallStackAddress Address in FStreamInfo.GlobalInstance.CallStackAddressArray )
			{
                if ((AddressIndex % NumAddressesPerTick) == 0)
                {
                    BGWorker.ReportProgress(AddressIndex / NumAddressesPerTick, ProgressString);
                }
                ++AddressIndex;

                // Look up symbol info via console support DLL.
				string Filename = "";
				string Function = "";
				ConsoleSymbolParser.ResolveAddressToSymboInfo( (uint) Address.ProgramCounter, ref Filename, ref Function, ref Address.LineNumber );
	
				// Look up filename index.
				if( NameToIndexMap.ContainsKey(Filename) )
				{
					// Use existing entry.
					Address.FilenameIndex = NameToIndexMap[Filename];
				}
				// Not found, so we use global name index to set new one.
				else
				{
					// Set name in map associated with global ever increasing index.
					Address.FilenameIndex = CurrentNameIndex++;
					NameToIndexMap.Add( Filename, Address.FilenameIndex );
				}

				// Look up function index.
				if( NameToIndexMap.ContainsKey(Function) )
				{
					// Use existing entry.
					Address.FunctionIndex = NameToIndexMap[Function];
				}
				// Not found, so we use global name index to set new one.
				else
				{
					// Set name in map associated with global ever increasing index.
					Address.FunctionIndex = CurrentNameIndex++;
					NameToIndexMap.Add( Function, Address.FunctionIndex );
				} 
			}

			// Create new name array based on dictionary.
			FStreamInfo.GlobalInstance.NameArray = new String[CurrentNameIndex];
			foreach (KeyValuePair<string, int> NameMapping in NameToIndexMap)
			{
				FStreamInfo.GlobalInstance.NameArray[NameMapping.Value] = NameMapping.Key;
			}

			// Unload symbols again.
			ConsoleSymbolParser.UnloadSymbols();
		}
	};
}