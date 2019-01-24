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

public partial class PCSCompileTimes : BasePage
{
	private void FillSeries( SqlConnection Connection, string Series, string Item, int CounterID )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue / 60000 AS " + Item + " FROM PerformanceData " +
													"WHERE ( CounterID = " + CounterID.ToString() + " ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 180 ) " +
													"ORDER BY DateTimeStamp DESC", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				RemoveOutliers( Table );

				PCSCompileChart.Series[Series].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, Item );
			}

			Reader.Close();
		}
	}	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "Rift Xbox360", "Rift360", 714 );
			FillSeries( Connection, "Rift PS3", "RiftPS3", 712 );
			FillSeries( Connection, "Rift SM3", "RiftSM3", 889 );
			Connection.Close();
		}
	}
}
