using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using CrashReport.Models;
using Naspinski.IQueryableSearch;
using System.Linq.Dynamic;
using System.Diagnostics;
using System.Web.Mvc;
using System.Data.Linq.SqlClient;
using System.Data.Linq;


namespace CrashReport.Models
{
    public class CrashRepository : ICrashRepository
    {
        private CrashReportDataContext mCrashDataContext;

        public CrashRepository()
        {
                mCrashDataContext = new CrashReportDataContext();
        }
        public CrashRepository(CrashReportDataContext Context)
        {
            mCrashDataContext = Context;
        }

        #region ICrashRepository Members

        public Crash Get(int id)
        {
            IQueryable<Crash> crash =
                (
                    from c in mCrashDataContext.Crashes
                    where c.Id == id
                    select c

                );


            return (Crash)crash.FirstOrDefault();

        }
        public Crash GetByAutoReporterId(int id)
        {
            IQueryable<Crash> crash =
                (
                    from c in mCrashDataContext.Crashes
                    where c.AutoReporterID == id
                    select c
                )
                ;
            return (Crash)crash.FirstOrDefault();
        }
        public FunctionCall GetFunctionCall(int id)
        {
            var FunctionCall = mCrashDataContext.FunctionCalls.Where(f => f.Id == id).First();
            return FunctionCall;
        }
       public Crash_FunctionCall GetCrashFunctionCall(int CrashId, int FunctionCallId)
       {
           var CrashesFunctionCall = mCrashDataContext.Crash_FunctionCalls.Where(c => c.CrashId == CrashId && c.FunctionCallId == FunctionCallId).First();
           return CrashesFunctionCall;
       }

       public void SubmitChanges()
       {
           // Turn on the logging.
           this.mCrashDataContext.Log = Console.Out;
           try
           {
               this.mCrashDataContext.SubmitChanges(ConflictMode.ContinueOnConflict);
           }
           catch (ChangeConflictException)
           {
               this.mCrashDataContext.ChangeConflicts.ResolveAll(RefreshMode.KeepChanges);
           }
           catch (Exception e)
           {
               //TODO Catch The Proper exceptions and do something with them
               //for now do nothing, just eat the exception
               var changeSet = this.mCrashDataContext.GetChangeSet();
               Console.Write(this.mCrashDataContext.ChangeConflicts);
               Console.Write(this.mCrashDataContext.GetChangeSet().ToString());
           }
       }
        public IQueryable<Crash> FindByPattern(string pattern)
        {
            //Don't need the % at the beginning or end if you're bulding it that way.
            //string p = "%"+pattern+"%";

            IQueryable<Crash> crashes =
            (
                from c in mCrashDataContext.Crashes
                where SqlMethods.Like(c.Pattern, pattern)
                select c
            ).OrderByDescending(c => c.TimeOfCrash);
            ;
            return crashes;
        }
        public IQueryable<Crash> ListAll()
        {

            var crashes =
                (
                    from c in mCrashDataContext.Crashes
                    select c
                ).OrderByDescending(c => c.TimeOfCrash)
                ;
            return crashes;

        }
        public int Count()
        {
            var results = ListAll().Count();
            return results;
        }

        public IQueryable<Crash> List(int skip, int take)
        {
            var crashes = 
                (
                from c in mCrashDataContext.Crashes
                select c
               )
            .OrderByDescending(c => c.TimeOfCrash) 
            .Skip(skip)
            .Take(take)
            
             ;
            return crashes;

        }

        public IQueryable<Crash> Search(IQueryable<Crash> Results, string query)
        { 
            bool OneQuery = false;
            return this.Search(Results, query, OneQuery);
        }

        public IQueryable<Crash> Search(IQueryable<Crash> Results, string query, bool OneQuery)
        {
            IQueryable<Crash> crashes;
            try
            {

                var q = HttpUtility.HtmlDecode(query.ToString());
                if (q == null) q = "";

                if (OneQuery)
                {
                    string[] Term = { q };

                    crashes = (IQueryable<Crash>)Results
                        .Search(Term);
                }
                else
                {
                    char[] SearchDelimiters = { ',', ' ', ';', '+' };
                    // take out terms starting with a - 
                    var terms = q.Split(SearchDelimiters);
                    var excludes = new List<string>();
                    string TermsToUse = "";
                    foreach (string term in terms)
                    {
                        if (term.StartsWith("-"))
                        {
                            char[] ExcludingCharacters = { '-' };
                            excludes.Add(term.Trim(ExcludingCharacters));
                        }
                        else
                        {
                            if (term != "" && term != " " && term != "-")
                                if (TermsToUse.Contains(term))
                                {
                                    //do nothing to ensure that we're only using unique terms
                                }
                                else
                                {
                                    TermsToUse = TermsToUse + "+" + term;
                                }
                        }
                    }
                    //   string[] ColumnsToSearch = { "RawCallStack" };
                    //  IQueryable<Crash> crashes = (IQueryable<Crash>)this._dataContext.Crashes.Search(new string[] { "GameName" }, new object[] { "Gear" });
                    crashes = (IQueryable<Crash>)Results
                        .Search(TermsToUse.Split(SearchDelimiters));

                    //remove results with the excluded terms in the callstack
                    //TODO do this for Every field or by user requested fields
                    if (excludes.Count() > 0)
                        foreach (string exclude in excludes)
                        {
                            if (exclude != "" && exclude != " " && exclude != "-")
                                crashes = crashes.Except(crashes.Where(c => c.RawCallStack.Contains(exclude) || c.UserName.Contains(exclude) || c.ComputerName.Contains(exclude) ));
                        }
                }
            }
            catch (NullReferenceException e)
            {
                crashes = Results;
            }

           return crashes ;

        }

        public CrashesViewModel GetResults(String SearchQuery, int Page, int PageSize, string SortTerm, string SortOrder, string PreviousOrder, string UserGroup, string DateFrom, string DateTo, string GameName, Boolean OneQuery)
        {                                                          

            var timer1 = Stopwatch.StartNew();
            IQueryable<Crash> Results = null;
            int Skip = (Page - 1) * PageSize;
            int Take = PageSize;
            if (DateFrom == null || DateFrom == "")
            {
                var df = new DateTime();
                df = DateTime.Now;
                df = df.AddDays(-7);
                DateFrom = df.ToShortDateString();
            }
            if (DateTo == null || DateTo == "")
            {
                var dt = new DateTime();
                dt = DateTime.Now;
                DateTo = dt.ToShortDateString();

            }



            Results = ListAll();

            Results = FilterByDate(Results, DateFrom, DateTo);

            //Grab Results 
            if (SearchQuery == null || SearchQuery == string.Empty)
            {
                //Do nothing
                
            }
            else
            {
                Results = Search(Results, SearchQuery.Trim(), OneQuery);
            }

            //Start Filtering the results

         

            
            //Filter by GameName
            if (GameName == null || GameName == string.Empty)
            {
                //do nothing
            }
            else
            {
                Results = Results.Where(c => c.GameName == GameName);
            }

            // Set UserGroup = All ResultCounts
            int AllResultsCount = Results.Count();
            // Get UserGroup ResultCounts
            var GroupCounts = this.GetCountsByGroup(Results);

            int GeneralResultsCount;
            int CoderResultsCount;
            int TesterResultsCount;
            int AutomatedResultsCount;
            try
            {
                GeneralResultsCount = GroupCounts["General"];
            }
            catch (KeyNotFoundException)
            {
                GeneralResultsCount = 0;
            }
            try
            {
                CoderResultsCount = GroupCounts["Coder"];
            }
            catch (KeyNotFoundException)
            {
                CoderResultsCount = 0;
            }
            try
            {
                TesterResultsCount = GroupCounts["Tester"];
            }
            catch (KeyNotFoundException)
            {
                TesterResultsCount = 0;
            }
            try
            {
                AutomatedResultsCount = GroupCounts["Automated"];
            }
            catch (KeyNotFoundException)
            {
                AutomatedResultsCount = 0;
            }


            //Filter by Usergroup if Present
            if (UserGroup != "All")
            {
                Results = Results.Where(c => c.User.UserGroup == UserGroup);
            }


            //Pass in the results and return them sorted properly
            Results = GetSortedResults(Results, SortTerm, SortOrder);

            //Get the Count for Pagination
            int ResultCount = Results.Count();

            //Grab just the results we want to display on this page
            Results = Results.Skip(Skip).Take(Take);

            //Process Call Stack for displya
            if (Results != null)
            {
                foreach (Crash c in Results)
                {
                    //Put callstacks into an IList so we can access them line by line in the view
                    //TODO handle these in the database so that the data we're returning is all we need/want
                    c.CallStackContainer = this.GetCallStack(c);
                }
            }
            ///////////////////////////////////////////////////////////

            timer1.Stop();
            return new CrashesViewModel
            {
                Results = Results,
                Query = SearchQuery,
                PagingInfo = new PagingInfo { CurrentPage = Page, PageSize = PageSize, TotalResults = ResultCount },
                Order = SortOrder,
                Term = SortTerm,
                UserGroup = UserGroup,
                DateFrom = DateFrom,
                DateTo = DateTo,
                GameName = GameName,
                AllResultsCount = AllResultsCount,
                GeneralResultsCount = GeneralResultsCount,
                CoderResultsCount = CoderResultsCount,
                TesterResultsCount = TesterResultsCount,
                AutomatedResultsCount = AutomatedResultsCount
            };        
        }
        public IDictionary<string, int> GetCountsByGroup(IQueryable<Crash> Crashes)
        {
            IDictionary<string, int> Results = new Dictionary<string, int>();

            var counts =
                from c in Crashes
                group c by c.User.UserGroup into CrashesGrouped
                select new { Key = CrashesGrouped.Key, Count = CrashesGrouped.Count() };


            foreach (var c in counts)
            {
                Results.Add(c.Key, c.Count);
            }

            return Results;
        }

        public IDictionary<DateTime, int> GetWeeklyCountsByGroup(IQueryable<Crash> Crashes, string UserGroup)
        {
            IDictionary<DateTime, int> Results = new Dictionary<DateTime, int>();
            if (UserGroup == "All")
            {
                var counts =
                from c in Crashes
                group c by c.TimeOfCrash.Value.AddDays(-(int)c.TimeOfCrash.Value.DayOfWeek).Date into g
                orderby g.Key
                select new { Count = g.Count(), Date = g.Key };
                
                foreach (var c in counts)
                {
                    Results.Add(c.Date, c.Count);
                }
            }
            else
            {
                var counts =
                    from c in Crashes
                    where c.User.UserGroup == UserGroup
                    group c by c.TimeOfCrash.Value.AddDays(-(int)c.TimeOfCrash.Value.DayOfWeek).Date into g
                    orderby g.Key
                    select new { Count = g.Count(), Date = g.Key };

                foreach (var c in counts)
                {
                    Results.Add(c.Date, c.Count);
                }
            }


            return Results;
        }
        public IDictionary<DateTime, int> GetDailyCountsByGroup(IQueryable<Crash> Crashes, string UserGroup)
        {
            IDictionary<DateTime, int> Results = new Dictionary<DateTime, int>();
            if (UserGroup == "All")
            {
                var counts =
                from c in Crashes
                group c by c.TimeOfCrash.Value.Date into g
                orderby g.Key
                select new { Count = g.Count(), Date = g.Key };

                foreach (var c in counts)
                {
                    Results.Add(c.Date, c.Count);
                }
            }
            else
            {
                var counts =
                    from c in Crashes
                    where c.User.UserGroup == UserGroup
                    group c by c.TimeOfCrash.Value.Date into g
                    orderby g.Key
                    select new { Count = g.Count(), Date = g.Key };

                foreach (var c in counts)
                {
                    Results.Add(c.Date, c.Count);
                }
            }


            return Results;
        }

        public IQueryable<Crash> FilterByDate(IQueryable<Crash> Results, string DateFrom, string DateTo)
        {
            //Filter By Date

            var df = new DateTime();
            var dt = new DateTime();
     

                df = System.Convert.ToDateTime(DateFrom);
                dt = System.Convert.ToDateTime(df.AddDays(1));

                if (DateTo != null && DateTo != "")
                {
                    dt = System.Convert.ToDateTime(DateTo);
                    // add a day since we're using less than date@12:00 am. 
                    dt = dt.AddDays(1);
                }

            Results = Results.Where(c => c.TimeOfCrash >= df &&  c.TimeOfCrash <= dt);

            return Results;

        }
        public IQueryable<Crash> GetSortedResults(IQueryable<Crash> Results, string SortTerm, string SortOrder)
        {
            //TODO Talk to WesH about how to shrink this up a bit
            //Or figure out how to use reflection properly to make this work dynamically
            switch (SortTerm)
            {
                case "Id":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.Id);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.Id);
                    }
                    break;
                case "RawCallStack":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.RawCallStack);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.RawCallStack);
                    }
                    break;
                case "UserName":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.UserName);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.UserName);
                    }
                    break;
                case "ComputerName":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.ComputerName);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.ComputerName);
                    }
                    break;
                case "GameName":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.GameName);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.GameName);
                    }
                    break;
                case "EngineMode":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.EngineMode);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.EngineMode);
                    }
                    break;
                case "PlatformName":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.PlatformName);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.TimeOfCrash);
                    }
                    break;
                case "CommandLine":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.CommandLine);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.CommandLine);
                    }
                    break;
                case "TimeOfCrash":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.TimeOfCrash);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.TimeOfCrash);
                    }
                    break;
                case "ChangeListVersion":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.ChangeListVersion);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.ChangeListVersion);
                    }
                    break;
                case "FixedChangeList":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.FixedChangeList);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.FixedChangeList);
                    }
                    break;
                case "TTPID":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.TTPID);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.TTPID);
                    }
                    break;
                case "Status":
                    if (SortOrder == "Descending")
                    {
                        Results = Results.OrderByDescending(c => c.Status);
                    }
                    else
                    {
                        Results = Results.OrderBy(c => c.Status);
                    }
                    break;
            }
            return Results;

        }
        public CallStackContainer GetCallStack(Crash crash)
        {
            var CachedResults = new CachedDataService(HttpContext.Current.Cache, this);

            CallStackContainer CallStack = CachedResults.GetCallStack(crash);

            return CallStack;
            
        }
        public CallStackContainer CreateCallStackContainer(Crash crash)
        {
            
            CallStackContainer callStack = new CallStackContainer(crash.RawCallStack, 100, true, true);
            callStack.DisplayFunctionNames = true;
            callStack.DisplayFileNames = false;
            callStack.DisplayFilePathNames = false;
            callStack.DisplayUnformattedCallStack = false;

            return callStack;
        }

        public void BuildPattern(Crash crash)
        {
            string pattern = "";
            var CallStack = CreateCallStackContainer(crash);
            if(crash.Pattern == null || crash.Pattern == "3625")
            //if(crash.Id == 9)
            {
                var Entries = CallStack.GetEntries();
                try
                {
                    foreach (CallStackEntry e in Entries)
                    {
                        string name = e.FunctionName;
                        if (name.StartsWith("Address"))
                        {
                            name = "Address = * (Placeholder for Address Calls)";
                        }
                        FunctionCall fc = new FunctionCall();

                        if (mCrashDataContext.FunctionCalls.Where(f => f.Call == name).Count() > 0)
                        {
                            fc = mCrashDataContext.FunctionCalls.Where(f => f.Call == name).First();
                        }
                        else
                        {
                            fc = new FunctionCall();
                            fc.Call = name;
                            mCrashDataContext.FunctionCalls.InsertOnSubmit(fc);

                        }

                        var count = mCrashDataContext.Crash_FunctionCalls.Where(c => c.CrashId == crash.Id && c.FunctionCallId == fc.Id).Count();
                        if (count > 0)
                        {
                            //Don't do anything we're already in the join table.
                        }
                        else
                        {
                            var jointable = new Crash_FunctionCall();
                            jointable.Crash = crash;
                            jointable.FunctionCall = fc;
                            mCrashDataContext.Crash_FunctionCalls.InsertOnSubmit(jointable);
                                
                        }

                        mCrashDataContext.SubmitChanges();

                        if (pattern == "")
                        {
                            pattern = fc.Id.ToString();
                        }
                        else
                        {
                            pattern = pattern + "+" + fc.Id.ToString();
                        }
                        
                    }
                    crash.Pattern = pattern;
                    mCrashDataContext.SubmitChanges();
                }
                catch (Exception e)
                { 
                    //do nothing just don't save this pattern etc.
                    var changeSet = mCrashDataContext.GetChangeSet();
                    if (true) ;//provide a break point for debugging.
                }
            }
        }

        #endregion
    }
}