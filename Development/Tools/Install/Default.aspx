<%@ Page Language="C#" AutoEventWireup="true"  CodeFile="Default.aspx.cs" Inherits="_Default" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title></title>
</head>
<body>
    <form id="DeployForm" runat="server">
    <div>
    <center>
        <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large" ForeColor="Blue" Height="48px" Text="Epic Deployment System" Width="640px">
        </asp:Label>
    <br />
        <asp:Label ID="Label_Welcome" runat="server" Height="32px" Width="384px" Font-Bold="True" Font-Names="Arial" Font-Size="Small" ForeColor="Blue">
        </asp:Label>
    <br />

        <asp:Label ID="Label_Builder" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="DarkGreen" Height="48px" Text="Builder Applications" Width="640px">
        </asp:Label>
    <br />
    <br />
            
    <asp:Button ID="Button_CISMonitor" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="CIS Monitor" OnClick="Button_TriggerBuild_Click" />
<br />
<br />
    <asp:Button ID="Button_Controller" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="Builder Controller (Internal)" OnClick="Button_TriggerBuild_Click" />
<br />
<br />
    <asp:Button ID="Button_Monitor" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="Builder Monitor (Internal)" OnClick="Button_TriggerBuild_Click" />
<br />
<br />
<br />
<br />
        <asp:Label ID="Label_Swarm" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="DarkGreen" Height="48px" Text="Swarm Applications" Width="640px">
        </asp:Label>
    <br />
    <br />
            
    <asp:Button ID="Button_SwarmAgent" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="Swarm Agent" OnClick="Button_TriggerBuild_Click" />
<br />
<br />
    <asp:Button ID="Button_SwarmAgentQA" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="Swarm Agent (QA)" OnClick="Button_TriggerBuild_Click" />
<br />
<br />
    <asp:Button ID="Button_SwarmCoordinator" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" Text="Swarm Coordinator (Internal)" OnClick="Button_TriggerBuild_Click" />
<br />
<br />

    </center>
    
    </div>
    </form>
</body>
</html>
