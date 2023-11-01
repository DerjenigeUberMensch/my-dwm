#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <event.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include <X11/Xlib.h>
//Note usage of restrict and char are only to decrease mem usage slighty ~.1MB - .6MB
//you can remove these if you deem it necessary or want more freedom
char *smprintf(char *fmt, ...)
{
    va_list fmtargs;
    char *ret;
    int len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    ret = malloc(++len);
    if (ret == NULL) {
        perror("Error: Memory allocation failed in smprintf");
        return -1;
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);
    return ret;
}

void settz(char *tzname)
{
    setenv("TZ", tzname, 1);
}

char *mk_times(char *fmt, char *restrict tzname) 
{
    char buf[129];
    time_t tim;
    struct tm *timtm;

    settz(tzname);
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL)
        return smprintf("");

    if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        return smprintf("");
    }

    return smprintf("%s", buf);
}

void set_status(char *str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char *load_avg(void)
{
    double avgs[3];

    if (getloadavg(avgs, 3) < 0)
        return smprintf("");

    return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *readfile(char *base, char *file)
{
    char *path, line[513];
    FILE *fd;

    memset(line, 0, sizeof(line));

    path = smprintf("%s/%s", base, file);
    fd = fopen(path, "r");
    free(path);
    if (fd == NULL)
        return NULL;

    if (fgets(line, sizeof(line)-1, fd) == NULL) {
        fclose(fd);
        return NULL;
    }
    fclose(fd);

    return smprintf("%s", line); 
}

char *get_battery(char *base)//can be greatly shorten but becomes unreadable
{
    char *co;
    char status;

    int descap;
    int remcap;

    descap = -1;
    remcap = -1;

    co = readfile(base, "present");
    if (co == NULL)
        return smprintf("0xNULL");
    else if(co[0] != '1')
    {
        free(co);
        return smprintf("not present");
    }

    co = readfile(base, "charge_full_design");
    if (co == NULL) {
        co = readfile(base, "energy_full_design");
        if (co == NULL)
            return smprintf("0xNULL");
    }
    sscanf(co, "%d", &descap);

    co = readfile(base, "charge_now");
    if (co == NULL) {
        co = readfile(base, "energy_now");
        if (co == NULL)
            return smprintf("");
    }
    sscanf(co, "%d", &remcap);

    co = readfile(base, "status");
    if (!strncmp(co, "Discharging", 11)) {
        status = '-';
    } else if(!strncmp(co, "Charging", 8)) {
        status = '+';
    } else {
        status = '.'; //status = '?';
    }
    free(co);
    if (remcap < 0 || descap < 0)
        return smprintf("invalid");

    float batteryPercentage = (float)remcap / (float)descap * 100;

    if(batteryPercentage >= 99) {
        return smprintf("%s", "AC"); //asume AC when bat>99
    }
    return smprintf("%.0f%%%c", batteryPercentage, status); //returns percentage charged as x%
}

char *get_temperature(char *base, char *sensor)
{
    char *co;

    co = readfile(base, sensor);
    if (co == NULL) {
        return smprintf("");
    }
    float temperature = atof(co)/1000;
    free(co); //Also deja vu
    //you need to free(co) or you get a memory leak, although very very small (you wuold have to run this for days to become serious) => you dont want things like this stacking up
    return smprintf("%02.0f°C", temperature );
}

char *exec_script(char *cmd)
{
    FILE *fp;
    char retval[1025], *rv;

    memset(retval, 0, sizeof(retval));

    fp = popen(cmd, "r");
    if (fp == NULL)
        return smprintf("");

    rv = fgets(retval, sizeof(retval), fp);
    pclose(fp);
    if (rv == NULL)
        return smprintf("");
    retval[strlen(retval)-1] = '\0';

    return smprintf("%s", retval);
}
void *dwm_status_bar()
{
    //STANDALONE ~2.4MB compiled with dwm ~1.8MB

    //modules
    char *status;
    char *avgs;
    char *battery;
    char *temp0;
    char *temp1;
    char *timezone;
    //Presets
    char *timezoneSet = "America/Chicago";  //you can use stuff like utc but doesnt work sometimes
    const char sleepTime = 1; //IN SECONDS //change to float if you want more freedom
    for(;;sleep(sleepTime))
    {
        temp0   = get_temperature("/sys/devices/virtual/thermal/thermal_zone3", "temp"); //Might want to change thermal_zone
        temp1   = get_temperature("/sys/devices/virtual/thermal/thermal_zone4", "temp"); //on different motherboard configs

        avgs    = load_avg();
        battery = get_battery("/sys/class/power_supply/BAT0");
        //%a dayName %d daynum %b %H hour %M minute monthName %Z timezoneOffset %Y year
        timezone= mk_times("%a %d %b %H:%M:%S %Z %Y", timezoneSet);

        /*format ("someString %typedef %orJustTypeDef", args)
         * use %s if unsure (unless you program some func to return an int or something else use %s if using default funcs)
         * *number of %typedef must equal to amount of args
         */
        status = smprintf("T:%s|%s L:%s %s %s",
                          temp0, temp1, avgs, battery, timezone);
        set_status(status);
        free(temp1);
        free(temp0);
        free(avgs);
        free(battery);
        free(timezone);
        free(status);
    }
}
static void start_status_bar(pthread_t threadId)
{
    pthread_create(&threadId, NULL, dwm_status_bar, NULL);

}
