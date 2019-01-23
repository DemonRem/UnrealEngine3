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

public partial class TotalJobCounts : BasePage
{
	private void FillSeries( SqlConnection Connection, string Series, string CommandType, bool Exceptions )
	{
		string Query = "SELECT COUNT( BuildStarted ) AS CommandCount, DATEDIFF( day, BuildStarted, GETDATE() ) AS BuildAge, ";
		Query += "DATEADD( day, -DATEDIFF( day, BuildStarted, GETDATE() ), GetDate() ) AS BuildDate ";
		Query += "FROM BuildLog ";
		Query += "WHERE ( DATEDIFF( day, BuildStarted, GETDATE() ) < 180 ) ";
		if( CommandType.Length > 0 )
		{
			Query += "AND ( SUBSTRING( Command, 1, " + CommandType.Length.ToString() + " ) = '" + CommandType + "' ) ";
		}
		else
		{
			Query += "AND ( SUBSTRING( Command, 1, 4 ) <> 'Soak' ) ";
		}

		if( Exceptions )
		{
			Query += "AND Command <> 'CIS/ProcessP4Changes' AND Command <> 'CIS/UpdateMonitorValues' ";
		}
		Query += "GROUP BY DATEDIFF( day, BuildStarted, GETDATE() ) ";
		Query += "ORDER BY BuildAge DESC";

		using( SqlCommand Command = new SqlCommand( Query, Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				CommandCountChart.Series[Series].Points.DataBindXY( Table.Rows, "BuildDate", Table.Rows, "CommandCount" );
			}

			Reader.Close();
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "Verification Builds", "CIS", true );
			FillSeries( Connection, "Compile Jobs", "Jobs/Unreal", false );
			FillSeries( Connection, "CIS Jobs", "Jobs/CIS", false );
			FillSeries( Connection, "PCS", "PCS", false );
			FillSeries( Connection, "Cooks", "Cook", false );
			FillSeries( Connection, "Builds", "Build", false );
			FillSeries( Connection, "Promotions", "Promote", false );
			FillSeries( Connection, "Total", "", true );
			Connection.Close();
		}
	}
}
