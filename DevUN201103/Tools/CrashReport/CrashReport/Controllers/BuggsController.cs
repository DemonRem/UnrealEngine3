using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using CrashReport.Models;

namespace CrashReport.Controllers
{
    public class BuggsController : Controller
    {

        private IBuggRepository mBuggRepository;
        private ICrashRepository mCrashRepository;

        public BuggsController() : this( new BuggRepository() )
        { 
        }

        public  BuggsController(IBuggRepository Repository)
        {
            mBuggRepository = Repository;
            mCrashRepository = new CrashRepository(mBuggRepository.GetDataContext());
        }


        //
        // GET: /Buggs/

        public ActionResult Index(FormCollection form)
        {

            String SearchQuery = Request.QueryString["SearchQuery"];
            int Page = 1;
            bool parsePage = int.TryParse(Request.QueryString["Page"], out Page);
            if (parsePage && Page > 0)
            {
                // do nothing
            }
            else
            {
                Page = 1;
            }


            int PageSize = 100;
            bool parsePageSize = int.TryParse(Request.QueryString["PageSize"], out PageSize);
            if (parsePageSize && PageSize > 0)
            {
                // do nothing
            }
            else
            {
                PageSize = 100;
            }




            string SortTerm = "CrashesInTimeFrame";
            if (Request.QueryString["SortTerm"] == string.Empty || Request.QueryString["SortTerm"] == null)
            {
                // do nothing
            }
            else
            {
                SortTerm = Request.QueryString["SortTerm"];
            }
            string SortOrder = "Descending";
            if (Request.QueryString["SortOrder"] == string.Empty || Request.QueryString["SortOrder"] == null)
            {
                // do nothing
            }
            else
            {
                SortOrder = Request.QueryString["SortOrder"];
            }

            string PreviousOrder = Request.QueryString["PreviousOrder"];
            string PreviousTerm = Request.QueryString["PreviousTerm"];
            string UserGroup = "All";
            if (Request.QueryString["UserGroup"] == string.Empty || Request.QueryString["UserGroup"] == null)
            {
                // do nothing
            }
            else
            {
                UserGroup = Request.QueryString["UserGroup"];
            }



            string DateFrom = Request.QueryString["DateFrom"];
            string DateTo = Request.QueryString["DateTo"];
            string GameName = Request.QueryString["GameName"];
            bool OneQuery = false;
            if (Request.QueryString["OneQuery"] == "true")
            {
                OneQuery = true;
            }

            //Set the sort order 
            if (PreviousOrder == "Descending" && PreviousTerm == SortTerm)
            {
                SortOrder = "Ascending";
            }
            else if (PreviousOrder == "Ascending" && PreviousTerm == SortTerm)
            {
                SortOrder = "Descending";
            }
            else if (PreviousOrder == string.Empty || PreviousOrder == null)
            {
                //keep SortOrder Where it's at.
            }
            else
            {
                SortOrder = "Descending";
            }
           

            BuggsViewModel Results = mBuggRepository.GetResults(SearchQuery, Page, PageSize, SortTerm, SortOrder, PreviousOrder, UserGroup, DateFrom, DateTo, GameName, OneQuery);
            
            



            return View("Index", Results );
        }

        public ActionResult Show(FormCollection form, int id)
        {
            bool DisplayFunctionNames = true;
            if (form["DisplayFunctionNames"] == "false")
            {
                DisplayFunctionNames = false;
            }
           
            bool DisplayFileNames = true;
            if (form["DisplayFileNames"] == "false")
            {
                DisplayFileNames = false;
            }
            bool DisplayFilePathNames = false;
            if (form["DisplayFilePathNames"] == "true")
            {
                DisplayFilePathNames = true;
            }
            bool DisplayUnformattedCallStack = false;
            if (form["DisplayUnformattedCallStack"] == "true")
            {
                DisplayUnformattedCallStack = true;
            }


            BuggViewModel Model = new BuggViewModel();
            try
            {
                Model.Bugg = mBuggRepository.Get(id);
                Model.Crashes = Model.Bugg.GetCrashes();
            }catch (NullReferenceException)
            {
                return RedirectToAction("");
            }
            var bugg = new Bugg();

            bugg = mBuggRepository.Get(id);

                if (form["SetStatus"] != "") bugg.Status = form["SetStatus"];
                if (form["SetFixedIn"] != "") bugg.FixedChangeList = form["SetFixedIn"];
                if (form["SetTTP"] != "") bugg.TTPID = form["SetTTP"];
                if (form["Description"] != "") bugg.Description = form["Description"];
                mBuggRepository.SubmitChanges();

            Crash crash = Model.Crashes.FirstOrDefault();
            if (crash == null)
            { }
            else
            {
                var callStack = new CallStackContainer(crash.RawCallStack, 100, true, true);
                //set callstack properties
                callStack.DisplayFunctionNames = DisplayFunctionNames;
                callStack.DisplayFileNames = DisplayFileNames;
                callStack.DisplayFilePathNames = DisplayFilePathNames;
                callStack.DisplayUnformattedCallStack = DisplayUnformattedCallStack;

                Model.CallStack = callStack;

                crash.CallStackContainer = crash.GetCallStack();
            }

            return View("Show", Model);
        }


        public ActionResult Create(int id, FormCollection form)
        {   


            Crash Crash = mCrashRepository.Get(id);
            
            string Pattern = Crash.Pattern;

            BuggViewModel Model = new BuggViewModel();
            Model.CrashId = id;
            Model.Bugg = new Bugg();
            Model.Bugg.Pattern = Pattern;
            var Crashes = mCrashRepository.FindByPattern(Pattern);
            if (form["FormPattern"] == "" || form["FormPattern"] == null)
            {
                //keep the above pattern
                Model.Pattern = Pattern;
            }
            else
            {
                Model.Pattern = form["FormPattern"];
                Crashes = mCrashRepository.FindByPattern(form["FormPattern"]);

            }
            

            Model.Crashes = Crashes;
            foreach (Crash c in Model.Crashes)
            {
                c.CallStackContainer = c.GetCallStack();
            }

            return View("Create", Model);
        }
        public ActionResult Save(FormCollection form)
        {
            if (form == null)
            {
                return RedirectToAction("Index");
            }
            else
            {

                var Pattern = form["FormPattern"];
                var ExistingBugg = mBuggRepository.GetByPattern(Pattern);
                if (ExistingBugg == null && Pattern != null)
                {
                    IQueryable<Crash> Crashes = mBuggRepository.GetCrashesByPattern(Pattern);

                    Bugg bugg = new Bugg();
                    bugg.Pattern = Pattern;


                    mBuggRepository.AddCrashes(bugg, Crashes);

                    mBuggRepository.Add(bugg);
                    mBuggRepository.SubmitChanges();

                    var bId = bugg.Id;
                    return RedirectToAction("Show", new { id = bId });
                }
                else if (form["FormCrashId"] == null)
                {
                    return RedirectToAction("Index");
                }else
                {
                   
                    return RedirectToAction("Create", new { id = form["FormCrashId"] });
                }
            }
            
        }
    }
}
