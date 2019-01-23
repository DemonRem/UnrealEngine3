using System;
using System.Drawing;
using System.Net.Mail;
using System.Text;

namespace Controller
{
    public class Emailer
    {
        const string MailServer = "zeus.epicgames.net";

        Main Parent;

        public Emailer( Main InParent )
        {
            Parent = InParent;
            Parent.Log( "Emailer created successfully", Color.Green );
        }

        public void SendMail( string To, string Subject, string Body )
        {
            SmtpClient Client = new SmtpClient( MailServer );
            if( Client != null )
            {
                MailAddress AddrFrom = new MailAddress( "builder@epicgames.com", Parent.MachineName );
                MailAddress AddrTo = new MailAddress( To );
                MailMessage Message = new MailMessage( AddrFrom, AddrTo );

                Message.Subject = Subject;
                Message.Body = Body;
                Message.IsBodyHtml = false;
                
                Client.Send( Message );

                Parent.Log( "Email sent to: " + To, Color.Orange );
            }
        }

        public void SendKilledMail( BuilderDB DB, int BuilderID, int BuildLogID )
        {
            string Subject = "[BUILDER] Build killed!";
            string To = DB.GetBuildString( BuilderID, "FailAddresses" );
            StringBuilder Body = new StringBuilder();

            Body.Append( "Build type: \"" + DB.GetBuildString( BuilderID, "Description" ) );
            Body.Append( "\" from changelist " + DB.GetInt( "BuildLog", BuildLogID, "ChangeList" ) + "\r\n" );
            Body.Append( "Build started: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildStarted" ) + "\r\n" );
            Body.Append( "Build ended: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildEnded" ) + "\r\n" );
            Body.Append( "Killed by: " + DB.GetString( "BuildLog", BuildLogID, "Operator" ) + "\r\n" );
            Body.Append( "\r\nCheers\r\nBuilder\r\n" );

            SendMail( To, Subject, Body.ToString() );
        }

        public void SendFailedMail( BuilderDB DB, int BuilderID, int BuildLogID, string FailureMessage )
        {
            string Subject = "[BUILDER] Build failed!";
            string To = DB.GetBuildString( BuilderID, "FailAddresses" );
            StringBuilder Body = new StringBuilder();

            Body.Append( "Build type: \"" + DB.GetBuildString( BuilderID, "Description" ) );
            Body.Append( "\" from changelist " + DB.GetInt( "BuildLog", BuildLogID, "ChangeList" ) + "\r\n" );
            Body.Append( "Build started: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildStarted" ) + "\r\n" );
            Body.Append( "Build ended: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildEnded" ) + "\r\n" );
            Body.Append( "\r\n" + FailureMessage + "\r\n" );
            Body.Append( "\r\nCheers\r\nBuilder\r\n" );

            SendMail( To, Subject, Body.ToString() );
        }

        public void SendSucceededMail( BuilderDB DB, int BuilderID, int BuildLogID, string SuccessMessage )
        {
            string Subject = "[BUILDER] Build succeeded!";
            string To = DB.GetBuildString( BuilderID, "SuccessAddresses" );
            StringBuilder Body = new StringBuilder();

            Body.Append( "Build type: \"" + DB.GetBuildString( BuilderID, "Description" ) );
            Body.Append( "\" from changelist " + DB.GetInt( "BuildLog", BuildLogID, "ChangeList" ) + "\r\n" );
            Body.Append( "Build started: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildStarted" ) + "\r\n" );
            Body.Append( "Build ended: " + DB.GetDateTime( "BuildLog", BuildLogID, "BuildEnded" ) + "\r\n" );
            Body.Append( "\r\n" + SuccessMessage + "\r\n" );
            Body.Append( "\r\nCheers\r\nBuilder\r\n" );

            SendMail( To, Subject, Body.ToString() );
        }
    }
}
