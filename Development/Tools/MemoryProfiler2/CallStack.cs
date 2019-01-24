/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;

namespace MemoryProfiler2
{
    /**
     * Encapsulates callstack information
     */
    public class FCallStack
    {
        /** CRC of callstack pointers. */
        private Int32 CRC;

        /** Callstack as indices into address list, from top to bottom. */
        public List<int> AddressIndices;

		/** First entry in the callstack that is *not* a container */
		public int FirstNonContainer;

		/** The class group that this callstack is associated with */
		public ClassGroup Group;

		/** Whether this callstack is truncated. */
		public bool	bIsTruncated;

        /**
         * Constructor
         * 
         * @param	BinaryStream	Stream to read from
         */
        public FCallStack(BinaryReader BinaryStream)
        {
            // Read CRC of original callstack.
            CRC = BinaryStream.ReadInt32();

            // Create new arrays.
            AddressIndices = new List<int>();

            // Read call stack address indices and parse into arrays.
            int AddressIndex = BinaryStream.ReadInt32();
            while(AddressIndex >= 0)
            {
                AddressIndices.Add(AddressIndex);
                AddressIndex = BinaryStream.ReadInt32();
            }

			// Normal callstacks are -1 terminated, whereof truncated ones are -2 terminated.
			if( AddressIndex == -2 )
			{
				bIsTruncated = true;
			}
			else
			{
				bIsTruncated = false;
			}

			FirstNonContainer = AddressIndices.Count - 1;

            // We added bottom to top but prefer top bottom for hierarchical view.
            AddressIndices.Reverse();
        }

		/** 
		 * Find the first non templated argument in the callstack
		 */
		static private List<string> CommonNames = new List<string>()
		{
			"operator new<",
			"FString::operator=",
			"FStringNoInit::operator=",
			"FString::FString",
			"FBestFitAllocator::",
			"FHeapAllocator::"
		};

		public void EvaluateFirstNonContainer()
		{
			for( int AddressIndex = AddressIndices.Count - 1; AddressIndex > 0; AddressIndex-- )
			{
				bool bIsContainer = false;
				string FunctionName = FStreamInfo.GlobalInstance.NameArray[FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndices[AddressIndex]].FunctionIndex];

				if( FunctionName.Contains( ">::" ) || FunctionName.Contains( "operator<<" ) )
				{
					bIsContainer = true;
				}
				else
				{
					// See if the function name is one of the common set to ignore
					foreach( string CommonName in CommonNames )
					{
						if( FunctionName.StartsWith( CommonName ) )
						{
							bIsContainer = true;
							break;
						}
					}
				}

				// if none are templates - we're good!
				if( !bIsContainer )
				{
					FirstNonContainer = AddressIndex;
					break;
				}
			}
		}

		private ClassGroup GetUngroupedGroup( ref List<ClassGroup> ClassGroups )
		{
			foreach( ClassGroup ClassGroup in ClassGroups )
			{
				if( ClassGroup.Name == "Ungrouped" )
				{
					return ( ClassGroup );
				}
			}

			return ( null );
		}

		/** 
		 * Find the class group this callstack belongs to
		 */
		public void EvaluateGroup( List<ClassGroup> ClassGroups )
		{
			// Check against any active classes
			Group = null;
			foreach( ClassGroup ClassGroup in ClassGroups )
			{
				// Continue searching while no group found, there are some class names to compare and there are enough entries in the callstack
				if( Group == null && ClassGroup.ClassNames != null && AddressIndices.Count > FirstNonContainer )
				{
					foreach( string ClassName in ClassGroup.ClassNames )
					{
						if( FStreamInfo.GlobalInstance.NameArray[FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndices[FirstNonContainer]].FunctionIndex].StartsWith( ClassName ) )
						{
							Group = ClassGroup;
							break;
						}
					}
				}
			}

			// Set to the ungrouped group if it doesn't fall into any other category
			if( Group == null )
			{
				Group = GetUngroupedGroup( ref ClassGroups );
			}
		}

		/**
		 * Compares two callstacks for sorting.
		 * 
		 * @param	A	First callstack to compare
		 * @param	B	Second callstack to compare
		 */
		public static int Compare( FCallStack A, FCallStack B )
		{
			// Not all callstacks have the same depth. Figure out min for comparision.
			int MinSize = Math.Min( A.AddressIndices.Count, B.AddressIndices.Count );

			// Iterate over matching size and compare.
			for( int i=0; i<MinSize; i++ )
			{
				// Sort by address
				if( A.AddressIndices[i] > B.AddressIndices[i] )
				{
					return 1;
				}
				else if( A.AddressIndices[i] < B.AddressIndices[i] )
				{
					return -1;
				}
			}

			// If we got here it means that the subset of addresses matches. In theory this means
			// that the callstacks should have the same size as you can't have the same address
			// doing the same thing, but let's simply be thorough and handle this case if the
			// stackwalker isn't 100% accurate.

			// Matching length means matching callstacks.
			if( A.AddressIndices.Count == B.AddressIndices.Count )
			{
				return 0;
			}
			// Sort by additional length.
			else
			{
				return A.AddressIndices.Count > B.AddressIndices.Count ? 1 : -1;
			}
		}

		public void AddToListView( ListView CallStackListView, bool bShowFromBottomUp )
		{
			for( int i=0; i<AddressIndices.Count; i++ )
			{
				// Handle iterating over addresses in reverse order.
				int AddressIndexIndex = bShowFromBottomUp ? AddressIndices.Count - 1 - i : i;
				FCallStackAddress Address = FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndices[AddressIndexIndex]];

				string[] Row = new string[]
				{
					FStreamInfo.GlobalInstance.NameArray[Address.FunctionIndex],
					FStreamInfo.GlobalInstance.NameArray[Address.FilenameIndex],
					Address.LineNumber.ToString()
				};
				CallStackListView.Items.Add( new ListViewItem(Row) );
			}
		}

		/**
		 * Filters this callstack based on passed in filter. This can either be an inclusion or exclusion.
		 * An inclusion means the test will pass if any of the addresses in the callstack match the inclusion
		 * filter passed in. 
		 * 
		 * @param	FilterText	Filter to use
		 * @return	TRUE if callstack passes filter, FALSE otherwise
		 */
		public bool PassesTextFilterTest( string FilterText )
		{
			// Check whether any of the addresses in the call graph match the filter.
			bool bIsMatch = false;
			foreach( int AddressIndex in AddressIndices )
			{
				bIsMatch = FStreamInfo.GlobalInstance.NameArray[FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndex].FunctionIndex].ToUpperInvariant().Contains( FilterText );
				if( bIsMatch )
				{
					break;
				}
			}

			return bIsMatch;
		}

		/** 
		 * Filter callstacks in based on the text filter AND the class filter
		 */
		private bool FilterIn( string FilterInText, List<ClassGroup> ClassGroups )
		{
			// Check against the simple text filter
			if( ( FilterInText.Length == 0 ) || PassesTextFilterTest( FilterInText ) )
			{
				// Check against any active classes
				if( ClassGroups.Contains( Group ) )
				{
					return ( true );
				}
			}

			return ( false );
		}

		/** 
		 * Filter callstacks out based on the text filter AND the class filter
		 */
		private bool FilterOut( string FilterInText, List<ClassGroup> ClassGroups )
		{
			// Check against the simple text filter
			if( ( FilterInText.Length > 0 ) && PassesTextFilterTest( FilterInText ) )
			{
				// Found match, we do not want this callstack
				return ( false );
			}

			// Check against any active classes
			if( ClassGroups.Contains( Group ) )
			{
				return ( false );
			}

			return ( true );
		}

		/** 
		 * Runs all the current filters on this callstack
		 */
		public bool RunFilters( string FilterInText, List<ClassGroup> ClassGroups, bool bFilterInClasses )
		{
			// Create a list of active groups
			List<ClassGroup> ActiveGroups = new List<ClassGroup>();
			foreach( ClassGroup Group in ClassGroups )
			{
				if( Group.bFilter )
				{
					ActiveGroups.Add( Group );
				}
			}

			// Filter groups in or out
			if( bFilterInClasses )
			{
				return ( FilterIn( FilterInText, ActiveGroups ) );
			}
			else
			{
				return ( FilterOut( FilterInText, ActiveGroups ) );
			}
		}
    };
}	