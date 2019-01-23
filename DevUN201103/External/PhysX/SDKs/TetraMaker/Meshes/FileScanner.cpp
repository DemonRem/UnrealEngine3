#include <string.h>
#include <float.h>
#include "FileScanner.h"

#define EOL 10

// ------------------------------------------------------------------------------------
FileScanner::FileScanner()
// ------------------------------------------------------------------------------------
{
	f = NULL; 
	lineNr = 0; 
	pos = line; 
	strcpy(errorMessage, "");
}

// ------------------------------------------------------------------------------------
FileScanner::~FileScanner()
// ------------------------------------------------------------------------------------
{
	if (f) 
		fclose(f);
}

// ------------------------------------------------------------------------------------
bool FileScanner::equalSymbol(char *sym, char *expectedSym, bool caseSensitive)
// ------------------------------------------------------------------------------------
{
	if (caseSensitive) 
		return strcmp(expectedSym, sym) == 0;
	else
#ifdef __BCPLUSPLUS__
		return strcmpi(expectedSym, sym) == 0;
#else
		return _stricmp(expectedSym, sym) == 0;
#endif
}


// ------------------------------------------------------------------------------------
bool FileScanner::open(char *filename)
// ------------------------------------------------------------------------------------
{
	f = fopen(filename, "r");
	if (!f) {
		strcpy(errorMessage, "file open error");
		return false;
	}
	fgets(line, maxLineLen, f);
	pos = line;
	lineNr = 1;
	return true;
}

// ------------------------------------------------------------------------------------
void FileScanner::close()
// ------------------------------------------------------------------------------------
{
	if (f) fclose(f);
	f = NULL;
}

// ------------------------------------------------------------------------------------
void FileScanner::getNextLine()
// ------------------------------------------------------------------------------------
{
	fgets(line, maxLineLen, f);
	pos = line;
	lineNr++;
}

bool singleCharSymbol(char ch)
{
	if (ch == '<' || ch == '>') return true;
	if (ch == '(' || ch == ')') return true;
	if (ch == '{' || ch == '}') return true;
	if (ch == '[' || ch == ']') return true;
	return false;
}

bool separatorChar(char ch)
{
	if (ch <= ' ') return true;
	if (ch == ',') return true;
	if (ch == ';') return true;
	return false;
}

// ------------------------------------------------------------------------------------
void FileScanner::getSymbol(char *sym)
// ------------------------------------------------------------------------------------
{
	while (!feof(f)) {
		while (!feof(f) && separatorChar(*pos)) {	// separators
			if (*pos == EOL) 
				getNextLine();
			else pos++;
		}
		if (!feof(f) && *pos == '/' && *(pos+1) == '/') 	// line comment
			getNextLine();
		else if (!feof(f) && *pos == '/' && *(pos+1) == '*') {	// long comment
			pos += 2;
			while (!feof(f) && (*pos != '*' || *(pos+1) != '/')) {
				if (*pos == EOL) 
					getNextLine();
				else pos++;
			}
			if (!feof(f)) pos += 2;
		}
		else break;
	}

	char *cp = sym;
	if (*pos == '\"' && !feof(f)) {		// read string 
		pos++;
		while (!feof(f) && *pos != '\"' && *pos != EOL) {
			*cp++ = *pos++;
		}
	    if (!feof(f) && *pos == '\"') pos++;	// override <"> 
	}
	else {					/* read other symbol */
		if (!feof(f)) {
			if (singleCharSymbol(*pos))		// single char symbol
				*cp++ = *pos++;
			else {							// multiple char symbol
				while (!separatorChar(*pos) && !singleCharSymbol(*pos) && !feof(f)) {
					*cp++ = *pos++;
				}
			}
		}
	}  
	*cp = 0;       
}


// ------------------------------------------------------------------------------------
bool FileScanner::checkSymbol(char *expectedSym, bool caseSensitive)
// ------------------------------------------------------------------------------------
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

	if (!equalSymbol(sym, expectedSym, caseSensitive)) {
		sprintf(errorMessage, "Parse error line %i, position %i: %s expected, %s read\n", 
			lineNr, (int)(pos - line), expectedSym, sym);
		return false;
	}
	return true;
}

/* ------------------------------------------------------------------- */
bool FileScanner::getIntSymbol(int &i)
/* ------------------------------------------------------------------- */
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

	int n = 0;
	for (int k = 0; k < (int)strlen(sym); k++) {
		if ((sym[k] >= '0') && (sym[k] <= '9')) n++;
		if ((sym[k] == '+') || (sym[k] == '-')) n++;
	}
	if (n < (int)strlen(sym)) { 
		setErrorMessage("integer expected");
		return false;
	}
	sscanf(sym,"%i",&i);
	return true;
}


/* ------------------------------------------------------------------- */
bool FileScanner::getFloatSymbol(float &r)
/* ------------------------------------------------------------------- */
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

#ifdef __BCPLUSPLUS__
	if (strcmpi(sym, "infinity") == 0) {
#else
	if (_stricmp(sym, "infinity") == 0) {
#endif
		r = FLT_MAX;
		return true;
	}
#ifdef __BCPLUSPLUS__
	if (strcmpi(sym, "-infinity") == 0) {
#else
	if (_stricmp(sym, "-infinity") == 0) {
#endif
		r = -FLT_MAX;
		return true;
	}

	int n = 0;
	for (int k = 0; k < (int)strlen(sym); k++) {
		if ((sym[k] >= '0') && (sym[k] <= '9')) n++;
		if ((sym[k] == '+') || (sym[k] == '-')) n++;
		if ((sym[k] == '.') || (sym[k] == 'e')) n++;
	}
	if (n < (int)strlen(sym)) {
		setErrorMessage("float expected");
		return false;
	}
	sscanf(sym,"%f",&r);
	return true;
}

/* ------------------------------------------------------------------- */
void FileScanner::setErrorMessage(char *msg)
/* ------------------------------------------------------------------- */
{
	sprintf(errorMessage, "parse error line %i, position %i: %s", 
			lineNr, (int)(pos - line), msg);
}

