<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <li class="crash">
    <h3 class="summary">Crash on exiting Matinee.</h3>  
         <span class='status'>New</span><span class="id">89246</span>  <span class="game">Gear</span>  <span class="mode">Editor</span>  <span class="platform">PC 64-bit</span>  <span class="changelist">527794</span>  
        <div class="callstack">Fatal error!
                                                UInterpTrackInstFloatMaterialParam::RestoreActorState()
                                                UInterpGroupInst::RestoreGroupActorState()
                                                WxInterpEd::OnClose()
         </div>  
        <span class="username">ThomasBrowett</span>  <span class="computername">TBROWETT-X-30</span>  <span class="ttp">0</span>
        <span class="description">Crash occurred after working with animating material properties in matinee, using tracks copied from another sequence</span>      
    </li>
</body>
</html>
