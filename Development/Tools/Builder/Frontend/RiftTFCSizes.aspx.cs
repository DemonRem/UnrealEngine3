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

public partial class RiftTFCSizes : BasePage
{
	private void FillSeries( SqlConnection Connection, string Item, int CounterID )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT DateTimeStamp, IntValue / ( 1024 * 1024 ) AS " + Item + " FROM PerformanceData " +
													"WHERE ( CounterID = " + CounterID.ToString() + " ) AND ( DATEDIFF( day, DateTimeStamp, GETDATE() ) < 150 ) " +
													"ORDER BY DateTimeStamp DESC", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				RiftTFCSizesChart.Series[Item].Points.DataBindXY( Table.Rows, "DateTimeStamp", Table.Rows, Item );
			}

			Reader.Close();
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();
			FillSeries( Connection, "RiftTexturesTFCSize", 953 );
			FillSeries( Connection, "RiftCharTexturesTFCSize", 955 );
			FillSeries( Connection, "RiftLightingTFCSize", 954 );
			FillSeries( Connection, "RiftCookedFolderSize", 956 );
			Connection.Close();
		}
	}
}
