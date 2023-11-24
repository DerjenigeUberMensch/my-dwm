/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void
die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fputc('\n', stderr);
    }

    exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
    void *p;

    if (!(p = calloc(nmemb, size)))
        die("calloc:");
    return p;
}
char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if(!ret)
		return "Failed to malloc memory at smprintf 'ret'";
	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);
	return ret;
}
void 
syslog(char *text)
{
    const char *filename = "~/.config/dwm/dwm.log";
    FILE *file = fopen(filename, "a");
    if (!file)
    {
        fprintf(stderr, "Error: Unable to open file %s for appending creating file...\n", filename);
        file = fopen(filename, "w");
        fclose(file);
        file = fopen(filename, "a");
    }
    fprintf(file, "%s\n", text);  // Append text with newline
    fclose(file);

}

