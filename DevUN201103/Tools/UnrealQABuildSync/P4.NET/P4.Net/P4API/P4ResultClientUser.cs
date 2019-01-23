/*
 * P4.Net *
Copyright (c) 2007 Shawn Hladky

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


using System;
using System.Collections;
using System.Text;

namespace P4API
{
    internal class P4ResultClientUser : p4dn.ClientUser 
    {
        private P4BaseRecordSet _P4Result;
        private bool _executeMergeTool = false;

        // throwing an exception in one of the overriden methods causes all kinds of problems.
        // this variable stores an exception, which can be used later to throw an exception when we're 
        // all done communicating with Perforce
        public Exception DeferedException = null;

        public bool ExecuteMergeTool
        {
            get
            {
                return _executeMergeTool;
            }
            set
            {
                _executeMergeTool = value;
            }
        }

        public P4ResultClientUser(P4BaseRecordSet p4Result)
        {
            this._P4Result = p4Result;
        }

        public override void Diff(System.IO.FileInfo f1, System.IO.FileInfo f2, int doPage, string diffFlags, p4dn.Error err)
        {
            // don't give the consumer any more messages if we're in an error state
            if (DeferedException != null) return;

            DeferedException = new Exceptions.DiffNotImplemented();
            
        }

        public override void Edit(System.IO.FileInfo f1, p4dn.Error err)
        {
            // don't give the consumer any more messages if we're in an error state
            if (DeferedException != null) return;

            // this only happens when the user is fetching a form w/o using FetchForm method.
            DeferedException = new P4API.Exceptions.FormCommandException();
        }

        public override void ErrorPause(string errBuf, p4dn.Error err)
        {
            if (DeferedException != null) return;

            // don't know how this would be called.  AFIK, the only way to get here is to fill out a form
            // incorrectly, and other code should deal with that.
            DeferedException = new P4API.Exceptions.ErrorPauseCalled();
        }

        public override void Finished()
        {
            //base.Finished();
        }

        public override void Prompt(string msg, ref string rsp, bool noEcho, p4dn.Error err)
        {
            // don't give the consumer any more messages if we're in an error state
            if (DeferedException != null) return;

            rsp = _P4Result.RaiseOnPromptEvent(msg);
        }

        public override void Merge(System.IO.FileInfo @base, System.IO.FileInfo leg1, System.IO.FileInfo leg2, System.IO.FileInfo result, p4dn.Error err)
        {
            // don't give the consumer any more messages if we're in an error state
            if (DeferedException != null) return;

            DeferedException = new Exceptions.MergeNotImplemented(); ;
        }

        public override void Message(p4dn.Error err)
        {
            switch (err.Severity)
            {
                case p4dn.Error.ErrorSeverity.Empty : // E_EMPTY (0) | no error 
                    // Is this ever even a legit severity???  I've never hit it in my testing.
                    _P4Result.AddString(err.Fmt());
                    break;
                case p4dn.Error.ErrorSeverity.Info : // E_INFO  (1) | information, not necessarily an error 
                    _P4Result.AddInfo(err.Fmt());
                    break;
                case p4dn.Error.ErrorSeverity.Warning : // E_WARN  (2) | a minor error occurred 
                    _P4Result.AddWarning(err.Fmt());
                    break;
                case p4dn.Error.ErrorSeverity.Failed : // E_FAILED(3) | the command was used incorrectly 
                    _P4Result.AddError(err.Fmt());
                    break;
                case p4dn.Error.ErrorSeverity.Fatal : // E_FATAL (4) | fatal error, the command can't be processed 
                    _P4Result.AddError(err.Fmt());
                    break;
                default:
                    //TODO throw an error... unknown severity
                    break;
            }
        }
        public override void HandleError(p4dn.Error err)
        {
            _P4Result.AddError(err.Fmt());
        }
        public override void OutputBinary(byte[] b)
        {
            _P4Result.BinaryOutput = b;
        }
        public override void OutputStat(Hashtable varList)
        {
            _P4Result.AddTag(varList);
        }
        public override void OutputText(string data)
        {
            _P4Result.AddInfo(data);
        }
        public override void SetSpecDef(string specdef)
        {
            _P4Result.SpecDef = specdef;
        }
        public override void InputForm(ref Hashtable varList, ref string specdef, p4dn.Error err)
        {
            specdef = _P4Result.SpecDef;
            varList = _P4Result.FormInput;
        }

        public override void OutputError(string errString)
        {
            _P4Result.AddError(errString);
        }

        public override void Help(string help)
        {
            _P4Result.AddInfo(help);
        }

        public override void  OutputInfo(char level, string data)
        {
            _P4Result.AddInfo(data);
        }

        public override void InputData(ref string buff, p4dn.Error err)
        {
            buff = _P4Result.InputData;
        }

    }
}
