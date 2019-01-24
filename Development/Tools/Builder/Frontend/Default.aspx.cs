/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Data.SqlClient;
using System.Drawing;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

public partial class _Default : BasePage
{
    private int ActiveBuilderCount = -1;

    protected void Page_Load( object sender, EventArgs e )
    {
        string LoggedOnUser = Context.User.Identity.Name;
        string MachineName = Context.Request.UserHostName;

        Label_Welcome.Text = "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"";

        ScriptTimer_Tick( sender, e );
		JobTimer_Tick( sender, e );
		VerifyTimer_Tick( sender, e );

        CheckSystemDown();
    }

    protected void CheckSystemDown()
    {
		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();

			string CommandString = "SELECT Value FROM [Variables] WHERE ( Variable = 'StatusMessage' )";
			string Message = ReadString( Connection, CommandString );

			Connection.Close();

			if( Message.Length > 0 )
			{
				Response.Redirect( "Offline.aspx" );
			}
		}
    }

    protected void Button_TriggerBuild_Click( object sender, EventArgs e )
    {
        Button Pressed = ( Button )sender;
        if( Pressed.ID == "Button_BuildStatus" )
        {
            Response.Redirect( "BuildStatus.aspx?BranchName=UnrealEngine3" );
        }
		if( Pressed.ID == "Button_BuildStatus_GFx" )
		{
			Response.Redirect( "BuildStatus.aspx?BranchName=UnrealEngine3-GFx" );
		}
		else if( Pressed.ID == "Button_TriggerBuild" )
        {
            Response.Redirect( "Builder.aspx" );
        }
        else if( Pressed.ID == "Button_TriggerPCS" )
        {
            Response.Redirect( "PCS.aspx" );
        }
		else if( Pressed.ID == "Button_RiftBeta" )
		{
			Response.Redirect( "RiftBeta.aspx" );
		}
        else if( Pressed.ID == "Button_Gears2" )
        {
            Response.Redirect( "Gears2X360.aspx" );
        }
		else if( Pressed.ID == "Button_Gears2_Japan" )
		{
			Response.Redirect( "Gears2Japan.aspx" );
		}
        else if( Pressed.ID == "Button_TriggerCook" )
        {
            Response.Redirect( "Cooker.aspx" );
        }
		else if( Pressed.ID == "Button_TriggerTool" )
		{
			Response.Redirect( "Tools.aspx" );
		}
		else if( Pressed.ID == "Button_TriggerPhysX" )
		{
			Response.Redirect( "SDKs.aspx" );
		}
		else if( Pressed.ID == "Button_PromoteBuild" )
		{
			Response.Redirect( "Promote.aspx" );
		}
		else if( Pressed.ID == "Button_Verification" )
		{
			Response.Redirect( "Verification.aspx" );
		}
		else if( Pressed.ID == "Button_Maintenance" )
		{
			Response.Redirect( "Maintenance.aspx" );
		}
		else if( Pressed.ID == "Button_CIS" )
		{
			Response.Redirect( "CIS.aspx" );
		}
		else if( Pressed.ID == "Button_BuildCharts" )
		{
			Response.Redirect( "BuildCharts.aspx" );
		}
	}

    protected void Repeater_BuildLog_ItemCommand( object source, RepeaterCommandEventArgs e )
    {
        if( e.Item != null )
        {
			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();

				// Find the command id that matches the description
				LinkButton Pressed = ( LinkButton )e.CommandSource;
				string Build = Pressed.Text.Substring( "Stop ".Length );
				string CommandString = "SELECT [ID] FROM [Commands] WHERE ( Description = '" + Build + "' )";
				int CommandID = ReadInt( Connection, CommandString );

				if( CommandID != 0 )
				{
					string User = Context.User.Identity.Name;
					int Offset = User.LastIndexOf( '\\' );
					if( Offset >= 0 )
					{
						User = User.Substring( Offset + 1 );
					}

					CommandString = "UPDATE Commands SET Killing = 1, Killer = '" + User + "' WHERE ( ID = " + CommandID.ToString() + " )";
					Update( Connection, CommandString );
				}

				Connection.Close();
			}
        }
    }
    
    protected string DateDiff( object Start )
    {
        TimeSpan Taken = DateTime.UtcNow - ( DateTime )Start;

        string TimeTaken = "Time taken :" + Environment.NewLine;
        TimeTaken += Taken.Hours.ToString( "00" ) + ":" + Taken.Minutes.ToString( "00" ) + ":" + Taken.Seconds.ToString( "00" );

        return ( TimeTaken );
    }

    protected string DateDiff2( object Start )
    {
        TimeSpan Taken = DateTime.UtcNow - ( DateTime )Start;

        string TimeTaken = "( " + Taken.Hours.ToString( "00" ) + ":" + Taken.Minutes.ToString( "00" ) + ":" + Taken.Seconds.ToString( "00" ) + " )";

        return ( TimeTaken );
    }

    protected Color CheckConnected( object LastPing )
    {
        if( LastPing.GetType() == DateTime.UtcNow.GetType() )
        {
            TimeSpan Taken = DateTime.UtcNow - ( DateTime )LastPing;

            // Check for no ping in 900 seconds
            if( Taken.TotalSeconds > 900 )
            {
                return ( Color.Red );
            }
        }

        return ( Color.DarkGreen );
    }

    protected string GetAvailability( object Machine, object LastPing )
    {
        if( LastPing.GetType() == DateTime.UtcNow.GetType() )
        {
            TimeSpan Taken = DateTime.UtcNow - ( DateTime )LastPing;

            if( Taken.TotalSeconds > 300 )
            {
                return ( "Controller '" + ( string )Machine + "' is NOT responding!" );
            }
        }

        return ( "Controller '" + ( string )Machine + "' is available" );
    }

    protected void ScriptTimer_Tick( object sender, EventArgs e )
    {
		try
		{
			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();

				DataSet StatusData = new DataSet();
				SqlDataAdapter StatusCommand = new SqlDataAdapter( "EXEC SelectBuildStatus", Connection );
				StatusCommand.Fill( StatusData, "ActiveBuilds" );
				Repeater_BuildLog.DataSource = StatusData;
				Repeater_BuildLog.DataBind();

				int BuilderCount = ReadIntSP( Connection, "GetActiveBuilderCount" );

				if( BuilderCount != ActiveBuilderCount )
				{
					DataSet MainBranchData = new DataSet();
					SqlDataAdapter MainBranchCommand = new SqlDataAdapter( "EXEC SelectBuilds 100,0", Connection );
					MainBranchCommand.Fill( MainBranchData, "MainBranch" );
					Repeater_MainBranch.DataSource = MainBranchData;
					Repeater_MainBranch.DataBind();

					DataSet BuildersData = new DataSet();
					SqlDataAdapter BuildersCommand = new SqlDataAdapter( "EXEC SelectActiveBuilders", Connection );
					BuildersCommand.Fill( BuildersData, "ActiveBuilders" );
					Repeater_Builders.DataSource = BuildersData;
					Repeater_Builders.DataBind();

					ActiveBuilderCount = BuilderCount;
				}

				Connection.Close();
			}
		}
		catch
		{
		}
    }

	protected void JobTimer_Tick( object sender, EventArgs e )
	{
		try
		{
			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();

				DataSet JobsData = new DataSet();
				SqlDataAdapter JobsCommand = new SqlDataAdapter( "EXEC SelectJobStatus", Connection );
				JobsCommand.Fill( JobsData, "ActiveJobs" );
				Repeater_JobLog.DataSource = JobsData;
				Repeater_JobLog.DataBind();

				string Query = "SELECT COUNT(*) FROM [Jobs] WHERE ( Active = 0 AND Complete = 0 AND PrimaryBuild = 0 )";
				int PendingTasks = ReadInt( Connection, Query );

				Label_PendingCISTasks.Text = "There are " + PendingTasks.ToString() + " pending CIS tasks";

				Connection.Close();
			}
		}
		catch
		{
		}
	}

    protected void VerifyTimer_Tick( object sender, EventArgs e )
	{
		try
		{
			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();

				DataSet StatusData = new DataSet();
				SqlDataAdapter StatusCommand = new SqlDataAdapter( "EXEC SelectVerifyStatus", Connection );
				StatusCommand.Fill( StatusData, "ActiveBuilds" );
				Repeater_Verify.DataSource = StatusData;
				Repeater_Verify.DataBind();

				Connection.Close();
			}
		}
		catch
		{
		}
	}
}
