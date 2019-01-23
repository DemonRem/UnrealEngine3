using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Text;

namespace GameplayProfiler
{
	/**
	 * Hodling data associated with a UFunction. Also has static helper functionality
	 * to deal with parsing and displaying information.
	 */
	class FunctionInfo
	{
		/** Inclusive time of this function					*/
		public float InclusiveTime;
		/** Children time of this function					*/
		public float ChildrenTime;
		/** Number of times this function has been called	*/
		public int Calls;

		/** Maximum exclusive time.							*/
		public float MaxExclusiveTime;
		/** Maximum inclusive time.							*/
		public float MaxInclusiveTime;

		/** Frame number max exclusive time occured in.		*/
		public int FrameMaxExclusiveTimeOccured;
		/** Frame number max inclusive time occured in.		*/
		public int FrameMaxInclusiveTimeOccured;

		/**
		 * Constructor, initializing all member variables.
		 */
		public FunctionInfo( int FrameIndex, float InInclusiveTime, float InChildrenTime )
		{
			InclusiveTime	= InInclusiveTime;
			ChildrenTime	= InChildrenTime;
			Calls			= 1;
			MaxExclusiveTime = 0;
			MaxInclusiveTime = InclusiveTime;
			FrameMaxExclusiveTimeOccured = FrameIndex;
			FrameMaxInclusiveTimeOccured = FrameIndex;
		}
		
		/**
		 * Adds function info to passed in mapping and creates new entry if necessary.
		 * 
		 * @param	FrameIndex				Index of frame.
		 * @param	Function				Function to update/ add to mapping
		 * @param	NameToFunctionInfoMap	Map from function name to info that is being updated
		 */
		public static void AddFunction(int FrameIndex, TokenFunction Function, Dictionary<string, FunctionInfo> NameToFunctionInfoMap)
		{		
			string FunctionName = Function.GetFunctionName();	
			if( NameToFunctionInfoMap.ContainsKey(FunctionName) )
			{
				var FunctionInfo = NameToFunctionInfoMap[FunctionName];
				FunctionInfo.InclusiveTime += Function.InclusiveTime;
				FunctionInfo.ChildrenTime += Function.ChildrenTime;
				FunctionInfo.Calls++;
			}
			else
			{
				var FunctionInfo = new FunctionInfo( FrameIndex, Function.InclusiveTime, Function.ChildrenTime );
				NameToFunctionInfoMap.Add( FunctionName, FunctionInfo );
			}
		}

		/**
		 * Dumps the passed in function map into the passed in list view taking the time threshold into account.
		 */
		public static void DumpFunctionInfoMapToList( ProfilerStream ProfilerStream, Dictionary<string, FunctionInfo> NameToFunctionInfoMap, ListView FunctionListView, float TimeThreshold, bool bDumpAggregateInfo )
		{
			FunctionListView.BeginUpdate();
			FunctionListView.Items.Clear();

			// Add all functions to list.
			foreach( var FunctionNameAndInfo in NameToFunctionInfoMap )
			{
				var FunctionName = FunctionNameAndInfo.Key;
				var FunctionInfo = FunctionNameAndInfo.Value;

				float ExclusiveTime = FunctionInfo.InclusiveTime - FunctionInfo.ChildrenTime;
				int NumFrames = ProfilerStream.Frames.Count;
				float AvgCallsPerFrame = ((float)FunctionInfo.Calls) / NumFrames;

				if( FunctionInfo.InclusiveTime > TimeThreshold )
				{
					var Row = new List<string>();
					
					Row.Add(FunctionName);																// name
					Row.Add(FunctionInfo.InclusiveTime.ToString("F2").PadLeft(5));						// incl
					if( bDumpAggregateInfo )
					{
						Row.Add(FunctionInfo.MaxInclusiveTime.ToString("F2").PadLeft(6));				// max incl.
					}
					Row.Add(ExclusiveTime.ToString("F2").PadLeft(5));									// excl
					if( bDumpAggregateInfo )
					{
						Row.Add(FunctionInfo.MaxExclusiveTime.ToString("F2").PadLeft(6));				// max incl.
					}
					Row.Add(FunctionInfo.Calls.ToString().PadLeft(6));									// calls
					if( bDumpAggregateInfo )
					{
						Row.Add(AvgCallsPerFrame.ToString("F1").PadLeft(6));							// avg calls/ frame
						Row.Add((FunctionInfo.InclusiveTime/NumFrames).ToString("F2").PadLeft(5));		// incl/ frame
						Row.Add((ExclusiveTime/NumFrames).ToString("F2").PadLeft(5));					// excl/ frame
					}
					else
					{
						Row.Add((FunctionInfo.InclusiveTime / FunctionInfo.Calls).ToString("F2").PadLeft(5));	// incl/ call
						Row.Add((ExclusiveTime / FunctionInfo.Calls).ToString("F2").PadLeft(5));				// excl/ call
					}
					
					ListViewItem Item = new ListViewItem(Row.ToArray());
					Item.Tag = FunctionInfo;

					FunctionListView.Items.Add( Item );
				}
			}

			FunctionListView.Sort();
			FunctionListView.EndUpdate();
		}
	}
}
