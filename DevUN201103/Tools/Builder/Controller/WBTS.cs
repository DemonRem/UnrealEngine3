/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Globalization;

namespace Controller
{
	public class submission
	{
		[XmlElement]
		public int project_id { get; set; }

		[XmlElement]
		public string sku_name { get; set; }

		[XmlElement]
		public string milestone { get; set; }

		[XmlElement]
		public string version { get; set; }

		[XmlElement]
		public string change_list { get; set; }

		[XmlElement]
		public string build_date { get; set; }

		[XmlElement]
		public string media_type { get; set; }

		[XmlElement]
		public string encryption_type { get; set; }

		[XmlElement]
		public string release_status { get; set; }

		[XmlElement]
		public string cqc { get; set; }

		[XmlElement]
		public string notes { get; set; }

		[XmlElement]
		public string email { get; set; }

		[XmlElement]
		public string filename { get; set; }

		public submission()
		{
			project_id = -1;
			version = "-1";
			change_list = "-1";

			build_date = DateTime.UtcNow.ToString( "yyyy-MM-dd" );

			sku_name = "WW";
			milestone = "current";
			media_type = "DVD9";
			encryption_type = "no encryption";
			release_status = "internal_only";
			cqc = "";
			notes = "";
			email = "";
			filename = "";
		}
	}
}
