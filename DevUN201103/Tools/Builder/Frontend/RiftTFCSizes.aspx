﻿<%@ Page Language="C#" AutoEventWireup="true" CodeFile="RiftTFCSizes.aspx.cs" Inherits="RiftTFCSizes" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head id="Head1" runat="server">
    <title></title>
</head>
<body>
<center>
    <form id="form1" runat="server">
    <asp:Label ID="Label_Title" runat="server" Font-Bold="True" Font-Names="Arial" Font-Size="X-Large" ForeColor="Blue" Height="48px" Text="Rift Texture File Cache sizes (MB) vs. Date (Last 150 Days)" Width="720px"></asp:Label>
<br />
    <div>
    
        <asp:Chart ID="RiftTFCSizesChart" runat="server" Height="1000px" Width="1500px">
            <Legends>
                <asp:Legend Name="Legend1" Alignment="Center" Docking="Bottom">
                </asp:Legend>
            </Legends>
            <series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="RiftTexturesTFCSize" 
                    Legend="Legend1" Color="Red" >
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="RiftCharTexturesTFCSize" 
                    Legend="Legend1" Color="Blue">
                </asp:Series>
                <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="RiftLightingTFCSize" 
                    Legend="Legend1" Color="Green">
                </asp:Series>
               <asp:Series ChartArea="ChartArea1" ChartType="Line" Name="RiftCookedFolderSize" 
                    Legend="Legend1" Color="Black">
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

