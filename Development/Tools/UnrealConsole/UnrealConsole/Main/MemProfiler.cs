using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

namespace UnrealConsole
{
	public class MemoryBlock
	{
		uint	Ptr;
		uint	Size;
		uint	Time;
		string	BackTrace;
		string	Tag;

		public MemoryBlock( MemoryBlock Block )
		{
			Ptr = Block.Ptr;
			Size = Block.Size;
			Time = Block.Time;
			BackTrace = Block.BackTrace;
			Tag = Block.Tag;
		}

		public MemoryBlock( uint InPtr, uint InSize, uint InTime, string InBackTrace, string InTag )
		{
			Ptr = InPtr;
			Size = InSize;
			Time = InTime;
			BackTrace = InBackTrace;
			Tag = InTag;
		}

		public uint GetAddress()
		{
			return( Ptr );
		}

		public uint GetTime()
		{
			return( Time );
		}

		public uint GetSize()
		{
			return( Size );
		}

		public string GetBackTrace()
		{
			return( BackTrace );
		}

		public string GetTag()
		{
			return( Tag );
		}
	}

	// A hash table of queues of allocations
	// Most of the time the queue will have 1 entry, but the allocs and frees can come out of order, and a queue will handle this.
	public class AllocationTracker
	{
		private Hashtable AddressToBlockMap = new Hashtable();

		public AllocationTracker()
		{
		}

		public AllocationTracker( AllocationTracker Old, uint StartTime )
		{
			foreach( Queue Addresses in Old.AddressToBlockMap.Values )
			{
				MemoryBlock Block = new MemoryBlock( ( MemoryBlock )Addresses.Peek() );
				if( Block.GetTime() > StartTime )
				{
					Add( Block.GetAddress(), Block );
				}
			}
		}

		public void Clear()
		{
			AddressToBlockMap.Clear();
		}

		public MemoryBlock GetMemoryBlock( uint key )
		{
			Debug.Assert( AddressToBlockMap.Contains( key ) );
			Queue Blocks = ( Queue )AddressToBlockMap[key];
			return( ( MemoryBlock )Blocks.Peek() );
		}

		public int GetCount()
		{
			return( AddressToBlockMap.Count );
		}

		public int GetCount( uint key )
		{
			return( ( ( Queue )AddressToBlockMap[key] ).Count );
		}

		public ICollection Keys  
		{
			get  
			{
				return( AddressToBlockMap.Keys );
			}
		}

		public ICollection Values  
		{
			get  
			{
				return( AddressToBlockMap.Values );
			}
		}

		public void Add( uint key, MemoryBlock value )  
		{
			if( !AddressToBlockMap.Contains( key ) )
			{
				AddressToBlockMap[key] = new Queue();
			}

			( ( Queue )AddressToBlockMap[key] ).Enqueue( value );
		}

		public void Remove( uint key )  
		{
			Queue Blocks = ( Queue )AddressToBlockMap[key];
			if (Blocks != null)
			{
				Blocks.Dequeue();
				if (Blocks.Count == 0)
				{
					AddressToBlockMap.Remove(key);
				}
			}
		}

		public bool Contains( uint key )  
		{
			return( AddressToBlockMap.Contains( key ) );
		}
	}

	public class BackTraceBlock
	{
		uint		BlockCount = 0;
		uint[]		BlockCounts;
		uint		BlockAllocated = 0;
		uint		AverageAge = 0;
		Queue		Addresses = null;

		public BackTraceBlock( BackTraceBlock Block )
		{
			BlockCount = Block.BlockCount;
			BlockCounts = Block.BlockCounts;
			BlockAllocated = Block.BlockAllocated;
			AverageAge = Block.AverageAge;
			Addresses = null;
		}

		public BackTraceBlock( MemoryBlock Alloc, uint TimeNow )
		{
			BlockCount = 1;
			BlockAllocated = Alloc.GetSize();
			if( TimeNow > 0 )
			{
				Debug.Assert( TimeNow >= Alloc.GetTime() );
				AverageAge = TimeNow - Alloc.GetTime();
			}
			else
			{
				AverageAge = Alloc.GetTime();
			}
			Addresses = new Queue();
			Addresses.Enqueue( Alloc.GetAddress() );
		}

		public void Update( MemoryBlock Alloc, uint TimeNow )
		{
			BlockCount++;
			BlockAllocated += Alloc.GetSize();
			if( TimeNow > 0 )
			{
				Debug.Assert( TimeNow >= Alloc.GetTime() );
				AverageAge += TimeNow - Alloc.GetTime();
			}
			else
			{
				AverageAge += Alloc.GetTime();
			}
			Addresses.Enqueue( Alloc.GetAddress() );
		}

		public uint[] ResetCounts( int Length )
		{
			int	i;

			BlockCounts = new uint[Length];
			for( i = 0; i < Length; i++ )
			{
				BlockCounts[i] = 0;
			}

			return( BlockCounts );
		}

		public Queue GetAddresses()
		{
			return( Addresses );
		}

		public uint GetCount()
		{
			return( BlockCount );
		}

		public uint GetAllocated()
		{
			return( BlockAllocated );
		}

		public string ToCSV()
		{
			return string.Format(
				"{0},{1},{2}",
				BlockCount,
				BlockAllocated,
				( AverageAge / BlockCount )
				);
		}

		public string ToCounts()
		{
			string Counts = "";
			foreach( uint Time in BlockCounts )
			{
				Counts += Time + ",";
			}

			Counts = Counts.TrimEnd( ',' );
			return( Counts );
		}
	}

	public class BackTraceTracker
	{
		private Hashtable CallstackDetails = new Hashtable();

		public BackTraceBlock this[ string key ]  
		{
			get  
			{
				return( ( BackTraceBlock )CallstackDetails[key] );
			}
			set  
			{
				CallstackDetails[key] = value;
			}
		}

		public int GetCount()
		{
			return( CallstackDetails.Count );
		}

		public ICollection Keys  
		{
			get  
			{
				return( CallstackDetails.Keys );
			}
		}

		public ICollection Values  
		{
			get  
			{
				return( CallstackDetails.Values );
			}
		}

		public void Add( string key, BackTraceBlock value )  
		{
			CallstackDetails.Add( key, value );
		}

		public bool Contains( string key )  
		{
			return( CallstackDetails.Contains( key ) );
		}

		public void Remove( string key )  
		{
			CallstackDetails.Remove( key );
		}
	}

	public class FMemoryTracker
	{
		private const int MAX_BACKTRACE_DEPTH = 10;
		private const string STATIC_INIT_STRING = "Static Init";

		private AllocationTracker MemoryAllocations = new AllocationTracker();
		BackTraceTracker MemoryBackTraces = null;
		BackTraceTracker PotentialLeaks = null;
		private Hashtable PerThreadMemoryTagStack = new Hashtable();

		private int MemoryAllocCount = 0;
		private int MemoryFreeCount = 0;
		private int MemoryReallocCount = 0;
		private Queue Baselines = new Queue();

		UnrealConsole ConsoleApp;
		RichTextBox TextOut;

		public FMemoryTracker(UnrealConsole InConsoleApp, RichTextBox InTextOut)
		{
			ConsoleApp = InConsoleApp;
			TextOut = InTextOut;
		}

		public void ClearCaptureData()
		{
			MemoryAllocCount = 0;
			MemoryFreeCount = 0;
			MemoryAllocations.Clear();
		}

		public void ClearOutput()
		{
			TextOut.Clear();
		}

		public void Print(string Text)
		{
			TextOut.AppendText(Text);
			TextOut.Refresh();
		}

		private string GetUniqueName( string Base )
		{
			string DateString = System.DateTime.Now.ToString( "yy-MM-dd_hh-mm" );
			string UniqueName = Base + ConsoleApp.TargetName + "-" + DateString + ".csv";
			return( UniqueName );
		}

		private void CollateBacktraces( AllocationTracker Allocations, uint TimeNow )
		{
			string Symbol;

			MemoryBackTraces = new BackTraceTracker();
			Hashtable AddressSymbolCache = new Hashtable();
			
			// Combine allocation with identical backtraces, and add them to the collection.
			foreach( Queue Blocks in Allocations.Values )
			{
				MemoryBlock Alloc = ( MemoryBlock )Blocks.Peek();
				string CompleteCallstack = "";

				// Add any info in the tag stack
				string Tag = Alloc.GetTag();
				if( Tag != null )
				{
					CompleteCallstack = "," + Tag;
				}
		
				// Add the callstack
				string BackTrace = Alloc.GetBackTrace();

				if( BackTrace.StartsWith( STATIC_INIT_STRING ) )
				{
					CompleteCallstack += "," + STATIC_INIT_STRING;
				}
				else
				{
					// Extract the addresses
					int index = 0;
					int length = BackTrace.Length;
					while( index * 8 < length && index < MAX_BACKTRACE_DEPTH )
					{
						uint Address = FromHex( BackTrace.Substring( index * 8, 8 ) );

						// Resolve the symbol for each address on the call stack.
						if( AddressSymbolCache.Contains( Address ) )
						{
							Symbol = ( string )AddressSymbolCache[Address];
						}
						else
						{
							Symbol = UnrealConsole.DLLInterface.ResolveAddressToString( Address );
							AddressSymbolCache.Add( Address, Symbol );
						}

						CompleteCallstack += "," + Symbol;
						index++;
					}
				}

				// Add to collection
				if( MemoryBackTraces.Contains( CompleteCallstack ) )
				{
					MemoryBackTraces[CompleteCallstack].Update( Alloc, TimeNow );
				}
				else
				{
					MemoryBackTraces.Add( CompleteCallstack, new BackTraceBlock( Alloc, TimeNow ) );
				}
			}

			Print( Allocations.GetCount() + " memory blocks collated into " + MemoryBackTraces.GetCount() + " backtraces.\r\n" );

			// Clean up
			AddressSymbolCache.Clear();
			AddressSymbolCache = null;
		}

		private void DumpMemoryBlocksToCSV()
		{
			string CSVName = GetUniqueName( ConsoleApp.PlatformName + "Memory-" );
			StreamWriter CSVFile = new StreamWriter( CSVName );
			Print( "\r\n ... writing to file \"" + CSVName + "\"\r\n\r\n" );

			// Write the headers
			CSVFile.Write( "Current count,Current size,Average age,Call stack\r\n" );

			// Write out the collated memory blocks
			foreach( string Callstack in MemoryBackTraces.Keys )
			{
				BackTraceBlock Block = MemoryBackTraces[Callstack];
				CSVFile.Write( Block.ToCSV() );
				CSVFile.Write( Callstack );
				CSVFile.Write( "\r\n" );
			}

			CSVFile.Close();
		}

		private void DumpCheckPointedMemoryBlocksToCSV( uint[] WorkTimes )
		{
			string CSVName = GetUniqueName(ConsoleApp.PlatformName + "Leaks-");
			StreamWriter CSVFile = new StreamWriter( CSVName );
			Print( "\r\n ... writing to file \"" + CSVName + "\"\r\n\r\n" );			

			// Write headers
			foreach( uint Count in WorkTimes )
			{
				CSVFile.Write( "Count @" + Count + "," );
			}
			CSVFile.Write( "Call stack\r\n" );

			// Write out the potential leaks
			foreach( string Callstack in PotentialLeaks.Keys )
			{
				BackTraceBlock Block = PotentialLeaks[Callstack];
				CSVFile.Write( Block.ToCounts() );
				CSVFile.Write( Callstack );
				CSVFile.Write( "\r\n" );
			}

			CSVFile.Close();
		}

		public void FindIncrementalBlocks( AllocationTracker WorkBlocks, uint[] WorkTimes )
		{
			int	i;

			Print( "Checking " + MemoryBackTraces.GetCount() + " backtraces for potential leaks.\r\n" );
			PotentialLeaks = new BackTraceTracker();

			foreach( string Callstack in MemoryBackTraces.Keys )
			{
				bool PotentialLeak = false;

				// Get collated data with queue of all allocations with this callstack
				BackTraceBlock Block = MemoryBackTraces[Callstack];
			
				// Clear out the counts
				uint[] BlockCounts = Block.ResetCounts( WorkTimes.Length );

				// Iterate over each allocation associated with this callstack
				foreach( uint Address in Block.GetAddresses() )
				{
					// Get block of memory
					MemoryBlock Alloc = WorkBlocks.GetMemoryBlock( Address );

					// Find which checkpoint it belongs to
					for( i = 1; i < WorkTimes.Length; i++ )
					{
						if( Alloc.GetTime() < WorkTimes[i] )
						{
							BlockCounts[i]++;
							PotentialLeak = true;
							break;
						}
					}
				}

				if( PotentialLeak )
				{
					// Every block count has to increment over time to be a leak
					for( i = 1; i < BlockCounts.Length - 1; i++ )
					{
						if( BlockCounts[i] < BlockCounts[i + 1] )
						{
							PotentialLeak = false;
						}
					}
				}

				// Remove this callstack if it's not a leak
				if( PotentialLeak )
				{
					PotentialLeaks.Add( Callstack, new BackTraceBlock( Block ) );
				}
			}

			Print( PotentialLeaks.GetCount() + " potential leaks detected.\r\n" );
		}

		public void DumpMemoryTracking(uint TimeNow)
		{

// 			// Initialize the symbol resolver.
// 			if( !CResolveCallStack.Init( DM.FindModule() ) )
// 			{
// 				Print( "No callstack info available!\r\n" );
// 			}
			
			CollateBacktraces( MemoryAllocations, TimeNow );

			// Dump all the data to a csv file
			DumpMemoryBlocksToCSV();
		}

		public void FindLeaks( uint TimeNow )
		{
			int	i;

			uint[] WorkTimes = new uint[Baselines.Count];
			i = 0;
			foreach( uint CheckPoint in Baselines )
			{
				WorkTimes[i] = CheckPoint;
				i++;
			}

			// Create a working set of allocations newer than the baseline
			AllocationTracker WorkBlocks = new AllocationTracker( MemoryAllocations, WorkTimes[0] );
			
			Print( WorkBlocks.GetCount() + " memory blocks newer than baseline time.\r\n" );

			// Create all the callstacks
			CollateBacktraces( WorkBlocks, TimeNow );

			// Find the blocks that increase in count over time
			FindIncrementalBlocks( WorkBlocks, WorkTimes );

			// Dump the remaining callstacks including counts over time
			DumpCheckPointedMemoryBlocksToCSV( WorkTimes );
		}

		/**
		 * Converts a hex string to a uint
		 */
		public static uint FromHex( string HexString )
		{
			HexString = HexString.ToLower();
			char[] HexArray = HexString.ToCharArray();
			int Value = 0;
			foreach( char Char in HexArray )
			{
				Value <<= 4;
				if( Char >= '0' && Char <= '9' )
				{
					Value += Char - '0';
				}
				else if( Char >= 'a' && Char <= 'f' )
				{
					Value += Char - 'a' + 10;
				}
				else
				{
					return( 0 );
				}
			}

			return( ( uint )Value );
		}

		/**
		 * Parses an individual memory tracking command, and processes it.
		 */
		private bool ProcessMemoryTrackingCommand(string Command)
		{
			if( Command.StartsWith( "&Reset" ) )
			{
				// Reset the memory allocation state.
				MemoryAllocations.Clear();
				PerThreadMemoryTagStack = new Hashtable();
				Baselines.Clear();

				MemoryAllocCount = 0;
				MemoryFreeCount = 0;
				MemoryReallocCount = 0;

				Print( "Resetting memory allocation state.\r\n" );
			}
			else if( Command.StartsWith( "&Allocate" ) )
			{
				if (Command.Length < 42 || ((Command.Length - 42) % 8) != 0)
				{
					return false;
				}
				uint Ptr = FromHex( Command.Substring( 9, 8 ) );
				uint Time = FromHex( Command.Substring( 17, 8 ) );
				uint Size = FromHex( Command.Substring( 25, 8 ) );
				uint ThreadId = FromHex( Command.Substring( 33, 8 ) );
				string BackTrace = Command.Substring( 42 );

				Stack MemoryTagStack = null;
				string Tag = null;
				if( PerThreadMemoryTagStack.Contains( ThreadId ) )
				{
					MemoryTagStack = ( Stack )PerThreadMemoryTagStack[ThreadId];
					if (MemoryTagStack.Count != 0)
					{
						Tag = (string)MemoryTagStack.Peek();
					}
				}

				MemoryAllocations.Add( Ptr, new MemoryBlock( Ptr, Size, Time, BackTrace, Tag ) );
				MemoryAllocCount++;
			}
			else if( Command.StartsWith( "&Free" ) )
			{
				if (Command.Length < 13)
				{
					return false;
				}
				uint Ptr = FromHex(Command.Substring(5, 8));

				MemoryAllocations.Remove( Ptr );
				MemoryFreeCount++;
			}
			else if( Command.StartsWith( "&Realloc" ) )
			{
				if (Command.Length < 48)
				{
					return false;
				}
				uint OldPtr = FromHex(Command.Substring(8, 8));
				uint NewPtr = FromHex( Command.Substring( 16, 8 ) );
				uint NewSize = FromHex( Command.Substring( 24, 8 ) );
				uint Time = FromHex( Command.Substring( 32, 8 ) );
				uint ThreadId = FromHex( Command.Substring( 40, 8 ) );

				string Backtrace = "";
				string Tag = null;

				if( !MemoryAllocations.Contains( OldPtr ) )
				{
					Backtrace = STATIC_INIT_STRING;
				}
				else
				{
					MemoryBlock Block = MemoryAllocations.GetMemoryBlock( OldPtr );
					Backtrace = Block.GetBackTrace();
					Tag = Block.GetTag();
					MemoryAllocations.Remove( OldPtr );
				}
				
				MemoryAllocations.Add( NewPtr, new MemoryBlock( NewPtr, NewSize, Time, Backtrace, Tag ) );
				MemoryReallocCount++;
			}
			else if( Command.StartsWith( "&PushTag" ) )
			{
				if (Command.Length < 16)
				{
					return false;
				}
				uint ThreadId = FromHex(Command.Substring(8, 8));
				string Tag = Command.Substring( 16 );

				if( !PerThreadMemoryTagStack.Contains( ThreadId ) )
				{
					PerThreadMemoryTagStack.Add( ThreadId, new Stack() );
				}

				( ( Stack )PerThreadMemoryTagStack[ThreadId] ).Push( Tag );
			}
			else if( Command.StartsWith( "&PopTag" ) )
			{
				if (Command.Length < 15)
				{
					return false;
				}
				uint ThreadId = FromHex(Command.Substring(7, 8));

				if( PerThreadMemoryTagStack.Contains( ThreadId ) )
				{
					if (((Stack)PerThreadMemoryTagStack[ThreadId]).Count > 0)
					{
						((Stack)PerThreadMemoryTagStack[ThreadId]).Pop();
					}
				}
			}
			else if( Command.StartsWith( "&DumpAllocs" ) )
			{
				if (Command.Length < 43)
				{
					return false;
				}
				uint Time = FromHex(Command.Substring(11, 8));	
				uint MallocCount = FromHex( Command.Substring( 19, 8 ) );	
				uint FreeCount = FromHex( Command.Substring( 27, 8 ) );	
				uint ReallocCount = FromHex( Command.Substring( 35, 8 ) );	

				Print( "Memory allocation dump at: " + Time + " ms\r\n" );
				Print( "Console reported: " + MallocCount + " mallocs, " + FreeCount + " frees and " + ReallocCount + " reallocs\r\n" );
				Print( "PC monitored    : " + MemoryAllocCount + " mallocs, " + MemoryFreeCount + " frees and " + MemoryReallocCount + " reallocs\r\n" );

				DumpMemoryTracking( Time );
			}
			else if( Command.StartsWith( "&FindLeaks" ) )
			{
				if (Command.Length < 18)
				{
					return false;
				}
				uint Time = FromHex(Command.Substring(10, 8));
				Print( "Finding leaks at " + Time + "ms (" + MemoryAllocations.GetCount() + " active allocations).\r\n" );
				FindLeaks( Time );
			}
			else if( Command.StartsWith( "&SetBaseline" ) )
			{
				if (Command.Length < 20)
				{
					return false;
				}
				uint Time = FromHex(Command.Substring(12, 8));
				Baselines.Enqueue( Time );
				
				if( Baselines.Count == 1 )
				{
					Print( "Set initial allocation state at " + Time + "ms (" + MemoryAllocations.GetCount() + " active allocations).\r\n" );
					Print( "Operations: " + MemoryAllocCount + " mallocs, " + MemoryFreeCount + " frees and " + MemoryReallocCount + " reallocs\r\n" );
				}
				else
				{
					int CheckPointNumber = Baselines.Count - 1;
					Print( "Set checkpoint " + CheckPointNumber + " at " + Time + "ms (" + MemoryAllocations.GetCount() + " active allocations)\r\n" );
					Print( "Operations: " + MemoryAllocCount + " mallocs, " + MemoryFreeCount + " frees and " + MemoryReallocCount + " reallocs\r\n" );
				}
			}

			return true;
		}

		/**
		 * Parses a list of commands delimited by & and terminated by $, calling ProcessMemoryTrackingCommand for each individual command.
		 */
		public void ProcessMemoryTrackingCommands( string Commands )
		{
			int BaseCommandIndex = 0;
			while( true )
			{
				Debug.Assert( BaseCommandIndex < Commands.Length );

				if( Commands[BaseCommandIndex] == '$' )
				{
					// The message must be terminated with a $.
					Debug.Assert( BaseCommandIndex == Commands.Length - 1 );
					break;
				}
				else
				{
					// if the message isn't proper, we must throw it away
					if (Commands[BaseCommandIndex] != '&')
					{
						Print("Bad tracking data, skipping command:\n  " + Commands.Substring(BaseCommandIndex) + "\n");
						break;
					}

					// Find the length of this command by searching for the next command delimiter or the end of the buffer.
					int NextCommandIndex = Commands.IndexOf( '&', BaseCommandIndex + 1 );
					int CommandLength = ( NextCommandIndex == -1 ? Commands.Length - 1 : NextCommandIndex ) - BaseCommandIndex;

					// Extract the individual command from the string and process it.
					if (!ProcessMemoryTrackingCommand( Commands.Substring( BaseCommandIndex, CommandLength ) ))
					{
						Print("Bad tracking data, skipping command:\n  " + Commands.Substring(BaseCommandIndex) + "\n");
						break;
					}

					// Advance to the next command.
					BaseCommandIndex += CommandLength;
				}
			}
		}
	}
}










/*


using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Windows.Forms;
using UnrealNetwork;

namespace UnrealConsole
{
	public class MemoryBlock
	{
		uint	Ptr;
		uint	Size;
		uint	Time;
		string	BackTrace;

		public MemoryBlock( uint InPtr, uint InSize, uint InTime, string InBackTrace )
		{
			Ptr = InPtr;
			Size = InSize;
			Time = InTime;
			BackTrace = InBackTrace;
		}

		public uint GetTime()
		{
			return( Time );
		}

		public uint GetSize()
		{
			return( Size );
		}

		public string GetBackTrace()
		{
			return( BackTrace );
		}

		public bool IsThisBackTrace( string BT )
		{
			return( BackTrace == BT );
		}
			
		public string ToString( uint BaseTime )
		{
			string Output;

			// Output the relevant info about the memory block
			Output = "Address: 0x" + Ptr.ToString( "x" ) + " : " + Size.ToString() + " (Age : " + ( ( BaseTime - Time ) / 1000 ).ToString() + "sec)";

			return( Output );
		}

		public string ToCSV( uint BaseTime )
		{
			string Output;

			// Output the relevant info about the memory block
			Output = Ptr.ToString() + "," + Size.ToString() + "," + ( ( BaseTime - Time ) / 1000 ).ToString();

			return( Output );
		}
	}


	public class AllocationTracker : DictionaryBase
	{
		public MemoryBlock this[ uint key ]  
		{
			get  
			{
				return( ( MemoryBlock )Dictionary[key] );
			}
			set  
			{
				Dictionary[key] = value;
			}
		}

		public ICollection Keys  
		{
			get  
			{
				return( Dictionary.Keys );
			}
		}

		public ICollection Values  
		{
			get  
			{
				return( Dictionary.Values );
			}
		}

		public bool Add( uint key, MemoryBlock value )  
		{
			try
			{
				Dictionary.Add( key, value );
			}
			catch (Exception)
			{
				// This means it was a duplicate
				return false;
			}
			return true;
		}

		public bool Contains( uint key )  
		{
			return( Dictionary.Contains( key ) );
		}

		public void Remove( uint key )  
		{
			Dictionary.Remove( key );
		}
	}

	public class BackTraceBlock
	{
		string[]	Callstack;
		uint		BlockCount;
		uint		BlockAllocated;
		uint		AverageAge;

		public BackTraceBlock()
		{
			BlockCount = 0;
			BlockAllocated = 0;
			AverageAge = 0;
		}

		public string[] GetCallstack()
		{
			return( Callstack );
		}

		public uint GetCount()
		{
			return( BlockCount );
		}

		public uint GetAllocated()
		{
			return( BlockAllocated );
		}

		public void IncBlockCount()
		{
			BlockCount++;
		}

		public void IncBlockAllocated( uint Size )
		{
			BlockAllocated += Size;
		}

		public void IncAge( uint Seconds )
		{
			AverageAge += Seconds;
		}

		public uint GetAverageAge()
		{
			return( AverageAge / BlockCount );
		}

		public void SetCallstack( string[] InCallstack )
		{
			Callstack = InCallstack;
		}

		public string ToCSV()
		{
			string Output = BlockCount + "," + BlockAllocated + "," + ( AverageAge / BlockCount );
			return( Output );
		}
	}

	public class BackTraceTracker : DictionaryBase
	{
		public BackTraceBlock this[ string key ]  
		{
			get  
			{
				return( ( BackTraceBlock )Dictionary[key] );
			}
			set  
			{
				Dictionary[key] = value;
			}
		}

		public ICollection Keys  
		{
			get  
			{
				return( Dictionary.Keys );
			}
		}

		public ICollection Values  
		{
			get  
			{
				return( Dictionary.Values );
			}
		}

		public void Add( string key, BackTraceBlock value )  
		{
			Dictionary.Add( key, value );
		}

		public bool Contains( string key )  
		{
			return( Dictionary.Contains( key ) );
		}

		public void Remove( string key )  
		{
			Dictionary.Remove( key );
		}
	}

	/// <summary>
	/// Summary description for MemProfiler.
	/// </summary>
	public class FMemProfiler
	{
		private const int			MAX_BACKTRACE_DEPTH = 10;
		private const int			GC_DURATION = 30000;

		private int					MemoryAllocCount = 0;
		private int					MemoryFreeCount = 0;
		private int					MemoryDuplicateMalloc = 0;
		private AllocationTracker	MemoryAllocations;
		private int					BlockAge = GC_DURATION;
		private RichTextBox			TextOut;
		private ServerConnection	Connection;
		private PlatformType		Platform;
		private string				ServerName;

		public FMemProfiler( RichTextBox TextBox )
		{
			TextOut = TextBox;

			// Create a map to store memory allocations
			MemoryAllocations = new AllocationTracker();
		}

		public bool IsCapturing( )
		{
			return (Connection != null);
		}

		public void StartCapture( ServerConnection InConnection )
		{
			Connection = InConnection;
			Platform = Connection.Platform;
			ServerName = Connection.Name;
			Print( "Memory capture started...\r\n" );
			Connection.SendCommand( "STARTTRACKING" );
		}

		public void StopCapture( )
		{
			Print( "Memory capture stopped!\r\n" );
			Connection.SendCommand( "STOPTRACKING" );
			Connection = null;
		}

		public void ClearCaptureData()
		{
			MemoryAllocCount = 0;
			MemoryFreeCount = 0;
			MemoryDuplicateMalloc = 0;
			MemoryAllocations.Clear();
		}

		public void ClearOutput( )
		{
			TextOut.Clear();
		}

		public void Print( string Text )
		{
			TextOut.AppendText( Text );
			TextOut.Refresh( );
		}

		private uint FromHex( Byte[] HexData, int Offset, int Length )
		{
			int Value = 0;
			for ( int Index=0; Index < Length; ++Index )
			{
				Value <<= 4;
				char Char = (char) HexData[ Offset + Index ];
				if( Char >= '0' && Char <= '9' )
				{
					Value += Char - '0';
				}
				else if( Char >= 'a' && Char <= 'f' )
				{
					Value += Char - 'a' + 10;
				}
				else if( Char >= 'A' && Char <= 'F' )
				{
					Value += Char - 'A' + 10;
				}
				else
				{
					return( 0 );
				}
			}
			return( ( uint )Value );
		}

		private uint FromHex( string HexData )
		{
			int Value = 0;
			for ( int Index=0; Index < HexData.Length; ++Index )
			{
				Value <<= 4;
				char Char = (char) HexData[ Index ];
				if( Char >= '0' && Char <= '9' )
				{
					Value += Char - '0';
				}
				else if( Char >= 'a' && Char <= 'f' )
				{
					Value += Char - 'a' + 10;
				}
				else if( Char >= 'A' && Char <= 'F' )
				{
					Value += Char - 'A' + 10;
				}
				else
				{
					return( 0 );
				}
			}
			return( ( uint )Value );
		}

		private void ExtractData( Byte[] Data, int Offset, int SizeSize, int DataSize, out uint Ptr, out string BackTrace, out MemoryBlock Block )
		{
			uint Time, Size;
			Ptr  = FromHex( Data, Offset+0, 8 );
			Time = FromHex( Data, Offset+8, 8 );
			Size = FromHex( Data, Offset+16, SizeSize ) << 2;
			Offset += 17 + SizeSize;
			DataSize -= 17 + SizeSize;
			BackTrace = Encoding.ASCII.GetString(Data, Offset, DataSize);
			Block = new MemoryBlock( Ptr, Size, Time, BackTrace );
		}

		private uint GetNewestBlockTime()
		{
			uint Newest = 0;

			ICollection Values = MemoryAllocations.Values;
			foreach( MemoryBlock Block in Values )
			{
				if( Block.GetTime() > Newest )
				{
					Newest = Block.GetTime();
				}
			}

			return( Newest );
		}

		private void CollateBlocks( uint Newest, BackTraceTracker MemoryBackTraces )
		{
			ICollection Blocks = MemoryAllocations.Values;
			foreach( MemoryBlock Block in Blocks )
			{
//				if( Newest - Block.GetTime() > BlockAge )
				{
					string BackTrace = Block.GetBackTrace();
					if( !MemoryBackTraces.Contains( BackTrace ) )
					{
						MemoryBackTraces.Add( BackTrace, new BackTraceBlock() );
					}

					MemoryBackTraces[BackTrace].IncBlockCount();
					MemoryBackTraces[BackTrace].IncBlockAllocated( Block.GetSize() );
					MemoryBackTraces[BackTrace].IncAge( ( Newest - Block.GetTime() ) / 1000 );
				}
			}
		}

		private void EvaluateBackTraces( BackTraceTracker MemoryBackTraces )
		{
			ICollection Blocks = MemoryBackTraces.Keys;
			foreach( string BackTrace in Blocks )
			{
				int			length, index;
				string[]	Callstack = new string [MAX_BACKTRACE_DEPTH];
				uint[]		BackTraceAddress = new uint [MAX_BACKTRACE_DEPTH];

				// Extract the addresses
				index = 0;
				length = BackTrace.Length;
				while( index * 8 < length && index < MAX_BACKTRACE_DEPTH )
				{
					BackTraceAddress[index] = FromHex( BackTrace.Substring( index * 8, 8 ) );
					index++;
				}

				// Construct a callstack
				index = 0;
				foreach( uint Address in BackTraceAddress )
				{
					if( Address != 0 )
					{
						Callstack[index] = UnrealConsole.DLLInterface.ResolveAddressToString( Address );
						index++;
					}
				}
			
				MemoryBackTraces[BackTrace].SetCallstack( Callstack );
			}
		}

		public void ParseMalloc( string Channel, Byte[] Data, int Offset, int DataSize )
		{
			uint		Ptr;
			int			SizeSize;
			string		BackTrace;
			MemoryBlock	Block;

			SizeSize = 1 << ( Channel[1] - '0' );
			ExtractData( Data, Offset, SizeSize, DataSize, out Ptr, out BackTrace, out Block );
			if ( !MemoryAllocations.Add( Ptr, Block ) )
			{
				MemoryDuplicateMalloc++;
			}
			MemoryAllocCount++;
		}

		public void ParseFree( Byte[] Data, int Offset )
		{
			uint Ptr = FromHex( Data, Offset+0, 8 );
			MemoryAllocations.Remove( Ptr );
			MemoryFreeCount++;
		}

		public void ParseGeneric( Byte[] Data, int Offset, int DataSize )
		{
			string Message = Encoding.ASCII.GetString(Data, Offset, DataSize);
			switch (Message)
			{
				case "TRACKINGSTOPPED":
					DumpMemoryTracking();
					break;
			}
		}

		public void DumpMemoryTracking()
		{
			// make sure we have some allocations and that the DLL can parse them
			if( MemoryAllocations.Count == 0)
			{
				return;
			}

			BackTraceTracker MemoryBackTraces = new BackTraceTracker();

			// Find the newest block for working out the age of each
			uint Newest = GetNewestBlockTime();

			// Find out the count and total usage of each backtrace
			CollateBlocks( Newest, MemoryBackTraces );

			// Evaluate the string of addresses to callstacks
			EvaluateBackTraces( MemoryBackTraces );

			// Dump the info to the log 
			// DumpMemoryBlocksToLog( Newest, MemoryBackTraces );

			// Dump all the data to a csv file
			DumpMemoryBlocksToCSV( Newest, MemoryBackTraces );

			UnrealConsole.DLLInterface.UnloadAllSymbols();

			// Clear up everything for a fresh start
			MemoryAllocations.Clear();
			MemoryAllocCount = 0;
			MemoryFreeCount = 0;
		}

		private void DumpMemoryBlocksToCSV( uint Newest, BackTraceTracker MemoryBackTraces )
		{
			string DateString = System.DateTime.Now.ToString( "yy-MM-dd_hh-mm" );
			string CSVName = Platform.ToString() + "Memory-" + ServerName + "-" + DateString + ".csv";
			StreamWriter CSVFile = new StreamWriter( CSVName );
			Print( "\r\n ... writing to file \"" + CSVName + "\"\r\n\r\n" );

			ICollection Blocks = MemoryBackTraces.Values;
			foreach( BackTraceBlock Block in Blocks )
			{
				CSVFile.Write( Block.ToCSV() );
				string[] BackTraceFunctions = Block.GetCallstack();

				foreach( string Function in BackTraceFunctions )
				{
					if( Function != null && Function.Length > 0 )
					{
						CSVFile.Write( "," + Function );
					}
				}
				CSVFile.Write( "\r\n" );
			}

			CSVFile.Close();
		}
	}
}


*/
