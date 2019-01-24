<%@ Page Language="C#" AutoEventWireup="true" CodeFile="ScriptBuildTimes.aspx.cs" Inherits="ScriptBuildTimes" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head id="Head1" runat="server">
    <title></title>
</head>
<body>
<center>
    <form id="form1" runat="server">
    <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="Blue" Height="48px" Text="Time of Script Compile vs. Date (Last 90 Days)" Width="640px"></asp:Label>
<br />
    <div>
    
        <asp:Chart ID="CISCompileChart" runat="server" Height="1000px" Width="1500px">
            <Legends>
                <asp:Legend Name="Legend1" Alignment="Center" Docking="Bottom">
                </asp:Legend>
            </Legends>
            <series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="ScriptExample" 
                    Legend="Legend1" BorderWidth="2" Color="Red" >
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="ScriptExampleFR" 
                    Legend="Legend1" Color="Maroon">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptGear" BorderWidth="2" Color="Lime">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptGearFR" Color="Green">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptNano" BorderWidth="2" Color="Blue">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptNanoFR" Color="Navy">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptUDK" BorderWidth="2" Color="Yellow">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="ScriptUDKFR" Color="Olive">
                </asp:Series>
            </series>
            <chartareas>
                <asp:ChartArea Name="ChartArea1">
                </asp:ChartArea>
            </chartareas>
        </asp:Chart>
    
    </div>
    </form>
    </center>
</body>
</html>

