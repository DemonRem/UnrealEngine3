using System;
using System.Collections.Generic;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

public partial class RiftBuildCharts : System.Web.UI.Page
{
	protected void Page_Load( object sender, EventArgs e )
	{
		string LoggedOnUser = Context.User.Identity.Name;
		string MachineName = Context.Request.UserHostName;
		
		Label_Welcome.Text = "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"";
	}

	protected void Button_PickChart_Click( object sender, EventArgs e )
	{
		Button Pressed = ( Button )sender;
		if( Pressed.ID == "Button_RiftCookTimes" )
		{
			Response.Redirect( "RiftCookTimes.aspx" );
		}
		else if( Pressed.ID == "Button_RiftTFCSizes" )
		{
			Response.Redirect( "RiftTFCSizes.aspx" );
		}
		else if( Pressed.ID == "Button_RiftDVDSize" )
		{
			Response.Redirect( "RiftDVDSize.aspx" );
		}
		else if( Pressed.ID == "Button_RiftTextureSize" )
		{
			Response.Redirect( "RiftTextureSize.aspx" );
		}
		else if( Pressed.ID == "Button_RiftLightingSize" )
		{
			Response.Redirect( "RiftLightingSize.aspx" );
		}
		else if( Pressed.ID == "Button_RiftShaderTimes" )
		{
			Response.Redirect( "PCSCompileTimes.aspx" );
		}
	}
}
