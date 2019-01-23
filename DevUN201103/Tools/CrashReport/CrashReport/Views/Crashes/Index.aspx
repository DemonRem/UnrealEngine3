<%@ Page Title="" Language="C#" MasterPageFile="~/Views/Shared/Site.Master" Inherits="System.Web.Mvc.ViewPage<CrashesViewModel>" %>
<%@ Import Namespace="CrashReport.Models" %>


<asp:Content ID="Content6" ContentPlaceHolderID="CssContent" runat="server">
	<link href="../../Content/CrashesIndex.css" rel="stylesheet" type="text/css" />
</asp:Content>

<asp:Content ID="Content1" ContentPlaceHolderID="TitleContent" runat="server">
	Crash Reports
</asp:Content>

<asp:Content ID="Content3"  ContentPlaceHolderID="PageTitle" runat="server" >
Crashes
</asp:Content>
<asp:Content ID="Content5"  ContentPlaceHolderID="ScriptContent" runat="server" >
<script type="text/javascript">
    $(function () {
        $("#dateFrom").datepicker({ maxDate: '+0D' });
        $("#dateTo").datepicker({ maxDate: '+0D' });
    });
     </script>


            <script>

                $(document).ready(function () {
                    //Select All
                    $("#CheckAll").click(function () {
                        $(":checkbox").attr('checked', true);
                        $("#CheckAll").css("color", "Black");
                        $("#CheckNone").css("color", "Blue");
                        $("#SetInput").unblock();
                    });
                    //Select None
                    $("#CheckNone").click(function () {
                        $(":checkbox").attr('checked', false);
                        $("#CheckAll").css("color", "Blue");
                        $("#CheckNone").css("color", "Black");
                        $("#SetInput").block({
                            message: null
                        });
                    });
                    //Shift Check box
                    $(":checkbox").shiftcheckbox();
                    //Zebrastripes
                    $("#CrashesTable tr:nth-child(even)").css("background-color", "#C3CAD0");


                    $.blockUI.defaults.overlayCSS.top = " -6pt";
                    $.blockUI.defaults.overlayCSS.left = " -6pt";
                    $.blockUI.defaults.overlayCSS.padding = "6pt";
                    $.blockUI.defaults.overlayCSS.backgroundColor = "#eeeeee";

                    $("#SetInput").block({
                        message: null

                    });
                    $("input:checkbox").click(function () {
                        var n = $("input:checked").length;
                        if (n > 0) {
                            $("#SetInput").unblock();
                        } else {
                            $("#SetInput").block({
                                message: null
                            });
                        }

                    });


                });

                </script>
</asp:Content>

<asp:Content ID="Content4"  ContentPlaceHolderID="AboveMainContent" runat="server" >
    <br style='clear' />
<div id='SearchForm'>
<%using (Html.BeginForm("", "Crashes", FormMethod.Get))
  { %>
  <%=Html.HiddenFor(u => u.UserGroup)%>

     <%=Html.Hidden("SortTerm", Model.Term)%>
      <%=Html.Hidden("SortOrder", Model.Order)%>

  <div id="SearchBox" ><%= Html.TextBox("SearchQuery", Model.Query, new { width = "1000" })%>  <input type="submit" value="Search" class='SearchButton' /></div>
    
    <script>$.datepicker.setDefaults($.datepicker.regional['']);</script>
    
    <span style="margin-left: 10px; font-weight:bold;">Filter by Date </span>
<span  >From: <input id="dateFrom" name="dateFrom" type="text" value="<%=Model.DateFrom %>" AUTOCOMPLETE=OFF/>
  </span>

<span >To: <input  id="dateTo" name="dateTo" type="text" value="<%=Model.DateTo %>" AUTOCOMPLETE=OFF/>
 </span>
 <span style="margin-left: 10px; font-weight:bold;">Filter Game Name: </span>
 <span ><input  id="GameName" name="GameName" type="text" value="<%=Model.GameName %>" AUTOCOMPLETE=OFF/>
 </span>
<%} %>
</div>

</asp:Content>
<asp:Content ID="Content2" ContentPlaceHolderID="MainContent" runat="server">

<div id='CrashesTableContainer'>

    <div id='UserGroupBar'>
        <span class = <%if (Model.UserGroup == "All"){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="AllTab">
            <%= Url.UserGroupLink("All", Model)%> 
                   <span class="UserGroupResults">
                        (<%=Model.AllResultsCount%>)
                    </span>
        </span>
        <span class = <%if (Model.UserGroup == "General"){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="GeneralTab">
            <%= Url.UserGroupLink("General", Model)%> 
                     <span class="UserGroupResults">
                        (<%=Model.GeneralResultsCount%>)
                     </span>
        </span>
        <span class = <%if (Model.UserGroup == "Coder"){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="CoderTab">
            <%= Url.UserGroupLink("Coder", Model)%>
                <span class="UserGroupResults">
                    (<%=Model.CoderResultsCount%>)
                </span>
        </span>
        <span class = <%if (Model.UserGroup == "Tester"){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="TesterTab">
            <%= Url.UserGroupLink("Tester", Model)%>
                <span class="UserGroupResults">
                    (<%=Model.TesterResultsCount%>)
                </span>
        </span>
        <span class = <%if (Model.UserGroup == "Automated"){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="AutomatedTab">
            <%= Url.UserGroupLink("Automated", Model)%>
                    <span class="UserGroupResults">
                    (<%=Model.AutomatedResultsCount%>)
                    </span>
       </span>
    </div>
   <span style="font-size: medium; margin-left: 10px;"> <%=Html.ActionLink("Buggs", "Index", "Buggs", new { }, new { style = "color:white" })%></span>
   
    <div id='SetForm'>
<%using (Html.BeginForm("", "Crashes"))
  { %>
    


</div>

    <br sytle='clear:both' />

     

 <% Html.RenderPartial("/Views/Crashes/ViewCrash.ascx"); %>

 <%} %>

    <div class="PaginationBox">
       <%= Html.PageLinks(Model.PagingInfo, i => Url.Action("", new { page = i, SearchQuery = Model.Query, SortTerm = Model.Term, SortOrder = Model.Order, UserGroup = Model.UserGroup, DateFrom = Model.DateFrom, DateTo = Model.DateTo }))%>
    </div>
</div>

</asp:Content>
