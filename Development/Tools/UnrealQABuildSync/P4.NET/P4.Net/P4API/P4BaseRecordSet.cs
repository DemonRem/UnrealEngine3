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
using System.Collections.Specialized;
using System.Text;

namespace P4API
{
    
    /// <summary>
    /// Base functionality for P4Recordset and P4UnParsedRecordset.
    /// </summary>
    /// <remarks>This is a base class for internal use.  Do not use directly.</remarks>
    /// <seealso cref="P4API.P4RecordSet"/>
    public abstract class P4BaseRecordSet
    {
        internal P4ResultClientUser ResultClientUser;
        internal ArrayList StringOutputs;
        internal ArrayList InfoOutputs;
        internal ArrayList ErrorOutputs;
        internal ArrayList WarningOutputs;
        internal ArrayList TaggedOutputs;
        private string _loginPassword;
        private string _inputData = String.Empty;

        private Hashtable FormInputDictionary;

        internal string _SpecDef;
        internal byte[] BinaryOutput;

        virtual internal string SpecDef
        {
            get
            {
                return _SpecDef;
            }
            set
            {
                _SpecDef = value;
            }
        }

        internal delegate void OnPromptEventHandler(object sender, P4PromptEventArgs e);
        internal event OnPromptEventHandler OnPrompt;

        internal P4BaseRecordSet()
        {
            StringOutputs = new ArrayList();
            InfoOutputs = new ArrayList();
            ErrorOutputs = new ArrayList();
            WarningOutputs = new ArrayList();
            TaggedOutputs = new ArrayList();
            this.ResultClientUser = new P4ResultClientUser(this);
        }

        internal void AddInfo(string S)
        {
            StringOutputs.Add(S);
        }
        internal void AddString(string S)
        {
            InfoOutputs.Add(S);
        }
        internal void AddError(string S)
        {
            ErrorOutputs.Add(S);
        }
        internal void AddWarning(string S)
        {
            WarningOutputs.Add(S);
        }
        virtual internal void AddTag(Hashtable S)
        {
            P4Record r = new P4Record(S);
            TaggedOutputs.Add(r);
        }
        /// <summary>
        /// Checks the Recordset to determine if errors occured.
        /// </summary>
        /// <returns>True if the p4 command returned errors.</returns>
        /// <remarks>This is the same as testing <c>Errors.Count</c>.</remarks>
        public bool HasErrors()
        {
            return (!(ErrorOutputs.Count == 0));
        }

        /// <summary>
        /// Checks the Recordset to determine if warnings occured.
        /// </summary>
        /// <remarks>This is the same as testing <c>Warnings.Count</c>.</remarks>
        /// <returns>True if the p4 command returned warnings.</returns>
        public bool HasWarnings()
        {
            return (!(WarningOutputs.Count == 0));
        }

        /// <summary>
        /// Gets an array of error messages returned from the Perforce command.
        /// </summary>
        /// <remarks>These are the error messages that are returned from Perforce.</remarks>
        public string[] Errors
        {
            get
            {
                return (string[])ErrorOutputs.ToArray(typeof(string));
            }
        }

        /// <summary>
        /// Gets an error messages returned from the Perforce command.
        /// </summary>
        /// <remarks>Errors are concatanated to one string, seperated by new lines.</remarks>
        public string ErrorMessage
        {
            get 
            {
                string ret = "";
                foreach (string s in Errors)
                {
                    ret += s + "\n";
                }
                return ret;
            }

        }

        /// <summary>
        /// Gets an array of warning messages returned from the Perforce command.
        /// </summary>
        /// <remarks>These are the warning messages that are returned from Perforce.</remarks>
        /// <note>Perforce is not always intuitive in how it defines warnings vs. messages.
        /// Always test various scenarios to determine whether the output is a warning or message.</note>
        /// <value>Array of warnings returned from Perforce.</value>
        public string[] Warnings
        {
            get
            {
                return (string[])WarningOutputs.ToArray(typeof(string));
            }
        }

        internal Hashtable FormInput
        {
            get
            {
                return FormInputDictionary;
            }
            set
            {
                FormInputDictionary = value;
            }
        }

        internal string InputData
        {
            get
            {
                return _inputData;
            }
            set
            {
                _inputData = value;
            }
        }

        internal string LoginPassword
        {
            get
            {
                return _loginPassword;
            }
            set
            {
                _loginPassword = value;
            }

        }

        internal string RaiseOnPromptEvent(string Message)
        {
            if (_loginPassword != null && Message == "Enter password: ")
            {
                //no need to raise an event...
                return _loginPassword;
            }


            // Make a temporary copy of the event to avoid possibility of
            // a race condition if the last subscriber unsubscribes
            // immediately after the null check and before the event is raised.
            OnPromptEventHandler handler = OnPrompt;

            // Event will be null if there are no subscribers
            if (handler != null)
            {
                P4PromptEventArgs e = new P4PromptEventArgs(Message);

                // Use the () operator to raise the event.
                handler(this, e);
                return e.Response;
            }
            else
            {
                return string.Empty;
            }
        }
    }
}
