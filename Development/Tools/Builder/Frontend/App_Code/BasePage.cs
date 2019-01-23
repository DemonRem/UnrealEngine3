using System;
using System.Data;
using System.Configuration;
using System.Data.SqlClient;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

public class BasePage : System.Web.UI.Page
{
    protected SqlConnection OpenConnection()
    {
        SqlConnection Connection = new SqlConnection( "Data Source=DB-01;Initial Catalog=Perf_Build;Integrated Security=True" );
        Connection.Open();
        return ( Connection );
    }

    protected void CloseConnection( SqlConnection Connection )
    {
        Connection.Close();
    }

    protected void Update( SqlConnection Connection, string CommandString )
    {
        SqlCommand Command = new SqlCommand( CommandString, Connection );
        Command.ExecuteNonQuery();
    }

    protected int ReadInt( SqlConnection Connection, string CommandString )
    {
        int Result = 0;

        try
        {
            SqlCommand Command = new SqlCommand( CommandString, Connection );
            SqlDataReader DataReader = Command.ExecuteReader();
            if( DataReader.Read() )
            {
                Result = DataReader.GetInt32( 0 );
            }
            DataReader.Close();
        }
        catch
        {
        }

        return ( Result );
    }

    protected string ReadString( SqlConnection Connection, string CommandString )
    {
        string Result = "";

        try
        {
            SqlCommand Command = new SqlCommand( CommandString, Connection );
            SqlDataReader DataReader = Command.ExecuteReader();
            if( DataReader.Read() )
            {
                Result = DataReader.GetString( 0 );
            }
            DataReader.Close();
        }
        catch
        {
        }

        return( Result );
    }

    protected DateTime ReadDateTime( SqlConnection Connection, string CommandString )
    {
        DateTime Result = DateTime.Now;

        try
        {
            SqlCommand Command = new SqlCommand( CommandString, Connection );
            SqlDataReader DataReader = Command.ExecuteReader();
            if( DataReader.Read() )
            {
                Result = DataReader.GetDateTime( 0 );
            }
            DataReader.Close();
        }
        catch
        {
        }

        return ( Result );
    }

    protected int GetBuilderID()
    {
        string CommandString;
        SqlConnection Connection = OpenConnection();

        CommandString = "SELECT ID FROM Builders WHERE ( CommandID is not NULL AND State != 'Dead' AND State != 'Zombied' )";
        int ID = ReadInt( Connection, CommandString );

        CloseConnection( Connection );

        return( ID );
    }

    protected bool IsControllerAvailable()
    {
        string CommandString;
        SqlConnection Connection = OpenConnection();

        CommandString = "SELECT ID FROM Builders WHERE ( State = 'Connected' AND CommandID is NULL )";
        int ID = ReadInt( Connection, CommandString );

        CloseConnection( Connection );

        return ( ID != 0 );
    }
}
