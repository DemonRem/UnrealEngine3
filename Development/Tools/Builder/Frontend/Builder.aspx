<%@ Page Language="C#" AutoEventWireup="true" CodeFile="Builder.aspx.cs" Inherits="Builder" Debug="true" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title>Trigger a Build</title>
</head>
<body>

<center>
    <form id="form1" runat="server">
    <div>
        <asp:Label ID="Label1" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large"
            Height="56px" Text="Epic Build System - Trigger Build" Width="640px" ForeColor="Blue"></asp:Label><br /><br />
        <asp:Label ID="Label_Welcome" runat="server" Height="32px" Width="384px" Font-Bold="True" Font-Names="Arial" Font-Size="Small" ForeColor="Blue"></asp:Label><br />
        <br />
            <asp:SqlDataSource ID="BuilderDBSource" runat="server" ConnectionString="<%$ ConnectionStrings:Perf_BuildConnectionString %>"
            SelectCommand="SELECT [Description], [Command] FROM [Commands]"></asp:SqlDataSource>
        <asp:Repeater ID="BuilderDBRepeater" runat="server" DataSourceID="BuilderDBSource" OnItemCommand="BuilderDBRepeater_ItemCommand">
        <ItemTemplate>
        <asp:Button runat="server" Font-Bold="True" Height="26px" Text=<%# DataBinder.Eval(Container.DataItem, "Description") %> Width="384px" />
        <br /><br />
        </ItemTemplate>
        </asp:Repeater>
        </div>
    </form>

</center>    
</body>
</html>

