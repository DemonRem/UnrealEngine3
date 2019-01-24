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

public partial class RiftCookTimes : BasePage
{
	private void FillSeries( SqlConnection Connection, string Item, int CounterID )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue / 60000 AS " + Item + " FROM PerformanceData " +
													"WHERE ( CounterID = " + CounterID.ToString() + " ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 360 ) " +
													"ORDER BY DateTimeStamp DESC", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				RemoveOutliers( Table );

				RiftCookChart.Series[Item].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, Item );
			}

			Reader.Close();
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "RiftCookXbox360", 321 );
			FillSeries( Connection, "RiftCookPS3", 460 );
			Connection.Close();
		}
	}
}
