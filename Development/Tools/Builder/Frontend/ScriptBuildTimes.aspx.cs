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

public partial class ScriptBuildTimes : BasePage
{
	private void FillSeries( SqlConnection Connection, string Item, int CounterID )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue / 1000 AS " + Item + " FROM PerformanceData " +
													"WHERE ( CounterID = " + CounterID.ToString() + " ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 90 ) " +
													"ORDER BY DateTimeStamp DESC", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				RemoveOutliers( Table );

				CISCompileChart.Series[Item].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, Item );
			}

			Reader.Close();
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "ScriptExample", 842 );
			FillSeries( Connection, "ScriptExampleFR", 843 );
			FillSeries( Connection, "ScriptGear", 839 );
			FillSeries( Connection, "ScriptGearFR", 841 );
			FillSeries( Connection, "ScriptNano", 836 );
			FillSeries( Connection, "ScriptNanoFR", 837 );
			FillSeries( Connection, "ScriptUDK", 838 );
			FillSeries( Connection, "ScriptUDKFR", 840 );
			Connection.Close();
		}
	}
}
