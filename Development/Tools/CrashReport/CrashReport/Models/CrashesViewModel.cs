using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace CrashReport.Models
{
    public class CrashesViewModel
    {
        public string Query { get; set; }
        public IEnumerable<Crash> Results { get; set;}
        public PagingInfo PagingInfo { get; set;}
        public string Term { get; set; }
        public string Order { get; set; }
        public FormCollection FormCollection { get; set; }
        public IEnumerable<string> SetStatus { get { return new List<String>(new string[] { "Unset", "Reviewed", "New", "Coder", "Tester" }); } }
        public string UserGroup { get; set; }
        public string DateFrom { get; set; }
        public string DateTo { get; set; }
        public string GameName { get; set; }
        public IList<CallStackContainer> CallStacks { get; set; }
        public int AllResultsCount { get; set; }
        public int GeneralResultsCount { get; set; }
        public int CoderResultsCount { get; set; }
        public int TesterResultsCount { get; set; }
        public int AutomatedResultsCount { get; set; }
    }
}