<%@ Page Language="C#" AutoEventWireup="true" CodeFile="CISDownTime.aspx.cs" Inherits="CISDownTime" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title></title>
</head>
<body>
<center>
    <form id="form1" runat="server">
        <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="Blue" Height="48px" Text="CIS Weekly Average Down Time in Hours in the Past Year" Width="800px"></asp:Label>
<br />    
<div>
        <asp:Chart ID="CISDownTimeChart" runat="server" Height="1000px" Width="1500px">
            <Legends>
                <asp:Legend Alignment="Center" Docking="Bottom" Name="Legend1">
                </asp:Legend>
            </Legends>
            <Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimeExample" Color="#33CCFF" BorderWidth="2">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimeGear" Color="Red" BorderWidth="2">
                </asp:Series>            
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimeNano" Color="#3333CC" BorderWidth="2">
                </asp:Series>            
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimeUT" Color="#996600" BorderWidth="2">
                </asp:Series>                        
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimeXbox360" Color="#009933" BorderWidth="2">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Legend="Legend1" 
                    Name="CISDownTimePS3" Color="#CC00CC" BorderWidth="2">
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
