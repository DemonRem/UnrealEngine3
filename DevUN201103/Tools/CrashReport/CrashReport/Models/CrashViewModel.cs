using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace CrashReport.Models
{
    public class CrashViewModel
    {
        public Crash Crash { get; set; }
        public CallStackContainer CallStack { get; set; }

    }
}