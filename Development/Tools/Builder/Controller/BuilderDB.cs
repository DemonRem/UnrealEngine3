using System;
using System.Data.SqlClient;
using System.Drawing;
using Controller;

namespace Controller
{
    public class BuilderDB
    {
        Main Parent = null;
        SqlConnection Connection;

        public BuilderDB( Main InParent )
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
                try
                {
                    if( DataReader.Read() )
                    {
                        CommandID = DataReader.GetInt32( 0 );
                    }
                }
                catch
                {
                }
                DataReader.Close();
            }
            catch
            {
                Parent.Log( "ERROR: DB ExecuteReader", Color.Red );
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
                try
                {
                    if( DataReader.Read() )
                    {
                        Result = DataReader.GetString( 0 );
                    }
                }
                catch
                {
                }
                DataReader.Close();
            }
            catch
            {
                Parent.Log( "ERROR: DB ExecuteReader", Color.Red );
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
                try
                {
                    if( DataReader.Read() )
                    {
                        Result = DataReader.GetDateTime( 0 );
                    }
                }
                catch
                {
                }
                DataReader.Close();
            }
            catch
            {
                Parent.Log( "ERROR: DB ExecuteReader", Color.Red );
            }

            return( Result );
        }

        public int GetBuildCommandID( int ID )
        {
            string Query = "SELECT CommandID FROM Builders WHERE ( ID = " + ID.ToString() + " )";
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
                    string Query = "SELECT " + Command + " FROM Commands WHERE ( ID = " + CommandID.ToString() + " )";
                    Result = ReadString( Query );
                }
            }

            return ( Result );
        }

        public void SetLastGoodBuild( int ID, int ChangeList, DateTime Time )
        {
            string Query = "SELECT CommandID FROM Builders WHERE ( ID = " + ID.ToString() + " )";
            ID = ReadInt( Query );
            if( ID != 0 )
            {
                SetInt( "Commands", ID, "LastGoodChangeList", ChangeList );
                SetDateTime( "Commands", ID, "LastGoodDateTime", Time );
            }
        }

        public int Poll()
        {
            int ID;
            string Query;

            Query = "SELECT ID FROM Builders WHERE ( State = 'Killing' )";
            ID = -ReadInt( Query );

            if( ID == 0 )
            {
                Query = "SELECT ID FROM Builders WHERE ( CommandID is not NULL AND State = 'Connected' )";
                ID = ReadInt( Query );
            }

            return( ID );
        }

        public int GetInt( string TableName, int ID, string Command )
        {
            int Result = 0;

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadInt( Query );
            }

            return ( Result );
        }

        public string GetString( string TableName, int ID, string Command )
        {
            string Result = "";

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadString( Query );
            }

            return ( Result );
        }

        public DateTime GetDateTime( string TableName, int ID, string Command )
        {
            DateTime Result = DateTime.Now;

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadDateTime( Query );
            }

            return ( Result );
        }

        public void SetString( string TableName, int ID, string Field, string Info )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = '" + Info + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void SetInt( string TableName, int ID, string Field, int Info )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = " + Info.ToString() + " WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void SetDateTime( string TableName, int ID, string Field, DateTime Time )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = '" + Time.ToString() + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void Delete( string TableName, int ID, string Field )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = null WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }
    }
}