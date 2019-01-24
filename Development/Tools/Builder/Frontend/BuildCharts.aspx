<%@ Page Language="C#" AutoEventWireup="true" CodeFile="BuildCharts.aspx.cs" Inherits="BuildCharts" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title>Builder Performance Charts</title>
</head>
<body>
<center>
    <form id="BuildChartsForm" runat="server">
    
<asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large" ForeColor="Blue" Height="48px" Text="Build System Performance Charts" Width="640px"></asp:Label>
<br />
<asp:Label ID="Label_Welcome" runat="server" Height="32px" Width="384px" Font-Bold="True" Font-Names="Arial" Font-Size="Small" ForeColor="Blue"></asp:Label>
    <div>
<asp:Button ID="Button_RiftBuildCharts" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift Build Charts" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_BuildStats" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Build Stats" OnClick="Button_Trigger_Click" />
<br />
<br />
<asp:Button ID="Button_CISBuildTimes" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="CIS Build Times" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_ScriptBuildTimes" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Script Build Times" OnClick="Button_PickChart_Click" />
<br />
<br />   
<asp:Button ID="Button_CISDownTime" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="CIS Down Time" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_BuildCounts" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Total Build Counts" OnClick="Button_PickChart_Click" />
<br />
<br /> 
<asp:Button ID="Button_ActiveBuilds" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Active Build Counts" OnClick="Button_PickChart_Click" />
<br />
<br />    
<asp:Button ID="Button_BuilderResources" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Build Machine Resources" OnClick="Button_PickChart_Click" />
<br />
<br />    
<asp:Button ID="Button_BuilderDiskStats" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Build Machine Disk Stats" OnClick="Button_PickChart_Click" />
<br />
<br />   
    </div>
    </form>
    </center>
</body>
</html>
