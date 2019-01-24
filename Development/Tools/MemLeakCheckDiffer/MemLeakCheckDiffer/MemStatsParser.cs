/*
Log: Memory Allocation Status
Log: Allocs       445284 Current /  8686224 Total
Log: 
Log: DmQueryTitleMemoryStatistics
Log: TotalPages              122880
Log: AvailablePages          10278
Log: StackPages              436
Log: VirtualPageTablePages   67
Log: SystemPageTablePages    0
Log: VirtualMappedPages      42217
Log: ImagePages              7344
Log: FileCachePages          256
Log: ContiguousPages         62136
Log: DebuggerPages           0
Log: 
Log: GlobalMemoryStatus
Log: dwTotalPhys             536870912
Log: dwAvailPhys             42098688
Log: dwTotalVirtual          805175296
Log: dwAvailVirtual          628518912
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using EpicCommonUtilities;

namespace MemLeakDiffer
{
    class MemStatsReportSection : KeyValuePairSection
    {
        public const int INVALID_SIZE = -999999;

        public int TitleFreeKB = INVALID_SIZE;
        public int LowestRecentTitleFreeKB = INVALID_SIZE;
        public int AllocUnusedKB = 0;
        public int AllocUsedKB = 0;
        public int AllocPureOverheadKB = 0; // not being captured yet
		public double LowestFreeMemOccuredAgo = 0.0;

        /*
		public double PhysicalFreeMemKB = INVALID_SIZE;
		public double PhysicalUsedMemKB = INVALID_SIZE;
		public double TaskResidentKB = INVALID_SIZE;
		public double TaskVirtualKB = INVALID_SIZE;
         */

		public double HighestRecentMemoryAllocatedKB = INVALID_SIZE;
		public double HighestMemoryAllocatedKB = INVALID_SIZE;
		public double HighestMemOccurredAgo = 0.0;

        public Dictionary<string, MemPoolSet> Pools = new Dictionary<string, MemPoolSet>();

        public class MemPoolSet
        {
            /// <summary>
            /// Memory zone type (Physical 'Cached', Physical 'WriteCombined', 'Virtual')
            /// </summary>
            public string SetType;

            /// <summary>
            /// List of blocks, each providing an allocation pool for a single discrete size
            /// </summary>
            public List<MemPoolLine> Blocks = new List<MemPoolLine>();

            /// <summary>
            /// Create a clone of this pool set and return it
            /// </summary>
            public MemPoolSet Clone()
            {
                MemPoolSet Result = new MemPoolSet();

                Result.SetType = SetType;
                foreach (MemPoolLine BlockEntry in Blocks)
                {
                    Result.Blocks.Add(BlockEntry.Clone());
                }

                return Result;
            }

            /// <summary>
            /// Returns the total memory for this pool where
            ///    TotalMemUsed = TotalGood + TotalWaste;
            ///    Efficiency = TotalGood/(TotalGood + TotalWaste) * 100%
            /// </summary>
            public void Calculate(out int TotalGood, out int TotalWaste)
            {
                // Calculate waste and total size
                TotalWaste = 0;
                TotalGood = 0;

                foreach (MemPoolLine BlockEntry in Blocks)
                {
                    int Waste;
                    int Good;
                    BlockEntry.Calculate(out Good, out Waste);
                    TotalGood += Good;
                    TotalWaste += Waste;
                }
            }

            /// <summary>
            /// Adds a new pool for a given block and element size.
            /// 
            /// Multiple calls to this method should be done from smallest to largest, otherwise GetPoolForSize will be non-optimal
            /// </summary>
            /// <param name="BlockSize"></param>
            /// <param name="ElementSize"></param>
            public void AddEmptyPool(int BlockSize, int ElementSize)
            {
                MemPoolLine NewBlock = new MemPoolLine();
                NewBlock.BlockSize = BlockSize;
                NewBlock.ElementSize = ElementSize;

                // Verify that this block is being added in the correct order
                if (Blocks.Count > 0)
                {
                    Debug.Assert(Blocks[Blocks.Count - 1].ElementSize < NewBlock.ElementSize);
                }
                Blocks.Add(NewBlock);
            }

            /// <summary>
            /// Construct an empty pool set
            /// </summary>
            public MemPoolSet()
            {
            }

            /// <summary>
            /// Construct a pool set with no allocations from an array of element sizes
            /// </summary>
            /// <param name="BlockSize">Backing allocation granularity.  This is the size of each OS request when an individual element pool needs more memory.</param>
            /// <param name="PoolSizeList">List of element sizes, which should be strictly-positive and in sorted ascending order</param>
            public MemPoolSet(int BlockSize, int[] PoolSizeList)
            {
                int LastSize = 0;
                foreach (int Size in PoolSizeList)
                {
                    // Verify strictly-positive, ascending order
                    Debug.Assert(LastSize < Size);
                    LastSize = Size;

                    // Create a pool for it
                    AddEmptyPool(BlockSize, Size);
                }
            }

            /// <summary>
            /// Returns the smallest pool that can satisfy a request of Size (caring only about ElementSize, not actual free elements available ATM)
            /// </summary>
            public MemPoolLine GetPoolForSize(int Size)
            {
                // Working with the assumption that the pools are sorted by size
                foreach (MemPoolLine Block in Blocks)
                {
                    if (Block.ElementSize >= Size)
                    {
                        return Block;
                    }
                }

                return null;
            }

            /// <summary>
            /// Casts all allocations from Source into this pool.  Throws an exception if any cannot be fit
            /// </summary>
            public void Recast(MemPoolSet Source)
            {
                foreach (MemPoolLine SrcPool in Source.Blocks)
                {
                    MemPoolLine DestPool = GetPoolForSize(SrcPool.ElementSize);
                    if (DestPool == null)
                    {
                        throw new Exception("No pools were large enough for a source allocation");
                    }
                    DestPool.AddElements(SrcPool.CurAllocs);
                }
            }

            public void MakeEmpty()
            {
                foreach (MemPoolLine Block in Blocks)
                {
                    Block.MakeEmpty();
                }
            }
        }

        /// <summary>
        /// Represents a single pool for a fixed allocation size, being doled out from one of a set of pages.
        /// </summary>
        public class MemPoolLine
        {
            // There are more fields, but they're all derived values
            public int BlockSize = 65536;
            public int ElementSize;
            public int NumBlocks;
            public int CurAllocs;

            public void MakeEmpty()
            {
                NumBlocks = 0;
                CurAllocs = 0;
            }

            public MemPoolLine Clone()
            {
                MemPoolLine result = new MemPoolLine();
                result.BlockSize = BlockSize;
                result.ElementSize = ElementSize;
                result.NumBlocks = NumBlocks;
                result.CurAllocs = CurAllocs;
                return result;
            }

            public void AddElements(int Count)
            {
                CurAllocs += Count;
                int AllocsPerBlock = BlockSize / ElementSize;
                NumBlocks = (CurAllocs + (AllocsPerBlock - 1)) / AllocsPerBlock;
            }

            public MemPoolLine()
            {
                ElementSize = BlockSize;
                NumBlocks = 0;
                CurAllocs = 0;
            }

            public void Calculate(out int TotalBytes, out int TotalWaste)
            {
                TotalWaste = NumBlocks * BlockSize - CurAllocs * ElementSize;
                TotalBytes = CurAllocs * ElementSize;
            }

            public MemPoolLine(string SourceTextLine, int CurAllocIndex)
            {
                // Remove KB indicators
                SourceTextLine = SourceTextLine.Replace('K', ' ');

                // Split into cells
                char[] Separators = {' '};
                string[] Cells = SourceTextLine.Split(Separators, StringSplitOptions.RemoveEmptyEntries);

                // Grab the interesting cells
                ElementSize = int.Parse(Cells[0]);
                NumBlocks = int.Parse(Cells[1]);
                CurAllocs = int.Parse(Cells[CurAllocIndex]);
            }
        }
    }


    /// <summary>
    /// Parser for the mem stats report from a pooled allocator
    /// </summary>
    class MemStatsParser : SmartParser
    {
        /// <summary>
        /// Parses one sub-report for a given memory type (header + a set of lines, one for each element size)
        /// </summary>
        public MemStatsReportSection.MemPoolSet ParsePool(string StartingLine, string[] Items, ref int i)
        {
            MemStatsReportSection.MemPoolSet Pool = new MemStatsReportSection.MemPoolSet();
            Pool.SetType = StartingLine.Substring("CacheType ".Length);

            string Headers;
            ReadLine(Items, ref i, out Headers);
            Ensure(Items, ref i, "----------");

            // Determine the current allocation column (varies depending on the age of the memleakcheck file

            // Newer Rift-era: Block Size Num Pools Max Pools Cur Allocs Total Allocs Min Req Max Req Mem Used Mem Align Efficiency
            // Rift-era:   Block Size Num Pools Available Exhausted Cur Allocs Mem Used  Mem Waste Efficiency  Page Loss
            // Gears2-era: Block Size Num Pools Cur Allocs Mem Used  Mem Waste Efficiency
            int CurAllocColumn = 2;
            if (Headers.Contains("Available Exhausted"))
            {
                CurAllocColumn += 2;
            }
            else if (Headers.Contains("Cur Allocs Total Allocs"))
            {
                CurAllocColumn += 1;
            }

            bool bEndOfSection = false;
            do
            {
                string line;
                bEndOfSection = !ReadLine(Items, ref i, out line);
                Debug.Assert(!bEndOfSection);

                if (line.StartsWith("BlkOverall") || (line.Trim() == ""))
                {
                    bEndOfSection = true;
                }
                else
                {
                    Pool.Blocks.Add(new MemStatsReportSection.MemPoolLine(line, CurAllocColumn));
                }
            }
            while (!bEndOfSection);

            return Pool;
        }

        // Parses "[Used / MaxSize] KByte" and returns Used, MaxSize
        protected void ParseStackUsageValues(string ValueOfStackLine, out int UsedKB, out int MaxSizeKB)
        {
            int OpenBracket = ValueOfStackLine.IndexOf('[');
            int CloseBracket = ValueOfStackLine.IndexOf(']');
            int Divider = ValueOfStackLine.IndexOf('/');

            string UsedString = ValueOfStackLine.Substring(OpenBracket + 1, Divider - OpenBracket - 1).Trim();
            string MaxSizeString = ValueOfStackLine.Substring(Divider + 1, CloseBracket - Divider - 1).Trim();

            UsedKB = int.Parse(UsedString);
            MaxSizeKB = int.Parse(MaxSizeString);
        }


        public override ReportSection Parse(string[] Items, ref int i)
        {
            MemStatsReportSection Result = new MemStatsReportSection();
            Result.MyHeading = "Memory Allocation Status";

            int TitleFreePages = MemStatsReportSection.INVALID_SIZE;
            int RecentLowestPages = MemStatsReportSection.INVALID_SIZE;
			int PhysicalFreeMem = MemStatsReportSection.INVALID_SIZE;
			int PhysicalUsedMem = MemStatsReportSection.INVALID_SIZE;
			int TaskResident = MemStatsReportSection.INVALID_SIZE;
			int TaskVirtual = MemStatsReportSection.INVALID_SIZE;
			int RecentHighestAllocatedMemory = MemStatsReportSection.INVALID_SIZE;
			int HighestAllocatedMemoryEver = MemStatsReportSection.INVALID_SIZE;
            string GMainThreadMemStackString = null;
            string GRenderThreadMemStackString = null;

            // Read the rest of the lines, ignoring most of them
            string line;
            while (ReadLine(Items, ref i, out line))
            {
                // Grab the TFP when it appears (or the recent lowest if it's there as well)
                ParseLineIntInMiddle(line, "AvailablePages", ref TitleFreePages);
                ParseLineIntInMiddle(line, "RecentLowestAvailPages", ref RecentLowestPages);
				ParseLineFloatInMiddle(line, "RecentLowestOccuredAgo", ref Result.LowestFreeMemOccuredAgo);
				ParseLineIntInMiddle(line, "dwPhysicalFree", ref PhysicalFreeMem);
				ParseLineIntInMiddle(line, "dwPhysicalUsed", ref PhysicalUsedMem);
				ParseLineIntInMiddle(line, "dwTaskResident", ref TaskResident);
				ParseLineIntInMiddle(line, "dwTaskVirtual", ref TaskVirtual);
				ParseLineIntInMiddle(line, "RecentHighestAllocatedMemory", ref RecentHighestAllocatedMemory);
				ParseLineFloatInMiddle(line, "RecentHighestOccurredAgo", ref Result.HighestMemOccurredAgo);
				ParseLineIntInMiddle(line, "HighestAllocatedMemoryEver", ref HighestAllocatedMemoryEver);
                ParseLineKeyValuePair(line, "GMainThreadMemStack allocation size [used/ unused]", ref GMainThreadMemStackString);
                ParseLineKeyValuePair(line, "GRenderingThreadMemStack allocation size [used/ unused]", ref GRenderThreadMemStackString);

                // Collect waste lines
                int UnusedMem = 0;
                if (ParseLineIntInMiddle(line, "TOTAL UNUSED MEMORY:", ref UnusedMem))
                {
                    Result.AllocUnusedKB += UnusedMem;
                }

                // Go into pool parsing mode if it's the start of a new pool
                if (line.StartsWith("CacheType"))
                {
                    MemStatsReportSection.MemPoolSet Pool = ParsePool(line, Items, ref i);
                    Result.Pools.Add(Pool.SetType, Pool);
                }
            }

            Result.TitleFreeKB = TitleFreePages * 4;
            Result.LowestRecentTitleFreeKB = (RecentLowestPages != MemStatsReportSection.INVALID_SIZE) ? RecentLowestPages * 4 : Result.TitleFreeKB;

            if (PhysicalFreeMem != MemStatsReportSection.INVALID_SIZE)
            {
                // Add iOS samples if available
                Result.AddSample("iOS_PhysicalFreeMem", -8, (double)PhysicalFreeMem / 1024.0, EStatType.Minimum);
                Result.AddSample("iOS_PhysicalUsedMem", -7, (double)PhysicalUsedMem / 1024.0, EStatType.Minimum);
                Result.AddSample("iOS_TaskResident", -8, (double)TaskResident / 1024.0, EStatType.Minimum);
                Result.AddSample("iOS_TaskVirtual", -8, (double)TaskVirtual / 1024.0, EStatType.Minimum);
            }
			Result.HighestRecentMemoryAllocatedKB = (double)RecentHighestAllocatedMemory / 1024.0;
			Result.HighestMemoryAllocatedKB = (double)HighestAllocatedMemoryEver / 1024.0;

            const int MiscStatPriority = 21;

            //Log: GMainThreadMemStack allocation size [used/ unused] = [0 / 127] KByte
            // Still has a value string of [0 / 127] KByte
            if (GMainThreadMemStackString != null)
            {
                int UsedKB;
                int MaxSizeKB;
                ParseStackUsageValues(GMainThreadMemStackString, out UsedKB, out MaxSizeKB);

                Result.AddSample("GMainThreadMemStackUsed", MiscStatPriority, UsedKB, EStatType.Maximum);
                Result.AddSample("GMainThreadMemStackMax", MiscStatPriority, MaxSizeKB, EStatType.Maximum);
            }

            if (GRenderThreadMemStackString != null)
            {
                int UsedKB;
                int MaxSizeKB;
                ParseStackUsageValues(GRenderThreadMemStackString, out UsedKB, out MaxSizeKB);

                Result.AddSample("GRenderingThreadMemStackUsed", MiscStatPriority, UsedKB, EStatType.Maximum);
                Result.AddSample("GRenderingThreadMemStackMax", MiscStatPriority, MaxSizeKB, EStatType.Maximum);
            }

            foreach (MemStatsReportSection.MemPoolSet pool in Result.Pools.Values)
            {
                int Good;
                int Waste;
                pool.Calculate(out Good, out Waste);

                Result.AllocUsedKB += Good / 1024;
            }

            if (Result.AllocUnusedKB != 0)
            {
                Result.AddSample("AllocatorUnused", -9, Result.AllocUnusedKB, EStatType.Maximum);
            }

            if (Result.AllocUsedKB != 0)
            {
                Result.AddSample("AllocatorUsed", -9, Result.AllocUsedKB, EStatType.Maximum);
            }
            
            return Result;
        }
    }


    class MemStatsReportSectionPS3 : TrueKeyValuePairSection
    {
        public static int PS3AdjustKB = 0;

        protected bool bSeenHostMemoryInfo = false;

        public MemStatsReportSectionPS3()
            : base("Memory Allocation Status (PS3)")
        {
            // Lines are of the form "System memory total:           210.00 MByte (220200960 Bytes)"
            Separators = new char[] { ':' };

            MyHeading = "Memory Allocation Status (PS3)";
            MyBasePriority = -8;
            bAdditiveMode = false;

            AddMappedSample("System memory total", "SystemMemoryTotal", EStatType.Minimum);
            AddMappedSample("System memory available", "SystemMemoryAvailable", EStatType.Minimum);
            AddMappedSample("Available for malloc", "SystemMemoryAvailableForMalloc", EStatType.Minimum);
            AddMappedSample("Total expected overhead", "AllocatorTotalExpectedOverhead", EStatType.Maximum);
            AddMappedSample("AllocatedRSXHostMemory", "AllocatedRSXHostMemory", EStatType.Maximum);
            AddMappedSample("AllocatedRSXLocalMemory", "AllocatedRSXLocalMemory", EStatType.Maximum);
        }

        protected override void AllowKeyValueAdjustment(ref string Key, ref string Value)
        {
            // PS3 memory report has total allocated twice, for a field we care about!
            // Log: GPU memory info:               249.00 MByte total
            // Log:   Total allocated:             232.03 MByte (243304208 Bytes)
            // Log: 
            // Log: Host memory info:               10.00 MByte total
            // Log:   Total allocated:               6.64 MByte (6960288 Bytes)
            if (Key == "Host memory info")
            {
                bSeenHostMemoryInfo = true;
            }
            else if (Key == "Total allocated")
            {
                if (bSeenHostMemoryInfo)
                {
                    Key = "AllocatedRSXHostMemory";
                }
                else
                {
                    Key = "AllocatedRSXLocalMemory";
                }
            }
        }

        /// <summary>
        /// Cooks the data
        /// </summary>
        public override void Cook()
        {
            base.Cook();

            // All of the read values are in MByte, so we normalize to KB here
            Dictionary<string, SampleRecord> NewValues = new Dictionary<string, SampleRecord>();
            foreach (var KVP in Samples)
            {
                NewValues[KVP.Key] = new SampleRecord(KVP.Value.Sample * 1024.0f, KVP.Value.Priority, KVP.Value.OverviewStatType);
            }
            Samples = NewValues;


            // Subtract off the devkit/testkit adjustment value from the total main memory readings
            Samples.Add("PS3AdjustmentSetting", new SampleRecord(PS3AdjustKB, -8, EStatType.Maximum));
            Samples["SystemMemoryAvailable"].Sample -= PS3AdjustKB;
            Samples["SystemMemoryAvailableForMalloc"].Sample -= PS3AdjustKB;
            Samples["SystemMemoryTotal"].Sample -= PS3AdjustKB;
        }
    }

    class MemStatsParserPS3 : SmartParser
    {
        public override ReportSection Parse(string[] Items, ref int i)
        {
            MemStatsReportSectionPS3 Result = new MemStatsReportSectionPS3();
            Result.Parse(Items, ref i);
            return Result;
        }
    }
}
