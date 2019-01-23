/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Data.SqlClient;
using System.Management;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

public partial class Promote : BasePage
{
    public Promote()
    {
        Page.Init += new System.EventHandler(Page_Init);
    }

    private DataSet GetDataSet( SqlConnection Connection, int PromoteID, int AccessMin, int AccessMax, int Count )
    {
        DataSet SetOfData = new DataSet();

        string ParentString = "SELECT TOP ( " + Count.ToString() + " ) B.[BuildLabel], B.[ID], B.[BuildStarted] FROM [BuildLog] AS B WHERE ( B.[BuildLabel] IS NOT NULL AND Promotable = " + PromoteID.ToString() + " ) ORDER BY B.BuildStarted DESC";
        SqlDataAdapter ParentCommand = new SqlDataAdapter( ParentString, Connection );
        ParentCommand.Fill( SetOfData, "BuildLabels" );

        SqlDataAdapter ChildCommand = new SqlDataAdapter( "SELECT C.[Description], C.[ID] AS CommandID, B.[ID], B.[BuildLabel] FROM [Commands] AS C CROSS JOIN ( " + ParentString + " ) AS B WHERE ( C.Access >= " + AccessMin.ToString() + " AND C.Access <= " + AccessMax.ToString() + " ) ORDER BY C.Access", Connection );
        ChildCommand.Fill( SetOfData, "Buttons" );

        SetOfData.Relations.Add( "PseudoRelation", SetOfData.Tables["BuildLabels"].Columns["ID"], SetOfData.Tables["Buttons"].Columns["ID"] );

        return ( SetOfData );
    }

    protected void Page_Load( object sender, EventArgs e )
    {
        string LoggedOnUser = Context.User.Identity.Name;
        string MachineName = Context.Request.UserHostName;

        Label_Welcome.Text = "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"";

		using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
		{
			Connection.Open();

			string Query = "SELECT LastGoodChangeList FROM [Commands] WHERE ( Description = 'Send QA Build Changes' )";
			int StartChangeList = ReadInt( Connection, Query );
			Query = "SELECT LastGoodChangeList FROM [Commands] WHERE ( Description = 'Promote QA Build' )";
			int EndChangeList = ReadInt( Connection, Query );
			Label_ChangesRange.Text = "Current range for QA changes is from " + StartChangeList.ToString() + " to " + EndChangeList.ToString();

			DataSet UE3 = GetDataSet( Connection, 1, 4000, 4999, 5 );
			UnrealEngine3Repeater.DataSource = UE3.Tables["BuildLabels"];

			DataSet RiftBeta = GetDataSet( Connection, 16, 16900, 16999, 5 );
			UnrealEngine3RiftBetaRepeater.DataSource = RiftBeta.Tables["BuildLabels"];

			DataSet ProjectSword = GetDataSet( Connection, 55, 55000, 55999, 5 );
			UnrealEngine3ProjectSwordRepeater.DataSource = ProjectSword.Tables["BuildLabels"];

			DataSet QA = GetDataSet( Connection, 1, 5000, 5999, 15 );
			UnrealEngine3QARepeater.DataSource = QA.Tables["BuildLabels"];

			Page.DataBind();
			Connection.Close();
		}
    }

    private void Page_Init( object sender, EventArgs e )
    {
        InitializeComponent();
    }
    private void InitializeComponent()
    {
        this.Load += new System.EventHandler( this.Page_Load );
    }

    private void BuilderDBRepeater_PromoteItemCommand( RepeaterCommandEventArgs e, string VariableName, string Branch )
    {
        if( e.Item != null )
        {
			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();

				int CommandID = Int32.Parse( ( string )e.CommandName );
				string BuildLabel = ( string )e.CommandArgument;

				// Find the command id that matches the description
				if( CommandID != 0 )
				{
					// Set the latest build variable
					string CommandString = "UPDATE Variables SET Value = '" + BuildLabel + "' WHERE ( Variable = '" + VariableName + "' AND Branch = '" + Branch + "' )";
					Update( Connection, CommandString );

					// Trigger the build promotion
					string User = Context.User.Identity.Name;
					int Offset = User.LastIndexOf( '\\' );
					if( Offset >= 0 )
					{
						User = User.Substring( Offset + 1 );
					}

					CommandString = "UPDATE [Commands] SET Pending = 1, Operator = '" + User + "' WHERE ( ID = " + CommandID.ToString() + " )";
					Update( Connection, CommandString );
				}

				Connection.Close();
			}

			Response.Redirect( "Default.aspx" );
		}
    }

    protected void BuilderDBRepeater_PromoteItemCommand_UE3( object source, RepeaterCommandEventArgs e )
    {
		BuilderDBRepeater_PromoteItemCommand( e, "LatestApprovedBuild", "UnrealEngine3" );
    }

	protected void BuilderDBRepeater_PromoteItemCommand_RiftBeta( object source, RepeaterCommandEventArgs e )
	{
		BuilderDBRepeater_PromoteItemCommand( e, "LatestApprovedBuild", "UnrealEngine3-RiftBeta" );
	}

	protected void BuilderDBRepeater_PromoteItemCommand_ProjectSword( object source, RepeaterCommandEventArgs e )
	{
		BuilderDBRepeater_PromoteItemCommand( e, "LatestApprovedBuild", "UnrealEngine3-ProjectSword" );
	}

	protected void BuilderDBRepeater_PromoteItemCommand_QA( object source, RepeaterCommandEventArgs e )
    {
        BuilderDBRepeater_PromoteItemCommand( e, "LatestApprovedQABuild", "UnrealEngine3" );
    }

    protected void Button_QAChanges_Click( object sender, EventArgs e )
    {
        Button Pressed = ( Button )sender;
        if( Pressed.ID == "Button_ChangesRange" )
        {
            string User = Context.User.Identity.Name;
            int Offset = User.LastIndexOf( '\\' );
            if( Offset >= 0 )
            {
                User = User.Substring( Offset + 1 );
            }

			using( SqlConnection Connection = new SqlConnection( ConfigurationManager.ConnectionStrings["BuilderConnectionString"].ConnectionString ) )
			{
				Connection.Open();
				string CommandString = "UPDATE [Commands] SET Pending = 1, Operator = '" + User + "' WHERE ( Description = '" + Pressed.Text + "' )";
				Update( Connection, CommandString );
				Connection.Close();
			}

            Response.Redirect( "Default.aspx" );
        }
    }
}
