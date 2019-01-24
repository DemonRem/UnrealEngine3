using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Web.Mvc;

namespace CrashReport.Models
{
    public interface ICrashRepository
    {

        
        Crash Get(int id);
        Crash GetByAutoReporterId(int id);
        FunctionCall GetFunctionCall(int id);
        Crash_FunctionCall GetCrashFunctionCall(int CrashId, int FunctionCallId);
        void SubmitChanges();
        IQueryable<Crash> ListAll();
        int Count();
        IQueryable<Crash> List(int skip, int take);
        IQueryable<Crash> Search(IQueryable<Crash> Results, string query, Boolean OneQuery);
        CrashesViewModel GetResults(String SearchQuery, int Page, int PageSize, string SortTerm, string SortOrder, string PreviousOrder, string UserGroup, string DateFrom, string DateTo, string GameName, Boolean OneQuery);
        IQueryable<Crash> FilterByDate(IQueryable<Crash> Results, string DateFrom, string DateTo);
        IQueryable<Crash> GetSortedResults(IQueryable<Crash> Results, string SortTerm, string SortOrder);
        CallStackContainer GetCallStack(Crash crash);
        CallStackContainer CreateCallStackContainer(Crash crash);
        void BuildPattern(Crash crash);
        IQueryable<Crash> FindByPattern(string pattern);

    }
}
