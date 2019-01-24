<?xml version="1.0" encoding="UTF-8"?>
<!-- saved from url=(0014)about:internet -->
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns="http://www.w3.org/1999/xhtml">

	<xsl:template match="/ListMethods">
		<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
			<head>
				<title>PIBFileWebServices - Method List</title>
				<link href="style.css" rel="stylesheet" type="text/css"/>
				<script type="text/javascript" src="clientcode.js"></script>
			</head>
			<body onload="mainOnLoad();">
				<div id="main">
					<div id="header"></div>
					<div id="divline"></div>
					<img src="logo.png" id="logo"/>
					<table class="content">
						<xsl:for-each select="Method">
							<tr>
								<td>
									<a>
										<xsl:attribute name="href">
											<xsl:value-of select="@src"/>
										</xsl:attribute>
										<xsl:value-of select="@name"/>
									</a> - <xsl:value-of select="."/>
								</td>
							</tr>
						</xsl:for-each>
					</table>
				</div>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="/GetFileList">
		<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
			<head>
				<title>PIBFileWebServices - GetFileList</title>
				<link href="style.css" rel="stylesheet" type="text/css"/>
				<script type="text/javascript" src="clientcode.js"></script>
			</head>
			<body onload="mainOnLoad();">
				<div id="main">
					<div id="header"></div>
					<div id="divline"></div>
					<img src="logo.png" id="logo"/>
					<table class="content">
						<xsl:for-each select="Files">
							<tr>
								<td>
									<xsl:value-of select="@Name"/>
								</td>
							</tr>
						</xsl:for-each>
					</table>
				</div>
			</body>
		</html>
	</xsl:template>

</xsl:stylesheet>
