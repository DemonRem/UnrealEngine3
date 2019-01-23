using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Drawing;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

public partial class _Default : BasePage
{
    protected void Page_Load( object sender, EventArgs e )
    {
        Button_TriggerBuild.Enabled = IsControllerAvailable();
    }
    protected void Button_TriggerBuild_Click( object sender, EventArgs e )
    {
        Server.Transfer( "Builder.aspx" );
    }
}
