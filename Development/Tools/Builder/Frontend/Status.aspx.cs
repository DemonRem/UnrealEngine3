using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Data.SqlClient;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

public partial class Status : BasePage
{
    protected void Page_Load( object sender, EventArgs e )
    {
        int BuilderID = GetBuilderID();
        if( BuilderID == 0 )
        {
            Server.Transfer( "Builder.aspx" );
        }

        UpdateStatus( BuilderID );
    }

    protected void UpdateStatus( int BuilderID )
    {
        string CommandString;
        SqlConnection Connection = OpenConnection();

        CommandString = "SELECT BuildLogID FROM Builders WHERE ( ID = '" + BuilderID.ToString() + "\' )";
        int ID = ReadInt( Connection, CommandString );

        if( ID != 0 )
        {
            CommandString = "SELECT BuildStarted FROM BuildLog WHERE ( BuildStarted is not NULL AND ID = " + ID.ToString() + " )";
            DateTime Started = ReadDateTime( Connection, CommandString );
            TimeSpan Duration = DateTime.Now - Started;
            TimerLabel.Text = Duration.Hours + " hours " + Duration.Minutes + " minutes " + Duration.Seconds + " seconds";

            CommandString = "SELECT CurrentStatus FROM BuildLog WHERE ( CurrentStatus is not NULL AND ID = " + ID.ToString() + " )";
            StatusLabel.Text = ReadString( Connection, CommandString );

            CommandString = "SELECT ChangeList FROM BuildLog WHERE ( ChangeList is not NULL AND ID = " + ID.ToString() + " )";
            ChangeListLabel.Text = "Building from changelist: " + ReadInt( Connection, CommandString ).ToString();
        }
        else
        {   
            // Timeout check here as the Controller hasn't triggered the build yet
        }

        CloseConnection( Connection );
    }

    protected void Button_StopBuild_Click( object sender, EventArgs e )
    {
        string CommandString;
        SqlConnection Connection = OpenConnection();

        CommandString = "SELECT ID FROM Builders WHERE ( State = 'Building' AND CommandID is not NULL )";
        int BuilderID = ReadInt( Connection, CommandString );

        if( BuilderID != 0 )
        {
            CommandString = "UPDATE Builders SET State = 'Killing' WHERE ( ID = " + BuilderID.ToString() + ")";
            Update( Connection, CommandString );

            CommandString = "SELECT BuildLogID FROM Builders WHERE ( BuildLogID is not NULL AND ID = " + BuilderID.ToString() + ")";
            int BuildLogID = ReadInt( Connection, CommandString );

            if( BuildLogID != 0 )
            {
                CommandString = "UPDATE BuildLog SET Operator = '" + Context.User.Identity.Name + "' WHERE ( ID = " + BuildLogID.ToString() + ")";
                Update( Connection, CommandString );
            }
        }

        CloseConnection( Connection );

        Server.Transfer( "Builder.aspx" );
    }

}
