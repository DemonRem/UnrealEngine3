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

public partial class Builder : BasePage
{
    protected void Page_Load( object sender, EventArgs e )
    {
        string LoggedOnUser = Context.User.Identity.Name;
        string MachineName = Context.Request.UserHostName;

        Label_Welcome.Text = "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"";

        int BuilderID = GetBuilderID();
        if( BuilderID != 0 )
        {
            Server.Transfer( "Status.aspx" );
        }
    }

    protected void BuilderDBRepeater_ItemCommand( object source, RepeaterCommandEventArgs e )
    {
        if( e.Item != null )
        {
            string CommandString;

            SqlConnection Connection = OpenConnection();

            // Find the command id that matches the description
            CommandString = "SELECT ID FROM Commands WHERE ( Description = '" + ( ( Button )e.CommandSource ).Text + "' )";
            int CommandID = ReadInt( Connection, CommandString );

            if( CommandID != 0 )
            {
                CommandString = "SELECT ID FROM Builders WHERE ( State = 'Connected' AND CommandID is NULL )";
                int BuilderID = ReadInt( Connection, CommandString );

                if( BuilderID != 0 )
                {
                    CommandString = "UPDATE Builders SET CommandID = '" + CommandID.ToString() + "' WHERE ( ID = " + BuilderID.ToString() + " )";
                    Update( Connection, CommandString );

                    Server.Transfer( "Status.aspx" );
                }
            }

            CloseConnection( Connection );
        }
    }
}
