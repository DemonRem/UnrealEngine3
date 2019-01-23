<%@ Page Language="C#" AutoEventWireup="true" CodeFile="Default.aspx.cs" Inherits="_Default" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title>Epic Build System - Main Page</title>
</head>
<body>
<center>
<asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="XX-Large"
ForeColor="Blue" Height="48px" Text="Epic Build System" Width="320px"></asp:Label>
<br />
<br />

<form ID="DefaultForm" runat="server">
<asp:Button ID="Button_TriggerBuild" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="Large" Text="Trigger Build" OnClick="Button_TriggerBuild_Click" />
</form>

</center>
<center>
&nbsp;</center>
<center>
<asp:SqlDataSource ID="BuilderDBSource_Builders" runat="server" ConnectionString="<%$ ConnectionStrings:Perf_BuildConnectionString %>"
SelectCommand="SELECT [Machine], [State] FROM [Builders] WHERE (([State] <> 'Dead') AND ([State] <> 'Zombied'))"></asp:SqlDataSource>
<asp:Repeater ID="Repeater_Status_Builders" runat="server" DataSourceID="BuilderDBSource_Builders">
<ItemTemplate>
Controller
<asp:Label ID="Label_Builder1" runat="server" Font-Bold="True" ForeColor="DarkGreen" Text=<%# DataBinder.Eval(Container.DataItem, "Machine") %> />
is
<asp:Label ID="Label_Builder2" runat="server" Font-Bold="True" ForeColor="DarkGreen" Text=<%# DataBinder.Eval(Container.DataItem, "State") %> />
</ItemTemplate>
</asp:Repeater>
</center>
<center>
<br />
<asp:SqlDataSource ID="BuilderDBSource_Commands" runat="server" ConnectionString="<%$ ConnectionStrings:Perf_BuildConnectionString %>"
SelectCommand="SELECT [Description],[LastGoodChangeList], [LastGoodDateTime] FROM [Commands]"></asp:SqlDataSource>
<asp:Repeater ID="Repeater_Status_Commands" runat="server" DataSourceID="BuilderDBSource_Commands">
<ItemTemplate>
<asp:Label ID="Label_Status1" runat="server" Font-Bold="True" ForeColor="DarkGreen" Text=<%# DataBinder.Eval(Container.DataItem, "Description") %> />
from ChangeList 
<asp:Label ID="Label_Status2" runat="server" Font-Bold="True" ForeColor="DarkGreen" Text=<%# DataBinder.Eval(Container.DataItem, "LastGoodChangeList") %> />
on 
<asp:Label ID="Label_Status3" runat="server" Font-Bold="True" ForeColor="DarkGreen" Text=<%# DataBinder.Eval(Container.DataItem, "LastGoodDateTime") %> /><br />
</ItemTemplate>
</asp:Repeater>
</center>
    
</body>
</html>
