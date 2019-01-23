/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

public partial class CISDownTime : System.Web.UI.Page
{
	private void CalculateUpTime( SqlConnection Connection, string Script, string Series )
	{
		using( SqlCommand Command = new SqlCommand( "SELECT BuildStarted, CurrentStatus FROM BuildLog " +
													"WHERE ( Command = '" + Script + "' ) AND ( DATEDIFF( week, BuildStarted, GETDATE() ) < 52 ) " +
													"ORDER BY BuildStarted", Connection ) )
		{
			SqlDataReader Reader = Command.ExecuteReader();
			if( Reader.HasRows )
			{
				DataTable Table = new DataTable();
				Table.Load( Reader );

				TimeSpan TotalDownTime = new TimeSpan( 0 );
				TimeSpan Week = new TimeSpan( 7, 0, 0, 0 );
				bool CISGood = true;

				// Find the first Sunday in the range, and start from there
				DateTime WeekStartTime = ( DateTime )Table.Rows[0].ItemArray[0];
				while( WeekStartTime.DayOfWeek != DayOfWeek.Sunday )
				{
					WeekStartTime += TimeSpan.FromDays( 1.0 );
				}
				WeekStartTime = new DateTime( WeekStartTime.Year, WeekStartTime.Month, WeekStartTime.Day, 0, 0, 0 ); 

				// Work out the end range
				DateTime StartRange = WeekStartTime;
				DateTime EndRange = StartRange;
				while( EndRange + Week < DateTime.UtcNow )
				{
					EndRange += Week;
				}

				DateTime LastChange = WeekStartTime;

				foreach( DataRow Row in Table.Rows )
				{
					DateTime Time = ( DateTime )Row.ItemArray[0];
					string Status = ( string )Row.ItemArray[1];

					if( Time > StartRange && Time < EndRange )
					{
						if( CISGood && Status == "Failed" )
						{
							// Change from good to bad
							LastChange = Time;
							CISGood = false;
						}
						else if( !CISGood && Status == "Succeeded" )
						{
							// Change from bad to good
							TotalDownTime += Time - LastChange;
							CISGood = true;
						}
						
						if( Time - WeekStartTime > Week )
						{
							if( !CISGood )
							{
								// Reset the clock for the new week
								TotalDownTime += Time - LastChange;
								LastChange = Time;
							}

							CISDownTimeChart.Series[Series].Points.AddXY( WeekStartTime, TotalDownTime.TotalMinutes / 60.0f );

							WeekStartTime += Week;
							TotalDownTime = new TimeSpan( 0 );
						}
					}
				}

				// Add in last entry
				CISDownTimeChart.Series[Series].Points.AddXY( WeekStartTime, TotalDownTime.TotalMinutes / 60.0f );
			}

			Reader.Close();
		}
	}

	protected void Page_Load( object sender, EventArgs e )
	{
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();

			CalculateUpTime( Connection, "Jobs/CISCodeBuilderExample", "CISDownTimeExample" );
			CalculateUpTime( Connection, "Jobs/CISCodeBuilderGear", "CISDownTimeGear" );
			CalculateUpTime( Connection, "Jobs/CISCodeBuilderNano", "CISDownTimeNano" );
			CalculateUpTime( Connection, "Jobs/CISCodeBuilderUT", "CISDownTimeUT" );
			CalculateUpTime( Connection, "Jobs/CISCodeBuilderXenon", "CISDownTimeXbox360" );
			CalculateUpTime( Connection, "Jobs/CISCodeBuilderPS3", "CISDownTimePS3" );

			Connection.Close();
		}
	}
}
