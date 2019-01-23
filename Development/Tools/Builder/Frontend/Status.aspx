<%@ Page Language="C#" AutoEventWireup="true" CodeFile="Status.aspx.cs" Inherits="Status" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head id="Head1" runat="server">
    <title>Status</title>
    <meta http-equiv="refresh" content="1" />
</head>
<body>
    <form id="StatusForm" runat="server">
    <div style="text-align: center">
        <asp:Label ID="Label1" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large"
            ForeColor="Blue" Height="56px" Text="Epic Build System - Build Status" Width="640px"></asp:Label><br />
        <br />
    <asp:Label runat="server" ID="TimerLabel" Font-Bold="True" Font-Names="Arial" ForeColor="DarkGreen" ></asp:Label>
        <br />
        <br />
        <asp:Label ID="ChangeListLabel" runat="server" Font-Bold="True" Font-Names="Arial"
            ForeColor="DarkGreen"></asp:Label><br />
        <br />
        <asp:Label ID="StatusLabel" runat="server" Font-Bold="True" Font-Names="Arial" ForeColor="DarkGreen"></asp:Label><br />
        <br />
        <asp:Button ID="Button_StopBuild" runat="server" Font-Bold="True" Font-Names="Arial"
            Font-Size="Large" ForeColor="Red" Height="32px" OnClick="Button_StopBuild_Click"
            Text="STOP BUILD!" Width="256px" /><br />
            </div>
    </form>
</body>
</html>
