using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace CrashReport.Controllers
{
    [HandleError]
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            ViewData["Message"] = "Collaborative Crash Investigation";

            return View();
        }

        public ActionResult About()
        {
              
            return View();
            
        }
    }
}
