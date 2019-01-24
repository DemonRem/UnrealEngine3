using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

public partial class _Default : System.Web.UI.Page 
{
    protected void Page_Load(object sender, EventArgs e)
    {
		string LoggedOnUser = Context.User.Identity.Name;
		string MachineName = Context.Request.UserHostName;

		Label_Welcome.Text = "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"";
    }

	protected void Button_TriggerBuild_Click( object sender, EventArgs e )
	{
		Button Pressed = ( Button )sender;
		if( Pressed.ID == "Button_CISMonitor" )
		{
			Response.Redirect( "http://Deploy/Builder/CISMonitor/publish.htm" );
		}
		else if( Pressed.ID == "Button_Controller" )
		{
			Response.Redirect( "http://Deploy/Builder/Controller/publish.htm" );
		}
		else if( Pressed.ID == "Button_Monitor" )
		{
			Response.Redirect( "http://Deploy/Builder/Monitor/publish.htm" );
		}
		else if( Pressed.ID == "Button_SwarmAgent" )
		{
			Response.Redirect( "http://Deploy/Swarm/SwarmAgent/publish.htm" );
		}
		else if( Pressed.ID == "Button_SwarmAgentQA" )
		{
			Response.Redirect( "http://Deploy/Swarm/SwarmAgentQA/publish.htm" );
		}
		else if( Pressed.ID == "Button_SwarmCoordinator" )
		{
			Response.Redirect( "http://Deploy/Swarm/SwarmCoordinator/publish.htm" );
		}
	}
}
