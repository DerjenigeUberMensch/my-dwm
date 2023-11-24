/*
 * THIS CONF READER IS NON PRODUCTION READY AND IS MEANT FOR DUBUGGING PURPOSES ONLY
 * DONOT USE IN PRODUCTION
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define BUFF_SIZE 1024
void 
logError(const char *text)
{
	FILE *file;
	char *dstdir;

	dstdir = "~/.config/dwm/log.txt";
	
	file = fopen(dstdir, "a");
	if(!file)
		return;
	fprintf("%s\n", text);
	fclose(file);
}
void 
parsefile(char *filename)
{
	FILE *file;
	char buff[BUFF_SIZE];
	int linenum;
	int failedparse;
	int failedconfset;
	char *varname;
	char *varvalue;
	char **strarr; /* arr[0] = varname, arr[1] = varvalue */
	char *failread;

	file = fopen(filename, "r");
	linenum = 0;
	failedparse = 0;
	failedconfset = 0;
	if(!file)
	{
		printf("%s Does not exist\n", filename);
		return;
	}


	char *pos;
	while( fgets( buff, BUFF_SIZE, file) != NULL)
	{
		++linenum;
		strarr = parseline(buff);
		failread = smprintf("WARN: Failed to read line at: %d", linenum);

		/* error checking */
		if(strarr == 0)
		{
			failedparse = 1;
			logError(failread);
			continue; /* strarr never created if read failed */
		}
		else if(strarr == 1) /* ignore */
			continue;

		failedconfset = setconf(strarr[0], strarr[1]);
		if(!failedconfset)
			logError(failread);
		free(strarr);
	}

	logError("Succes");
	if(failedparse)
		logError("Make sure format is correct: 'varname = vartype END'; yes even the spaces matter");
}

int 
checkconf(char *name, char *value)
{
	char *end;
	int BASE_SYSTEM = 10;
	long lvalue= strtol(value, &end, BASE_SYSTEM);
	if(end == lvalue || *end != '\0' || errno == ERANGE)
	{
		/*probably not a number */
		return 1;
	}
	return 2;
}
int 
setconf(char *name, char *value)
{
	/* 1 == string 2 == number */
	int i;
	int checktype;
	int varexists;
	int number;
	int string;
	checktype = checkconf(name, value);
	const char *varnames[] =
	{
		"borderpx", "snap", "windowrate", "hoverfocus", "barpadding", 
		"fastinputbar", "tabmodkey", "tabcyclekey","tabposx",
		"tabposy", "centertabtext", "maxwtab", "maxhtab",
		"minwidthdraw", "ayowtf"
	};
	if(checktype == 2)
	{
		number = atoi(value);
		for(i; i < LENGTH(varnames); ++i)
		{
			if(strcmp(name, varnames[i]) == 0)
				varexists = 1;
		}
		if(!varexists)
			return 0;
		/* why */
		if(strcmp(name, "borderpx") == 0)
		{
			borderpx = number;
		}
		if(strcmp(name, "snap") == 0)
		{
			snap = number;
		}
		if(strcmp(name, "windowrate") == 0)
		{
			windowrate = number;
		}
		if(strcmp(name, "hoverfocus")== 0)
		{
			hoverfocus = number;
		}
		if(strcmp(name, "barpadding") == 0)
		{
			barpadding = number;
		}
		if(strcmp(name, "showbar") == 0)
		{
			showbar = number;
		}
		if(strcmp(name, "tabmodkey") == 0)
		{
			tabModKey = number;
		}
		if(strcmp(name, "tabcyclekey") == 0)
		{
			tabCycleKey = number;
		}
		if(strcmp(name, "tabposx") == 0)
		{
			tabPosX = number;
		}
		if(strcmp(name, "tabposy") == 0)
		{
			tabPosY = number;
		}
		if(strcmp(name, "centertabtext") == 0)
		{
			centerTabText = number;
		}
		if(strcmp(name, "maxwtab") == 0)
		{
			maxWTab= number;
		}
		if(strcmp(name, "maxhtab") == 0)
		{
			maxHTab= number;
		}
		if(strcmp(name, "minwidthdraw") == 0)
		{
			minWidthDraw = number;
		}
		return 1;
	}
	return 0;

}
char** createArray(int size) { 
	char** strArray = (char**) malloc(size * sizeof(char*)); 
	for (int i = 0; i < size; i++) { 
		strArray[i] = (char*) malloc(BUFF_SIZE * sizeof(char)); 
	} 
	return strArray; 
} 
char** parseline(char *buffer)
{
	char *delimeterChar;
	char *previousToken[BUFF_SIZE];
	char *lastToken;
	char *varname[BUFF_SIZE];
	char *varvalue[BUFF_SIZE];
	char **strarr;
	int  pastequal;

	delimeterChar = " ";
	pastequal = 0;

	lastToken = strtok(buffer, delimeterChar);
	strcpy(previousToken, lastToken);

	while(lastToken != NULL)
	{
		if(pastequal)
		{
			strcpy(varvalue, lastToken);
			break;
		}
		if(strcmp(lastToken, "=") == 0)
		{
			strcpy(varname, previousToken);
			pastequal = 1;
		}

		strcpy(previousToken, lastToken);
		lastToken = strtok( NULL, delimeterChar);
	}
	if(lastToken == NULL || previousToken == NULL)
	{
		if(strcmp(buffer, "\n") == 0)
			return 1; /* ignore */
		return 0; /* failed */
	}
	strarr = createArray(2);
	strarr[0] = varname;
	strarr[1] = varvalue;
	return strarr;
}
void readconfig()
{
	char *filedir = "~/.config/dwm/config.conf";
	parsefile(filedir);
}

int main()
{
	readconfig();
}
