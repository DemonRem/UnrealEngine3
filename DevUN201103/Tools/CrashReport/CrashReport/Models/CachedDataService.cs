using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Caching;
using CrashReport.Models;

namespace CrashReport.Models
{
    public class CachedDataService
    {
        private ICrashRepository _repository;
        private Cache cache;
        private const string cacheKeyPrefix = "__CachedDataService";
        private const string callstackKeyPrefix = "_CallStack_";
        public CachedDataService(Cache cache)
        {
            this.cache = cache;
            _repository = new CrashRepository();
        }
        public CachedDataService(Cache cache, CrashRepository repository)
        {
            this.cache = cache;
            _repository = repository;
        }


        public CallStackContainer GetCallStack(Crash Crash)
        {
            string key = cacheKeyPrefix + callstackKeyPrefix + Crash.Id;
            CallStackContainer CallStack = (CallStackContainer)cache[key];
            if (CallStack == null)
            {
                CallStack  = this._repository.CreateCallStackContainer(Crash);
                cache.Insert(key, CallStack);
            }

            return CallStack;
        }
        public IList<Crash> GetCrashes(string DateFrom, string DateTo)
        {
            string key = cacheKeyPrefix + DateFrom + DateTo;
            IList<Crash> Data = (IList<Crash>)cache[key];
            if (Data == null)
            {
                IQueryable<Crash> DataQuery = _repository.ListAll();
                DataQuery = _repository.FilterByDate(DataQuery, DateFrom, DateTo);
                Data = DataQuery.ToList();
                cache.Insert(key, Data);

            }
            return Data;
        }

    }
}