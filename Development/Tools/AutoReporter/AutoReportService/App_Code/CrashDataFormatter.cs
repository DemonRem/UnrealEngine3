using System;
using System.Data;
using System.Configuration;
using System.Collections;
using System.Collections.Generic;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using System.Web.UI.HtmlControls;

namespace CrashDataFormattingRoutines
{
    public class FunctionDesc
    {
        public int begin;
        public int count;

        public FunctionDesc()
        {
            begin = -1;
            count = -1;
        }
    }

    public class CallStackDesc
    {
        public string fileName;
        public string filePath;
        public string functionName;

        private string GetFileNameAndPath(string CallStack, int startIndex, int count)
        {
            string fileNameAndPath = "filename not found";

            int FileBeginIndex = CallStack.IndexOf("[File", startIndex, count);
            int FileNotFoundIndex = CallStack.IndexOf("(filename not found)", startIndex, count);

            if (FileNotFoundIndex == -1 && FileBeginIndex >= 0)
            {
                int FileEndIndex = CallStack.IndexOf("]", FileBeginIndex + 1);
                if (FileEndIndex >= 0)
                {
                    fileNameAndPath = CallStack.Substring(FileBeginIndex + 6, FileEndIndex - FileBeginIndex - 6);
                }
            }

            return fileNameAndPath;
        }

        private FunctionDesc GetNextFunction(string CallStack, int startIndex)
        {
            FunctionDesc foundFunction = new FunctionDesc();

            int ParenthesisIndex = CallStack.IndexOf("()", startIndex);
            if (ParenthesisIndex >= 0)
            {
                int FunctionBeginIndex = ParenthesisIndex;
                while (FunctionBeginIndex > startIndex && 
                    !(CallStack[FunctionBeginIndex].Equals(' ') || CallStack[FunctionBeginIndex].Equals('\n')))
                {
                    FunctionBeginIndex--;
                }

                foundFunction.begin = FunctionBeginIndex;
                foundFunction.count = ParenthesisIndex + 2 - FunctionBeginIndex;
            }
            return foundFunction;
        }

        private string GetPath(string fileNameAndPath)
        {
            string PathName = "";
            int lastSlashIndex = fileNameAndPath.LastIndexOf("\\");
            if (lastSlashIndex != -1)
            {
                PathName = fileNameAndPath.Substring(0, lastSlashIndex + 1);
            }

            return PathName;
        }

        private string GetFileName(string fileNameAndPath)
        {
            string extractedFileName = "not found";
            int lastSlashIndex = fileNameAndPath.LastIndexOf("\\");
            if (lastSlashIndex != -1)
            {
                extractedFileName = fileNameAndPath.Substring(lastSlashIndex + 1);
            }

            return extractedFileName;
        }

        public int CreateFromCallStack(string CallStack, int startIndex)
        {

            functionName = "not specified";

            FunctionDesc foundFunction = GetNextFunction(CallStack, startIndex);
            if (foundFunction.begin >= 0)
            {
                functionName = CallStack.Substring(foundFunction.begin, foundFunction.count);

            }

            FunctionDesc nextFoundFunction = GetNextFunction(CallStack, foundFunction.begin + foundFunction.count);

            fileName = "not found";
            filePath = "";
            if (nextFoundFunction.begin != -1)
            {
                string fileNameAndPath = GetFileNameAndPath(CallStack, foundFunction.begin, nextFoundFunction.begin - foundFunction.begin);

                fileName = GetFileName(fileNameAndPath);
                filePath = GetPath(fileNameAndPath);
            }

            return nextFoundFunction.begin;
        }

        public int FindLineBegin(string CallStack, int lastLineEnd)
        {

            FunctionDesc foundFunction = GetNextFunction(CallStack, lastLineEnd);

            return foundFunction.begin;
        }

    }

    public class CallStackContainer
    {
        private string ErrorMsg;
        private string UnformattedCallStack;
        private ArrayList DescVector;

        public bool displayUnformattedCallStack;

        public bool displayFunctionNames;
        public bool displayFileNames;
        public bool displayFilePathNames;

        public CallStackContainer(string CallStack)
        {
            ParseCallStack(CallStack, 100);
        }

        public CallStackContainer(string CallStack, int FunctionParseCount)
        {
            ParseCallStack(CallStack, FunctionParseCount);
        }

        private void ParseCallStack(string CallStack, int FunctionParseCount)
        {
            displayUnformattedCallStack = false;
            displayFunctionNames = true;
            displayFileNames = false;
            displayFilePathNames = false;

            UnformattedCallStack = CallStack;

            DescVector = new ArrayList();

            CallStackDesc tempDesc = new CallStackDesc();
            int firstLineIndex = tempDesc.FindLineBegin(CallStack, 0);

            if (firstLineIndex <= 0)
            {
                ErrorMsg = CallStack;
                return;
            }
            else
            {
                ErrorMsg = CallStack.Substring(0, firstLineIndex);
            }

            
            int lastLineEndIndex = 0;
            int numFunctionsParsed = 0;
            while (lastLineEndIndex >= 0 && numFunctionsParsed < FunctionParseCount)
            {
                CallStackDesc newDesc = new CallStackDesc();
                lastLineEndIndex = newDesc.CreateFromCallStack(CallStack, lastLineEndIndex);
                if (lastLineEndIndex >= 0)
                {
                    numFunctionsParsed++;
                    DescVector.Add(newDesc);
                }
            }
           
        }

        public string GetFormattedCallStack()
        {
            if (displayUnformattedCallStack)
            {
                return UnformattedCallStack;
            }

            string formattedCallStack = ErrorMsg;
            formattedCallStack += "<br>";
            for (int i = 0; i < DescVector.Count; i++)
            {
                CallStackDesc currentDesc = ((CallStackDesc)DescVector[i]);
                if (displayFunctionNames)
                {
                    formattedCallStack += "<b><font color=\"#151B8D\">";
                    formattedCallStack += currentDesc.functionName;
                    formattedCallStack += "</font></b>";
                }

                if (displayFilePathNames)
                {
                    formattedCallStack += " --- ";
                    formattedCallStack += currentDesc.filePath;
                }

                if (displayFileNames)
                {
                    if (!displayFilePathNames)
                    {
                        formattedCallStack += " --- ";
                    }
                    formattedCallStack += "<b>";
                    formattedCallStack += currentDesc.fileName;
                    formattedCallStack += "</b>";
                }

                formattedCallStack += "<br>";
            }
            return formattedCallStack;
        }


    }
}
