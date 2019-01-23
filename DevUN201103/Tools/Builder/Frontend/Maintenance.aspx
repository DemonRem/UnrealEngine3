<%@ Page Language="C#" AutoEventWireup="true" CodeFile="Maintenance.aspx.cs" Inherits="Maintenance" Debug="true" %>

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
            Height="56px" Text="Epic Build System - Perform Builder Maintenance" Width="640px" ForeColor="Blue"></asp:Label><br /><br />
        <asp:Label ID="Label_Welcome" runat="server" Height="32px" Width="384px" Font-Bold="True" Font-Names="Arial" Font-Size="Small" ForeColor="Blue"></asp:Label><br />
        <br />
            <asp:SqlDataSource ID="BuilderDBSource_Trigger" runat="server" ConnectionString="<%$ ConnectionStrings:BuilderConnectionString %>"
            SelectCommandType="StoredProcedure" SelectCommand="GetTriggerable">
            <SelectParameters>
            <asp:QueryStringParameter Name="MinAccess" Type="Int32" DefaultValue="10000" />
            <asp:QueryStringParameter Name="MaxAccess" Type="Int32" DefaultValue="10100" />
            </SelectParameters>
            </asp:SqlDataSource>
        <asp:Repeater ID="BuilderDBRepeater_Trigger" runat="server" DataSourceID="BuilderDBSource_Trigger" OnItemCommand="MaintenanceDBRepeater_ItemCommand">
        <ItemTemplate>
        <asp:Button runat="server" Font-Bold="True" Height="26px" Text=<%# DataBinder.Eval(Container.DataItem, "[\"Description\"]") %> CommandName=<%# DataBinder.Eval(Container.DataItem, "[\"ID\"]") %> CommandArgument=<%# DataBinder.Eval(Container.DataItem, "[\"MachineLock\"]") %> OnPreRender="MaintenanceDBRepeater_OnPreRender" Width="384px" />
        <br /><br />
        </ItemTemplate>
        </asp:Repeater>
        </div>
     
    </form>

</center>    
</body>
</html>

