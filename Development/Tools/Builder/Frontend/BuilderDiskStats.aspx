<%@ Page Language="C#" AutoEventWireup="true" CodeFile="BuilderDiskStats.aspx.cs" Inherits="BuilderDiskStats" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title></title>
</head>
<body>
<center>
    <form id="form1" runat="server">
        <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" 
            Font-Size="X-Large" ForeColor="Blue" Height="48px" 
            Text="Build Machine Disk Stats vs. Time of Day (Last 60 Days)" Width="640px"></asp:Label>
<br />
    <div>
        <asp:Chart ID="DiskQueueChart" runat="server" Width="1500px" Height="500px" 
            IsMapAreaAttributesEncoded="True">
            <Legends>
                <asp:Legend Alignment="Center" Docking="Bottom" Name="Legend1">
                </asp:Legend>
            </Legends>
            <Series>
                <asp:Series ChartArea="ChartArea2" ChartType="StackedArea" Legend="Legend1" 
                    Name="DiskQueueLength" XValueType="Time">
                </asp:Series>
                <asp:Series ChartArea="ChartArea2" ChartType="StackedArea" Legend="Legend1" 
                    Name="DiskReadQueueLength" XValueType="Time">
                </asp:Series>
            </Series>
            <ChartAreas>
                <asp:ChartArea Name="ChartArea2">
                    <AxisY Interval="100" IntervalType="Number">
                    </AxisY>
                    <AxisX Interval="1" IntervalType="Hours">
                    </AxisX>
                </asp:ChartArea>
            </ChartAreas>
        </asp:Chart>
    </div>
    </form>
    <p>
        <asp:Chart ID="DiskLatencyChart" runat="server" Width="1500px" Height="500px">
            <Legends>
                <asp:Legend Alignment="Center" Docking="Bottom" Name="Legend1">
                </asp:Legend>
            </Legends>
            <Series>
                <asp:Series Name="DiskReadLatency" ChartType="Line" Legend="Legend1" 
                    XValueType="Time"></asp:Series>
                <asp:Series Name="DiskWriteLatency" ChartType="Line" Legend="Legend1" 
                    XValueType="Time"></asp:Series>
                <asp:Series Name="DiskTransferLatency" ChartType="Line" Legend="Legend1" 
                    XValueType="Time"></asp:Series>
            </Series>
            <ChartAreas>
                <asp:ChartArea Name="ChartArea1">
                    <AxisX Interval="1" IntervalType="Hours">
                    </AxisX>
                </asp:ChartArea>
            </ChartAreas>
        </asp:Chart>
    </p>
</center>
</body>
</html>
