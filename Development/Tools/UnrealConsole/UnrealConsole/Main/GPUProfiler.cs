
using System;
using System.IO;


namespace UnrealConsole
{
	/// <summary>
	/// Summary description for GPUProfiler.
	/// </summary>
	public class FGPUProfiler
	{
		public FGPUProfiler()
		{
		}

		public void ConvertToExcel( Stream InputStream, string ExcelFilename )
		{
			BinaryReader Reader = new BinaryReader( InputStream );
			StreamWriter ExcelStream = new StreamWriter( ExcelFilename );

			UInt32 Cookie	  = Read32(Reader);
			UInt32 Version    = Read32(Reader);
			Int64 CPURef	  = (Int64) Read64(Reader);
			Int64 GPURef	  = (Int64) Read64(Reader);
			Int64 TimeTolerance = (Int64) Read64(Reader);
			UInt32 GPUStart	  = Read32(Reader);
			UInt32 GPUEnd	  = Read32(Reader);
			UInt32 CPUStart	  = Read32(Reader);
			UInt32 CPUEnd	  = Read32(Reader);
			UInt32 NumSamples = Read32(Reader);
			Int64 GPUOrigin	  = (Int64) Read64(Reader);
			Int64 GPU2CPU	  = GPUOrigin - GPURef + CPURef - CPUStart;
			GPUStart		  = (UInt32) (GPUStart + GPU2CPU);
			GPUEnd			  = (UInt32) (GPUEnd + GPU2CPU);
			ExcelStream.WriteLine( "CPU start time,{0:f2}", 0.0 );
			ExcelStream.WriteLine( "CPU duration,{0:f2}", (CPUEnd-CPUStart)/1000.0 );
			ExcelStream.WriteLine( "GPU duration,{0:f2}", (GPUEnd-GPUStart)/1000.0 );
			ExcelStream.WriteLine( "Time tolerance,{0:f2}", (TimeTolerance)/1000.0 );
			ExcelStream.WriteLine( "NumSamples,{0}", NumSamples );
			ExcelStream.WriteLine( "" );
			ExcelStream.WriteLine( "Event:,CPU Start:,CPU Duration:,GPU Start:,GPU Duration:,GPU Lag:" );
			for ( int Sample=0; Sample < NumSamples; ++Sample )
			{
				Char ch;
				String EventName = "";
				while ( (ch=Reader.ReadChar()) != '\0' )
				{
					EventName += ch;
				}
				Int64 CPUTimestamp	= Read32(Reader);				// All CPU timestamps are based off of CPUStart.
				Int64 CPUDuration	= Read32(Reader);
				Int64 GPUTimestamp	= Read32(Reader) + GPU2CPU;		// All GPU timestamps are based off of GPUOrigin, make them based off of CPUStart as well.
				Int64 GPUDuration	= Read32(Reader);
				ExcelStream.WriteLine( "{0},{1,5:f2},{2,5:f2},{3,5:f2},{4,5:f2},{5,5:f2}",
					EventName, CPUTimestamp/1000.0, CPUDuration/1000.0, GPUTimestamp/1000.0, GPUDuration/1000.0,
					(GPUTimestamp+GPUDuration-CPUTimestamp-CPUDuration)/1000.0 );
			}

			ExcelStream.Close();
			Reader.Close();
		}

		private UInt32 Read32(BinaryReader Reader)
		{
			UInt32 Value = Reader.ReadUInt32();
			return (Value>>24) | (Value<<24) | ((Value&0x00ff0000)>>8) | ((Value&0x0000ff00)<<8);
		}

		private UInt64 Read64(BinaryReader Reader)
		{
			UInt64 Value = Reader.ReadUInt64();
			return ((Value&0xff00000000000000)>>56) | 
				((Value&0x00ff000000000000)>>40) |
				((Value&0x0000ff0000000000)>>24) |
				((Value&0x000000ff00000000)>> 8) |
				((Value&0x00000000ff000000)<< 8) |
				((Value&0x0000000000ff0000)<<24) |
				((Value&0x000000000000ff00)<<40) |
				((Value&0x00000000000000ff)<<56);
		}
	}
}
