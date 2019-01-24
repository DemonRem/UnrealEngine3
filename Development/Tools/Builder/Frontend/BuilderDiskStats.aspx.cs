/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Data.SqlClient;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

public partial class BuilderDiskStats : BasePage
{
	private void FillSeries( SqlConnection Connection, string Item, int CounterID )
	{
		string MinuteOfDayQuery = "DATEDIFF( minute, FLOOR( CAST( DATEADD( hour, DATEDIFF( hour, GETUTCDATE(), GETDATE() ), DateTimeStamp ) AS FLOAT ) ), DATEADD( hour, DATEDIFF( hour, GETUTCDATE(), GETDATE() ), DateTimeStamp ) )";
		using( SqlCommand Command = new SqlCommand(
			"SELECT DATEADD( minute, " + MinuteOfDayQuery + ", '1970-01-01' ) AS MinuteOfDay, AVG( IntValue * 1.0 ) AS " + Item + " " +
			"FROM PerformanceData " +
			"WHERE ( CounterID = " + CounterID.ToString() + ") AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 60 ) " +
			"GROUP BY " + MinuteOfDayQuery +
			"ORDER BY MinuteOfDay", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				DiskLatencyChart.Series[Item].Points.DataBindXY( Table.Rows, "MinuteOfDay", Table.Rows, Item );
			}
			Reader.Close();
		}
	}

	private void FillQueueSeries( SqlConnection Connection, string Item, int CounterID )
	{
		string MinuteOfDayQuery = "DATEDIFF( minute, FLOOR( CAST( DATEADD( hour, DATEDIFF( hour, GETUTCDATE(), GETDATE() ), DateTimeStamp ) AS FLOAT ) ), DATEADD( hour, DATEDIFF( hour, GETUTCDATE(), GETDATE() ), DateTimeStamp ) )";
		using( SqlCommand Command = new SqlCommand(
			"SELECT DATEADD( minute, " + MinuteOfDayQuery + ", '1970-01-01' ) AS MinuteOfDay, AVG( IntValue / 100.0 ) AS " + Item + " " +
			"FROM PerformanceData " +
			"WHERE ( CounterID = " + CounterID.ToString() + ") AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 60 ) " +
			"GROUP BY " + MinuteOfDayQuery +
			"ORDER BY MinuteOfDay", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				DiskQueueChart.Series[Item].Points.DataBindXY( Table.Rows, "MinuteOfDay", Table.Rows, Item );
			}
			Reader.Close();
		}
	}

	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();

			FillSeries( Connection, "DiskReadLatency", 413 );
			FillSeries( Connection, "DiskWriteLatency", 414 );
			FillSeries( Connection, "DiskTransferLatency", 415 );

			FillQueueSeries( Connection, "DiskQueueLength", 416 );
			FillQueueSeries( Connection, "DiskReadQueueLength", 417 );

			Connection.Close();
		}
	}
}
