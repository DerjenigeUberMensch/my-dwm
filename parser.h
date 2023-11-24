static char**
parseline(char *buffText);
static void parsefile(char *filename);
static int checkconf(char *name, char *value);
static int setconf(char *name, char *value);
static char **
createArray(int size);
static void readconfig();
static void logError(const char *text);
