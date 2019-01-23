using System;
using System.Data.SqlClient;
using System.Drawing;
using Controller;

namespace Controller
{
    public class BobDB
    {
        Main Parent = null;
        SqlConnection Connection;

        public BobDB( Main InParent )
        {
            Parent = InParent;

            Connection = new SqlConnection( "Data Source=DB-01;Initial Catalog=Perf_Build;Integrated Security=True" );
            Connection.Open();

            Parent.Log( "Connected to database OK", Color.Green );
        }

        public void Destroy()
        {
            Parent.Log( "Closing database connection", Color.Green );
            if( Connection != null )
            {
                Connection.Close();
            }
        }

        public void Update( string CommandString )
        {
            SqlCommand Command = new SqlCommand( CommandString, Connection );
            Command.ExecuteNonQuery();
        }

        public int ReadInt( string CommandString )
        {
            int CommandID = 0;

            try
            {
                SqlCommand Command = new SqlCommand( CommandString, Connection );
                SqlDataReader DataReader = Command.ExecuteReader();
                if( DataReader.Read() )
                {
                    CommandID = DataReader.GetInt32( 0 );
                }
                DataReader.Close();
            }
            catch
            {
            }

            return ( CommandID );
        }

        public string ReadString( string CommandString )
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

            return ( Result );
        }

        public DateTime ReadDateTime( string CommandString )
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

            return( Result );
        }

        public int GetBuildCommandID( int ID )
        {
            string Query = "SELECT CommandID FROM BuildLog WHERE ( ID = " + ID.ToString() + ")";
            int CommandID = ReadInt( Query );

            return( CommandID );
        }

        public string GetBuildString( int ID, string Command )
        {
            string Result = "";

            if( ID != 0 )
            {
                int CommandID = GetBuildCommandID( ID );

                if( CommandID != 0 )
                {
                    string Query = "SELECT " + Command + " FROM Commands WHERE ( ID = " + CommandID.ToString() + ")";
                    Result = ReadString( Query );
                }
            }

            return ( Result );
        }

        public int GetInt( int ID, string Command )
        {
            string Query = "SELECT " + Command + " FROM BuildLog WHERE ( ID = " + ID.ToString() + " )";
            int Result = ReadInt( Query );

            return ( Result );
        }
        
        public string GetString( int ID, string Command )
        {
            string Query = "SELECT " + Command + " FROM BuildLog WHERE ( ID = " + ID.ToString() + " )";
            string Result = ReadString( Query );

            return ( Result );
        }

        public DateTime GetDateTime( int ID, string Command )
        {
            string Query = "SELECT " + Command + " FROM BuildLog WHERE ( ID = " + ID.ToString() + " )";
            DateTime Result = ReadDateTime( Query );

            return( Result );
        }

        public int Poll()
        {
            int ID;
            string Query;

            Query = "SELECT ID FROM BuildLog WHERE Status = 'Killing'";
            ID = -ReadInt( Query );

            if( ID == 0 )
            {
                Query = "SELECT ID FROM BuildLog WHERE ( BuildStarted is NULL )";
                ID = ReadInt( Query );
            }

            return( ID );
        }

        public void SetString( int ID, string Field, string Info )
        {
            if( ID != 0 )
            {
                string CommandString = "UPDATE BuildLog SET " + Field + " = '" + Info + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( CommandString );
            }
        }

        public void SetInt( int ID, string Field, int Info )
        {
            if( ID != 0 )
            {
                string CommandString = "UPDATE BuildLog SET " + Field + " = " + Info.ToString() + " WHERE ( ID = " + ID.ToString() + " )";
                Update( CommandString );
            }
        }

        public void SetDateTime( int ID, string Field, DateTime Time )
        {
            if( ID != 0 )
            {
                string CommandString = "UPDATE BuildLog SET " + Field + " = '" + Time.ToString() + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( CommandString );
            }
        }
    }
}