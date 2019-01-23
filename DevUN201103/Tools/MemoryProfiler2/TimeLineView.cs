/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization;
using System.Windows.Forms.DataVisualization.Charting;

namespace MemoryProfiler2
{
	public static class FTimeLineChartView
	{
		private static void AddSeries( Chart TimeLineChart, string SeriesName, FStreamSnapshot Snapshot, SliceTypes SliceType )
		{
			Series ChartSeries = TimeLineChart.Series[SeriesName];
			if( ChartSeries != null )
			{
				ChartSeries.Points.Clear();

				foreach( FMemorySlice Slice in Snapshot.OverallMemorySlice )
				{
					DataPoint Point = new DataPoint();
					Point.YValues[0] = Slice.GetSliceInfo( SliceType ) / ( 1024.0 * 1024.0 );
					ChartSeries.Points.Add( Point );
				}
			}
		}

		public static void ParseSnapshot( Chart TimeLineChart, FStreamSnapshot Snapshot )
		{
			AddSeries( TimeLineChart, "Allocated Memory", Snapshot, SliceTypes.TotalUsed );
			AddSeries( TimeLineChart, "Virtual Used", Snapshot, SliceTypes.CPUUsed );
			AddSeries( TimeLineChart, "Virtual Slack", Snapshot, SliceTypes.CPUSlack );
			AddSeries( TimeLineChart, "Virtual Waste", Snapshot, SliceTypes.CPUWaste );
			AddSeries( TimeLineChart, "Physical Used", Snapshot, SliceTypes.GPUUsed );
			AddSeries( TimeLineChart, "Physical Slack", Snapshot, SliceTypes.GPUSlack );
			AddSeries( TimeLineChart, "Physical Waste", Snapshot, SliceTypes.GPUWaste );
			AddSeries( TimeLineChart, "OS Overhead", Snapshot, SliceTypes.OSOverhead );
			AddSeries( TimeLineChart, "Image Size", Snapshot, SliceTypes.ImageSize );
		}
	}
}

