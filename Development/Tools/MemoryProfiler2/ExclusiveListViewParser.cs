/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace MemoryProfiler2
{
	/**
	 * Class parsing snapshot into a tree from a given root node.
	 */
	public static class FExclusiveListViewParser
	{
		public static void ParseSnapshot( ListView ExclusiveListView, List<FCallStackAllocationInfo> CallStackList, MainWindow MainMProfWindow, bool bShouldSortBySize, string FilterText )
		{
			MainMProfWindow.UpdateStatus("Updating exclusive list view for " + MainMProfWindow.CurrentFilename);

			ExclusiveListView.BeginUpdate();

			// Sort based on passed in metric.
			if( bShouldSortBySize )
			{
                CallStackList.Sort( CompareAbsSize );
			}
			else
			{
				CallStackList.Sort( CompareCount );
			}

			bool bFilterIn = ( MainMProfWindow.FilterTypeSplitButton.Text == "Filter In" );

			// Figure out total size and count for percentages.
			long TotalSize = 0;
			long TotalCount = 0;
			foreach( FCallStackAllocationInfo AllocationInfo in CallStackList )
			{
				// Apply optional filter.
				if( FStreamInfo.GlobalInstance.CallStackArray[AllocationInfo.CallStackIndex].RunFilters( FilterText, MainMProfWindow.Options.ClassGroups, bFilterIn ) )
				{
					TotalSize += AllocationInfo.Size;
					TotalCount += AllocationInfo.Count;
				}
			}

			// Clear out existing entries and add top 400.
			ExclusiveListView.Items.Clear();
			for( int i=0; i<CallStackList.Count && ExclusiveListView.Items.Count <= 400; i++ )
			{
				FCallStackAllocationInfo AllocationInfo = CallStackList[i];

				// Apply optional filter.
                FCallStack CallStack = FStreamInfo.GlobalInstance.CallStackArray[AllocationInfo.CallStackIndex];
				if( CallStack.RunFilters( FilterText, MainMProfWindow.Options.ClassGroups, bFilterIn ) )
				{
					string CallSite = "";
					if( MainMProfWindow.ContainersSplitButton.Text == "Show Containers" )
					{
						FCallStackAddress Address = FStreamInfo.GlobalInstance.CallStackAddressArray[CallStack.AddressIndices[CallStack.AddressIndices.Count - 1]];
						CallSite = FStreamInfo.GlobalInstance.NameArray[Address.FunctionIndex];
					}
					else
					{
						FCallStackAddress Address = FStreamInfo.GlobalInstance.CallStackAddressArray[CallStack.AddressIndices[CallStack.FirstNonContainer]];
						CallSite = FStreamInfo.GlobalInstance.NameArray[Address.FunctionIndex];
					}

					string SizeInKByte		= String.Format( "{0:0}", (float) AllocationInfo.Size / 1024 ).PadLeft( 10, ' ' );
					string SizePercent		= String.Format( "{0:0.00}", (float) AllocationInfo.Size / TotalSize * 100 ).PadLeft( 10, ' ' );
					string Count			= String.Format( "{0:0}", AllocationInfo.Count ).PadLeft( 10, ' ' );
					string CountPercent		= String.Format( "{0:0.00}", (float) AllocationInfo.Count / TotalCount * 100 ).PadLeft( 10, ' ' );
					string GroupName		= ( CallStack.Group != null ) ? CallStack.Group.Name : "Ungrouped";

                    string[] Row = new string[]
					{
						SizeInKByte,
						SizePercent,
						Count,
						CountPercent,
						GroupName,
                        CallSite
					};

					ListViewItem Item = new ListViewItem(Row);
					Item.Tag = AllocationInfo;
					ExclusiveListView.Items.Add( Item );
				}
			}

			ExclusiveListView.EndUpdate();
		}

		/**
		 * Compare helper function, sorting FCallStackAllocation by size.
		 */
		private static int CompareSize( FCallStackAllocationInfo A, FCallStackAllocationInfo B )
		{
			return Math.Sign( B.Size - A.Size );
		}

        /**
         * Compare helper function, sorting FCallStackAllocation by abs(size).
         */
        private static int CompareAbsSize(FCallStackAllocationInfo A, FCallStackAllocationInfo B)
        {
            return Math.Sign(Math.Abs(B.Size) - Math.Abs(A.Size));
        }

		/**
		 * Compare helper function, sorting FCallStackAllocation by count.
		 */
		private static int CompareCount( FCallStackAllocationInfo A, FCallStackAllocationInfo B )
		{
			return Math.Sign( B.Count - A.Count );
		}	
	};
}