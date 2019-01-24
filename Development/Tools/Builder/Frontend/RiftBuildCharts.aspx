<%@ Page Language="C#" AutoEventWireup="true" CodeFile="RiftBuildCharts.aspx.cs" Inherits="RiftBuildCharts" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title>Builder Performance Charts</title>
</head>
<body>
<center>
    <form id="RiftBuildChartsForm" runat="server">
    
<asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large" ForeColor="Blue" Height="48px" Text="Build System Performance Charts" Width="640px"></asp:Label>
<br />
<asp:Label ID="Label_Welcome" runat="server" Height="32px" Width="384px" Font-Bold="True" Font-Names="Arial" Font-Size="Small" ForeColor="Blue"></asp:Label>
    <div>
<asp:Button ID="Button_RiftDVDSize" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift DVD Size" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_RiftTextureSize" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift Texture Size" OnClick="Button_PickChart_Click" />
<br />
<br />
<br /><asp:Button ID="Button_RiftTFCSizes" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift TFC Sizes" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_RiftCookTimes" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift Cook Times" OnClick="Button_PickChart_Click" />
<br />
<br />
<asp:Button ID="Button_RiftShaderTimes" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Rift Shader Compile Times" OnClick="Button_PickChart_Click" />
<br />
<br />
    </div>
    </form>
    </center>
</body>
</html>
