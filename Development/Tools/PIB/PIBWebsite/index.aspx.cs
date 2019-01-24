/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

namespace PIBWebsite
{
	public partial class PIBPage : System.Web.UI.Page
	{
		public string BrowserName = "Unknown";

		protected void Page_Load( object sender, EventArgs e )
		{
			System.Web.HttpBrowserCapabilities Browser = Request.Browser;

			// IE/Firefox/AppleMAC-Safari/Opera
			BrowserName = Browser.Browser;
		}
	}

	public class PIBPanel : System.Web.UI.WebControls.Panel
	{
		protected override void Render( HtmlTextWriter Writer )
		{
			PIBPage MainPage = ( PIBPage )Page;

			switch( MainPage.BrowserName )
			{
			case "IE":
				Writer.Write( "<OBJECT ID=\"ATLUE3Control\" CLASSID=\"CLSID:8AF65954-58E5-4DC5-8DD0-A28B13E77AC7\" width=\"946\" height=\"470\" codebase=\"http://pib/ATLUE3.cab#version=1,0,6717,0\">" );
				Writer.Write( "</OBJECT>" );
				break;

			case "Firefox":
			case "Opera":
			case "AppleMAC-Safari":
			default:
				Writer.Write( "<embed type=\"application/ue3-plugin\" width=\"946\" height=\"470\" />" );
				break;
			}
		}
	}
}