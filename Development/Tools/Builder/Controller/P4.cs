using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using P4COM;

namespace Controller
{
    class P4
    {
        private Main Parent;
        private P4COM.p4 Interface;
        private string RootFolderName = "\\UnrealEngine3-Builder";
        private ERRORS ErrorLevel = ERRORS.None;
        private bool ClientSpecValidated = false;
        private string CurrentWorkingDirectory = "";

        public P4( Main InParent )
        {
            Parent = InParent;
            Interface = new P4COM.p4();
        }

        public ERRORS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        public void Init()
        {
            ClientSpecValidated = false;
        }

        private void Write( StreamWriter Log, System.Array Output )
        {
            foreach( string Line in Output )
            {
                Log.Write( Line + "\r\n" );
            }
        }

        private void Write( StreamWriter Log, string Output )
        {
            Log.Write( Output + "\r\n" );
        }

        private bool ValidateClientSpec( string ClientSpec )
        {
            if( !ClientSpecValidated )
            {
                string P4Folder = GetClientRoot( ClientSpec ) + RootFolderName;
                CurrentWorkingDirectory = Environment.CurrentDirectory;

                if( CurrentWorkingDirectory.ToLower() != P4Folder.ToLower() )
                {
                    Parent.Log( "P4ERROR: ClientSpec \'" + ClientSpec + "\' does not operate on \'" + CurrentWorkingDirectory + "\'", Color.Red );
                    return ( false );
                }

                ClientSpecValidated = true;
            }

            return ( ClientSpecValidated );
        }

        public string GetClientRoot( string ClientSpec )
        {
            System.Array Output;
            string RootFolder = "<Unknown>";

            ErrorLevel = ERRORS.None;
            Interface.Connect();

            Interface.Client = ClientSpec;
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 1;
            try
            {
                Output = Interface.run( "info" );

                foreach( string Line in Output )
                {
                    if( Line.StartsWith( "Client root:" ) )
                    {
                        RootFolder = Line.Substring( "Client root:".Length ).Trim();
                        break;
                    }
                }
            }
            catch( System.Runtime.InteropServices.COMException )
            {
                ErrorLevel = ERRORS.SCC_GetClientRoot;
            }

            Interface.Disconnect();

            return ( RootFolder );
        }

        public void SyncToChangeList( StreamWriter Log, string ClientSpec, int ChangeList, string Label )
        {
            System.Array Output;
            string Command;

            if( !ValidateClientSpec( ClientSpec ) )
            {
                return;
            }

            Command = "sync UnrealEngine3-Builder//...";
            if( ChangeList != 0 )
            {
                 Command += "@" + ChangeList.ToString();
            }
            else if( Label.Length > 0 )
            {
                Command += "@" + Label;
            }

            ErrorLevel = ERRORS.None;
            Interface.Connect();

            Interface.Client = ClientSpec;
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 1;
            try
            {
                Output = Interface.run( Command );
                Write( Log, Output );
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR: Sync " + ex.Message );
                ErrorLevel = ERRORS.SCC_Sync;
            }

            Interface.Disconnect();
        }

        public void Revert( StreamWriter Log, string ClientSpec, string FileSpec )
        {
            System.Array Output;

            if( !ValidateClientSpec( ClientSpec ) )
            {
                return;
            }

            ErrorLevel = ERRORS.None;
            Interface.Connect();

            Interface.Client = ClientSpec;
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 1;
            try
            {
                Output = Interface.run( "revert " + FileSpec );
                Write( Log, Output );
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR: Revert " + ex.Message );
                ErrorLevel = ERRORS.SCC_Revert;
            }

            Interface.Disconnect();
        }

        public void CheckoutFileSpec( StreamWriter Log, string ClientSpec, string FileSpec, bool Lock )
        {
            System.Array Output;

            if( !ValidateClientSpec( ClientSpec ) )
            {
                return;
            }

            ErrorLevel = ERRORS.None;
            Interface.Connect();
            Interface.Client = ClientSpec;
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 2;

            try
            {
                Output = Interface.run( "edit " + FileSpec );
                Write( Log, Output );

                if( Lock )
                {
                    Output = Interface.run( "lock " + FileSpec );
                    Write( Log, Output );
                }
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR: Edit (& Lock) " + ex.Message );
                ErrorLevel = ERRORS.SCC_Checkout;
            }

            Interface.Disconnect();
        }

        public void Submit( StreamWriter Log, string ClientSpec, int ChangeList )
        {
            System.Array Output;

            if( !ValidateClientSpec( ClientSpec ) )
            {
                return;
            }

            ErrorLevel = ERRORS.None;

            Interface.ParseForms();
            Interface.Connect();
            Interface.Client = ClientSpec;
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 2;

            try
            {
                Output = Interface.run( "change -o" );
                Write( Log, Output );

                Interface.set_Var( "Description", "Built from ChangeList " + ChangeList.ToString() );

                Output = Interface.run( "submit -i" );
                Write( Log, Output );
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR: Submit " + ex.Message );
                ErrorLevel = ERRORS.SCC_Submit;
            }

            Interface.Disconnect();
        }

        public void GetMostRecentBuild( ScriptParser Builder, StreamWriter Log, string BuilderName )
        {
            System.Array Output;

            if( !ValidateClientSpec( Builder.GetClientSpec() ) )
            {
                return;
            }

            ErrorLevel = ERRORS.None;
            Interface.Connect();
            Interface.Client = Builder.GetClientSpec();
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 2;

            try
            {
                Output = Interface.run( "changes -c " + BuilderName );
                Write( Log, Output );

                foreach( string Line in Output )
                {
                    if( Line.StartsWith( "Change" ) && Line.IndexOf( "UnrealEngine3_Build_" ) > 0 )
                    {
                        string Temp = Line.Substring( "Change ".Length );
                        Temp = Temp.Substring( 0, Temp.IndexOf( ' ' ) );
                        Builder.SetMostRecentBuild( Temp );
                        break;
                    }
                }
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR!: GetMostRecentBuild " + ex.Message );
                ErrorLevel = ERRORS.SCC_GetChanges;
            }

            Interface.Disconnect();
        }

        public void GetChangesSinceLastBuild( ScriptParser Builder, StreamWriter Log )
        {
            System.Array ChangeOutput;
            System.Array DescribeOutput;

            if( !ValidateClientSpec( Builder.GetClientSpec() ) )
            {
                return;
            }

            ErrorLevel = ERRORS.None;
            Interface.Connect();
            Interface.Client = Builder.GetClientSpec();
            Interface.Cwd = CurrentWorkingDirectory;
            Interface.ExceptionLevel = 2;

            try
            {
                ChangeOutput = Interface.run( "changes ...@" + Builder.GetMostRecentBuild().ToString() + ",#head" );

                foreach( string Line in ChangeOutput )
                {
                    if( Line.StartsWith( "Change" ) )
                    {
                        string ChangeList = Line.Substring( "Change".Length ).Trim();
                        ChangeList = ChangeList.Substring( 0, ChangeList.IndexOf( ' ' ) );
                        DescribeOutput = Interface.run( "describe " + ChangeList );

                        Builder.ProcessChangeList( ChangeList, DescribeOutput );
                    }
                }
            }
            catch( System.Runtime.InteropServices.COMException ex )
            {
                Write( Log, "P4ERROR!: GetChangesSinceLastBuild " + ex.Message );
                ErrorLevel = ERRORS.SCC_GetChanges;
            }

            Interface.Disconnect();
        }
    }
}
