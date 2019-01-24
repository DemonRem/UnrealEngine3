<%@ Page Language="C#" AutoEventWireup="true" CodeFile="BuilderResources.aspx.cs" Inherits="BuilderResources" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title></title>
</head>
<body>
<center>
    <form id="form1" runat="server">
        <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="Blue" Height="48px" Text="Build Machine Hardware Resources vs. Date (Last 60 Days)" Width="800px"></asp:Label>
<br />
    <div>
    <asp:Chart ID="BuildResourcesChart0" runat="server" Height="500px" 
        IsMapEnabled="False" Palette="Pastel" Width="1500px">
        <Legends>
            <asp:Legend Alignment="Center" Docking="Bottom" Name="Legend1">
            </asp:Legend>
        </Legends>
        <Series>
            <asp:Series Legend="Legend1" Name="CPUBusy">
            </asp:Series>
        </Series>
        <ChartAreas>
            <asp:ChartArea Name="ChartArea1">
            </asp:ChartArea>
        </ChartAreas>
    </asp:Chart>
    </div>
    <div>
        <asp:Chart ID="BuildResourcesChart1" runat="server" Height="500px" 
            IsMapEnabled="False" Palette="Berry" Width="1500px">
            <Legends>
                <asp:Legend Alignment="Center" Docking="Bottom" Name="Legend1">
                </asp:Legend>
            </Legends>
            <Series>
                <asp:Series Legend="Legend1" Name="UsedMemory">
                </asp:Series>
            </Series>
            <ChartAreas>
                <asp:ChartArea Name="ChartArea1">
                </asp:ChartArea>
            </ChartAreas>
        </asp:Chart>
    </div>
    </form>
</center>
</body>
</html>
