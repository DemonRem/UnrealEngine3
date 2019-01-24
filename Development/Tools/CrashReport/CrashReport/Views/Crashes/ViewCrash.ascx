<%@ Control Language="C#" Inherits="System.Web.Mvc.ViewUserControl<CrashesViewModel>" %>
<%@ Import Namespace="CrashReport.Models" %>





     <table id ='CrashesTable' style="table-layout:auto; width:auto; min-width:1300px"  cellspacing="0" cellpadding="0">
     <tr id="SelectBar" style='background-color: #e8eef4; border: none; !important' border=0> 
        <td colspan='15' style='background-color: #e8eef4; border: none !important; border-right:none !important;' border=0>Select: <span class='SelectorLink' id='CheckAll' >All</span><span class='SelectorLink' id="CheckNone">None</span>
        <div class="PaginationBox">
        
        <%= Html.PageLinks(Model.PagingInfo, i => Url.Action("", new { page = i, SearchQuery = Model.Query, SortTerm = Model.Term, SortOrder = Model.Order, UserGroup = Model.UserGroup, DateFrom = Model.DateFrom, DateTo = Model.DateTo }))%>
    </div>
    <!-- form starts and ends in site.master-->
          <div id='SetForm'>
             
                    <%=Html.HiddenFor(u => u.UserGroup) %>
                    <%=Html.Hidden("SortTerm", Model.Term) %>
                      <%=Html.Hidden("SortOrder", Model.Order) %>
                      <%=Html.Hidden("Page", Model.PagingInfo.CurrentPage) %>
                      <%=Html.Hidden("PageSize", Model.PagingInfo.PageSize) %>
                      <%=Html.Hidden("SearchQuery", Model.Query)%>
                          <%=Html.HiddenFor(m => m.DateFrom) %>
                    <%=Html.HiddenFor(m => m.DateTo) %>
    <Div id="SetInput">
                     <span id="set-status" style="vertical-align: middle;">Set Status</span>
     
                    <select name="SetStatus" id="SetStatus" >
                        <option selected="selected" value=""></option>
	                    <option  value="Unset">Unset</option>
                        <option value="Reviewed">Reviewed</option>
	                    <option value="New">New</option>
	                    <option value="Coder">Coder</option>
	                    <option value="Tester">Tester</option>

                    </select>
                    <input type="submit" name="SetStatusSubmit" value="Set" class="SetButton"   />

                   <span id="set-ttp" style="">TTP</span>
                    <input name="SetTTP" type="text" id="ttp-text-box"   />
                    <input type="submit" name="SetTTPSubmit" value="Set" class="SetButton" />
    
                   <span id="set-fixed-in" style="">FixedIn</span>
                    <input name="SetFixedIn" type="text" id="fixed-in-text-box" />
                    <input type="submit" name="SetFixedInSubmit" value="Set" class="SetButton" />
                    
       </div>

        </div>
    
    </td>
     </tr>
     <tr>
        <th>&nbsp;</th> 
        <!-- if you add a new column be sure to update the switch statement in crashescontroller.cs -->
        <th style='width: 6em;'><%=Url.TableHeader("Id", "Id", Model)%></th>
         <!--<th>Username</th>--> 
           <th style='width: 15em'><%=Url.TableHeader("TimeOfCrash", "TimeOfCrash", Model)%></th>
          <th style='width: 12em'><%=Url.TableHeader("UserName", "UserName", Model)%></th>
         <th style='width: 12em;'><%=Url.TableHeader("CallStack", "RawCallStack", Model)%></th> 
           <th style='width: 11em;'><%=Url.TableHeader("Game", "GameName", Model)%></th> 
         <th style='width: 11em;'><%=Url.TableHeader("Mode", "EngineMode", Model)%></th> 
         <th style='width: 14em;'><%=Url.TableHeader("Platform", "PlatformName", Model)%></th> 
          <th>Summary</th> 
         <th>Description</th>
          <th style='width: 12em;'><%=Url.TableHeader("Computer", "ComputerName", Model)%></th>
          
       <!--<th><%=Url.TableHeader("Command Line", "CommandLine", Model)%></th> -->
          <th style='width: 9em;'><%=Url.TableHeader("CL#", "ChangeListVersion", Model)%></th>
         <th style='width: 16em;'><%=Url.TableHeader("FixedCL#", "FixedChangeList", Model)%></th>
         <th style='width: 9em;'><%=Url.TableHeader("TTP", "TTPID", Model)%></th>
         
         <th style='width: 12em;'><%=Url.TableHeader("Status", "Status", Model)%></th>
     </tr>
     <%if (Model.Results.ToList() != null)
       { %>
          <%
              int iter = 0;
           foreach (Crash c in (IEnumerable)Model.Results)
           {
               iter++;%>
            
            <tr class='CrashRow <%=c.User.UserGroup %>'>
                <td class="CrashTd"><INPUT TYPE="CHECKBOX" Value="<%=iter %>"NAME="<%=c.Id%>" id="<%=c.Id %>" class='input CrashCheckBox' ></td> 
                <td class="Id" ><%=Html.ActionLink(c.Id.ToString(), "Show", new { controller = "crashes", id = c.Id }, null)%></td> 
                <td class="TimeOfCrash">
                     <%=c.GetTimeOfCrash()[0]%><br />
                    <%=c.GetTimeOfCrash()[1]%>
                    <%=c.GetTimeOfCrash()[2]%>
                </td>
                <td class="Username"><%=c.UserName%></td>
                  <td class="CallStack" >
                  <div style="clip : auto; ">
                        <div id='<%=c.Id %>-TrimmedCallStackBox' class='TrimmedCallStackBox'>

                        <% 
                            int i = 0;
                            foreach (string error in c.GetCallStackErrors(5,50, "Address"))
                           {  %>
                        <%=Html.Encode(error)%>
                            
                        <%
                            i++;
                            if (i > 2) break;
                            } %>
                            <br />
                        <%
                            foreach(CallStackEntry Entry in c.GetCallStackEntries(4) )
                            {
                              %>

                         <span class = "function-name">
                        <%=Url.CallStackSearchLink( Html.Encode(Entry.GetTrimmedFunctionName(45) ), Model )%>
                        </span><br />
                        <%} %>
                        </div>
                        
                        
                            <a class='FullCallStackTrigger' ><span class='FullCallStackTriggerText'>Full Callstack</span>
                                <div id='<%=c.Id %>-FullCallStackBox' class='FullCallStackBox' >
                                    <%foreach(CallStackEntry Entry in c.GetCallStackEntries(60) )
                                        {%>
                                                <span class = "FunctionName">
                                                    <%=Html.Encode(Entry.GetTrimmedFunctionName(0))%>
                                                </span>
                                                <span class = "FileName">
                                                    <%=Html.DisplayFor(m => Entry.FileName) %>
                                                </span>
                                                <span class = "LineNumber">
                                                    <em><%=Html.DisplayFor(m => Entry.LineNumber) %></em>
                                                </span>
                                                <br />
                                    <%} %>
                            
                                </div>
                            </a>
                </div>
                </td> 
                <td class="Game"><%=c.GameName%></td> 
                <td class="Mode"><%=c.EngineMode%></td> 
                <td class="Platform"><%=c.PlatformName%></td> 
                
                 <td class="Summary"><%=c.Summary%>&nbsp;</td> 
                <td class="Description"><span class="TableData"><%=c.Description%>&nbsp;</span></td> 
                       
                
                <td class="Computer"><%=c.ComputerName%></td>
                
                
              <!--  <td class="Commandline" width="150">
                        <div stlye="width:150px; overflow-x: scroll; display: none;">
                        <%= c.CommandLine %>
                        </div>
                 </td> -->
      
                
                <td class="ChangeListVersion"><%=c.ChangeListVersion%></td>
                <td class="FixedChangeList"><%=c.FixedChangeList%></td>
                <td class="Ttp"><%=c.TTPID%>&nbsp;</td>
                <td class="Status"><%=c.Status%></td>
            </tr>
        
          <% } %>
        <%} %>
       
             </Table>




    