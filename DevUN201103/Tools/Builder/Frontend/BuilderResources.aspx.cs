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

public partial class BuilderResources : BasePage
{
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();

			using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue AS CPUBusy FROM PerformanceData " +
														"WHERE ( CounterID = 412 ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 60 ) " +
														"ORDER BY DateTimeStamp DESC", Connection ) )
			{
				SqlDataReader Reader = Command.ExecuteReader();
				if( Reader.HasRows )
				{
					DataTable Table = new DataTable();
					Table.Load( Reader );

					//RemoveOutliers( Table );

					BuildResourcesChart0.Series["CPUBusy"].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, "CPUBusy" );
				}
				Reader.Close();
			}

			using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue AS UsedMemory FROM PerformanceData " +
														"WHERE ( CounterID = 411 ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 60 ) " +
														"ORDER BY DateTimeStamp DESC", Connection ) )
			{
				SqlDataReader Reader = Command.ExecuteReader();
				if( Reader.HasRows )
				{
					DataTable Table = new DataTable();
					Table.Load( Reader );

					//RemoveOutliers( Table );

					BuildResourcesChart1.Series["UsedMemory"].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, "UsedMemory" );
				}
				Reader.Close();
			}

			Connection.Close();
		}
	}
}
