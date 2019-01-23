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

public partial class CISBuildTimes : BasePage
{
	private void FillSeries( SqlConnection Connection, string Series, string Item, int CounterID )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue / 1000 AS " + Item + " FROM PerformanceData " +
													"WHERE ( CounterID = " + CounterID.ToString() + " ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 60 ) " +
													"ORDER BY DateTimeStamp DESC", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				RemoveOutliers( Table );

				CISCompileChart.Series[Series].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, Item );
			}

			Reader.Close();
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "CIS Win32", "CISWin32", 610 );
			FillSeries( Connection, "CIS Win64", "CISWin64", 609 );
			FillSeries( Connection, "CIS Xbox360", "CISXbox360", 268 );
			FillSeries( Connection, "CIS PS3", "CISPS3", 420 );
			Connection.Close();
		}
	}
}
