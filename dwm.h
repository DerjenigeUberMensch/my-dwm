#ifndef DWM_H_
#define DWM_H_

#include <stdint.h>

#include "pool.h"
#include "drw.h"
#include "winutil.h"
/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
        * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + ((X)->bw << 1))
#define HEIGHT(X)               ((X)->h + ((X)->bw << 1))
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TAGSLENGTH              (LENGTH(tags))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define OPAQUE                  0xffU
#define SESSION_FILE            "/tmp/dwm-session"
#define BROKEN                  "borked."
/* motif window decoration */
#define MWM_HINTS_FLAGS_FIELD       0
#define MWM_HINTS_DECORATIONS_FIELD 2
#define MWM_HINTS_DECORATIONS       (1 << 1)
#define MWM_DECOR_ALL               (1 << 0)
#define MWM_DECOR_BORDER            (1 << 1)
#define MWM_DECOR_TITLE             (1 << 3)
/* enums */
/* cursor */
enum CurImg
{
    CurNormal,
    CurResizeTopLeft, CurResizeTopRight, CurResizeBottomLeft, CurResizeBottomRight,
    CurMove,
    CurLast
};
/* color schemes */
enum SchemeType
{
    SchemeNorm, SchemeSel,                      /* default */
    SchemeUrgent, SchemeWarn,                   /* signals */
    SchemeAltTab, SchemeAltTabSelect,           /* alt tab */
    SchemeBarTabActive, SchemeBarTabInactive,   /* bar tab */
    SchemeTagActive,                            /*  tags   */
};

/* clicks */
enum ClkType
{
    ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
    ClkClientWin, ClkRootWin, ClkLast,
};

/* layouts */
enum LayoutType
{
    Tiled, Floating, Monocle, Grid
};

/* kill client */
enum KillType
{
    Graceful, Safedestroy, Destroy,
};

extern enum CurImg CurgImg;
extern enum SchemeType SchemeType;
extern enum ClkType ClkType;
extern enum LayoutType LayoutType;
extern enum KillType KillType;

typedef union  Arg Arg;
typedef struct Key Key;
typedef struct Button Button;
typedef struct Monitor Monitor;
typedef struct Tag Tag;
typedef struct Client Client;
typedef struct Layout Layout;
typedef struct Rule Rule;

union Arg
{
    int i;
    unsigned int ui;
    float f;
    const void *v;
};
struct Key
{
    int type;
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
};
struct Button
{
    unsigned int click;
    unsigned int mask;
    unsigned int button;
    void (*func)(const Arg *arg);
    const Arg arg;
};

struct Client
{
    char name[256];
    float mina, maxa;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int bw, oldbw; /* border width */

    unsigned int tags;
    unsigned int alwaysontop;
    unsigned int stayontop;
    unsigned int hintsvalid;
    unsigned int wasfloating;
    unsigned int isfixed;
    unsigned int isfloating;
    unsigned int isurgent;
    unsigned int neverfocus;
    unsigned int isfullscreen;
    unsigned int num;
    pid_t pid;
    /* icon */
    unsigned int icw;
    unsigned int ich;
    Picture icon;   /* ulong */
    Client *prev, *next;
    Client *sprev, *snext;
    Monitor *mon;
    Window win; /* ulong */
};

struct Layout
{
    const char *symbol;
    void (*arrange)(Monitor *);
};

struct Monitor
{
    char ltsymbol[16];
    float mfact;
    int nmaster;
    int num;
    int by;               /* bar geometry */
    int mx, my, mw, mh;   /* screen size  */
    int wx, wy, ww, wh;   /* window area  */
    unsigned int isfullscreen;     /* toggle fullscr vs reg fullscr */
    unsigned int showbar;
    unsigned int oshowbar;
    unsigned int topbar;

    unsigned int lyt;   /*    layout    */
    unsigned int olyt;  /*  old layout  */
    unsigned int seltags;
    unsigned int sellt;
    unsigned int tagset[2];
    unsigned int cc; /* client counter */
    Client *clients, *clast;
    Client *stack, *slast;
    Client *sel;
    Client *tabnext;    /* alt tab next client */
    Monitor *next;
    Pixmap *tagmap;
    Window barwin;      /* ulong */
    Window tabwin;      /* ulong */
    Window tagwin;
    const Layout *lt[2];
};

struct Rule
{
    const char *class;
    const char *instance;
    const char *title;
    unsigned int tags;
    int isfloating;
    int monitor;
};

/* function declarations */
extern Client *alttab(int ended);
extern void alttabend(Client *tabnext);
extern void applyrules(Client *c);
extern int  applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
extern void arrange(Monitor *m);
extern void arrangeall(void);
extern void arrangemon(Monitor *m);
extern void attach(Client *c);
extern void attachstack(Client *c);
extern void buttonpress(XEvent *e);
extern void buttonrelease(XEvent *e);
extern void checkotherwm(void);
extern void circulaterequest(XEvent *e);
extern void circulatenotify(XEvent *e);
extern void cleanup(void);
extern void cleanupmon(Monitor *mon);
extern void cleanupsbar(Monitor *m);
extern void cleanuptabwin(Monitor *m);
extern void clientmessage(XEvent *e);
extern void configure(Client *c);
extern void configurenotify(XEvent *e);
extern void configurerequest(XEvent *e);
extern void createnotify(XEvent *e);
extern Monitor *createmon(void);
extern void destroynotify(XEvent *e);
extern void detach(Client *c);
extern void detachstack(Client *c);
extern Monitor *dirtomon(int dir);
extern int docked(Client *c);
extern int dockedvert(Client *c);
extern int dockedhorz(Client *c);
extern void drawalttab(int first, Monitor *m);
extern void drawbar(Monitor *m);
extern int  drawbarname(Monitor *m);
extern void drawbars(void);
extern int  drawbarstatus(Monitor *m, int x);
extern int  drawbarsym(Monitor *m, int x);
extern int  drawbartabs(Monitor *m, int x, int maxw);
extern int  drawbartags(Monitor *m, int x);
extern void enternotify(XEvent *e);
extern void expose(XEvent *e);
extern void focus(Client *c);
extern void focusin(XEvent *e);
extern void focusout(XEvent *e);
extern void freeicon(Client *c);
extern Atom getatomprop(Client *c, Atom prop);
extern const Layout *getmonlyt(Monitor *m);
extern pid_t getwinpid(Window window);
extern Picture geticonprop(Window win, unsigned int *picw, unsigned int *pich);
extern int  getrootptr(int *x, int *y);
extern long getstate(Window w);
extern int  gettextprop(Window w, Atom atom, char *text, unsigned int size);
extern void grabbuttons(Client *c, int focused);
extern void grabkeys(void);
extern void graphicsexpose(XEvent *e);
extern void grid(Monitor *m);
extern void keypress(XEvent *e);
extern void keyrelease(XEvent *e);
extern void keymapnotify(XEvent *e);
extern void killclient(Client *c, int type);
extern void leavenotify(XEvent *e);
extern void manage(Window w, XWindowAttributes *wa);
extern void mappingnotify(XEvent *e);
extern void maprequest(XEvent *e);
extern void maximize(Client *c);
extern void maximizevert(Client *c);
extern void maximizehorz(Client *c);
extern void monocle(Monitor *m);
extern void motionnotify(XEvent *e);
extern Client *nexttiled(Client *c);
extern Client *nextvisible(Client *c);
extern void noexpose(XEvent *e);
extern uint32_t prealpha(uint32_t p);
extern void pop(Client *c);
extern void propertynotify(XEvent *e);
extern void quit(void);
extern void restoresession(void);
extern void restoremonsession(Monitor *m);
extern Monitor *recttomon(int x, int y, int w, int h);
extern void resize(Client *c, int x, int y, int w, int h, int interact);
extern void resizeclient(Client *c, int x, int y, int w, int h);
extern void resizerequest(XEvent *e);
extern void restack(Monitor *m);
extern void restart(void);
extern void run(void);
extern void savesession(void);
extern void savemonsession(Monitor *m);
extern void scan(void);
extern int  sendevent(Client *c, Atom proto);
extern void sendmon(Client *c, Monitor *m);
extern void setclientstate(Client *c, long state);
extern void setdesktop(void);
extern void setdesktopnames(void);
extern void setdesktopnum(void);
extern void setfloating(Client *c, int isfloating);
extern void setfocus(Client *c);
extern void setfullscreen(Client *c, int fullscreen);
extern void setmonlyt(Monitor *m, int layout);
extern void setshowbar(Monitor *m, int show);
extern void setsticky(Client *c, int sticky);
extern void setup(void);
extern void setupatom(void);
extern void setupcur(void);
extern void setuppool(void);
extern void setuptags(void);
extern void setuptimezone(void);
extern void seturgent(Client *c, int urg);
extern void setviewport(void);
extern void showhide(Client *c);
extern void showhidemap(Client *c);
extern void sigchld(int signo);
extern void sighandler(void);
extern void sighup(int signo); 
extern void sigterm(int signo); 
extern void tile(Monitor *m);
extern void unfocus(Client *c, int setfocus);
extern void unmanage(Client *c, int destroyed);
extern void unmapnotify(XEvent *e);
extern void updatebarpos(Monitor *m);
extern void updatebars(void);
extern void updateclientlist(void);
extern void updatedesktop(void);
extern int  updategeom(void);
extern void updatemotifhints(Client *c);
extern void updatenumlockmask(void);
extern void updatesizehints(Client *c);
extern void updatestatus(void);
extern void updatetitle(Client *c);
extern void updateicon(Client *c);
extern void updatewindowstate(Client *c, Atom state, int data);
extern void updatewindowtype(Client *c);
extern void updatewmhints(Client *c);
extern void visiblitynotify(XEvent *e);
extern void winsetstate(Window win, long state);
extern void winmap(Client *c, int deiconify);
extern void winunmap(Window win, Window winroot, int iconify);
extern Client  *wintoclient(Window w);
extern Monitor *wintomon(Window w);

/* yes these shadow dpy; no dont change them */
extern int  xerror(Display *dpy, XErrorEvent *ee);
extern int  xerrordummy(Display *dpy, XErrorEvent *ee);
extern int  xerrorstart(Display *dpy, XErrorEvent *ee);

/* variables */
extern char stext[256];     /* status WM_NAME text */
extern int screen;
extern int sw, sh;          /* X display screen geometry width, height */
extern int bh;              /* bar height */
extern int lrpad;           /* sum of left and right padding for text */
extern int (*xerrorxlib)(Display *, XErrorEvent *);
extern unsigned int numlockmask;
extern Atom wmatom[WMLast], motifatom;
extern Atom netatom[NetLast];
extern Cur *cursor[CurLast];
extern Clr **scheme;
extern Clr **tagscheme;
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *selmon;
extern Window root, wmcheckwin;
/* ACC */
extern unsigned int accnum; /* Active client counter Number */


/* dwm stuff */
extern int running;
extern int RESTART;
extern Client *lastfocused;
extern Pool *pl;

#endif
