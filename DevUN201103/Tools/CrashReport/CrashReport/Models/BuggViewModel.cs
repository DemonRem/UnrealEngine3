using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace CrashReport.Models
{
    public class BuggViewModel
    {
        public Bugg Bugg { get; set; }
        public IEnumerable<Crash> Crashes { get; set; }
        public CallStackContainer CallStack { get; set; }
        public String Pattern { get; set; }
        public int CrashId { get; set; }
    }
}