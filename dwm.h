#ifndef DWM_H_
#define DWM_H_
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
enum
{
    CurNormal,
    CurResizeTopLeft, CurResizeTopRight, CurResizeBottomLeft, CurResizeBottomRight,
    CurMove,
    CurLast
};

/* color schemes */
enum
{
    SchemeNorm, SchemeSel,                      /* default */
    SchemeUrgent, SchemeWarn,                   /* signals */
    SchemeAltTab, SchemeAltTabSelect,           /* alt tab */
    SchemeBarTabActive, SchemeBarTabInactive,   /* bar tab */
    SchemeTagActive,                            /*  tags   */
};

/* EWMH atoms */
/* when adding new properties make sure NetLast is indeed last to allocate enough memory to store all Nets in an array */
enum
{
    NetSupported, NetWMName, NetWMIcon, NetWMState, NetWMCheck,
    NetWMFullscreen, NetWMAlwaysOnTop,NetActiveWindow,
    NetWMWindowType, NetWMWindowTypeDialog, NetClientList,
    NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops, NetCurrentDesktop, /* EMWH */
    NetWMWindowsOpacity, /* unset */
    NetLast,
};

/* default atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast, };

/* clicks */
enum
{
    ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
    ClkClientWin, ClkRootWin, ClkLast,
};
/* stack shifting */
enum
{
    BEFORE, PREVSEL, NEXT,
    FIRST, SECOND, THIRD, LAST,
};
/* layouts */
enum
{
    TILED, FLOATING, MONOCLE, GRID,
};

/* kill client */
enum
{
    GRACEFUL, SAFEDESTROY, DESTROY,
};

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

struct Tag
{
    int num;

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
    unsigned int alwaysontop     : 1;
    unsigned int hintsvalid      : 1;
    unsigned int ismax           : 1;
    unsigned int wasfloating     : 1;
    unsigned int isfixed         : 1;
    unsigned int isfloating      : 1;
    unsigned int isurgent        : 1;
    unsigned int neverfocus      : 1;
    unsigned int isfullscreen    : 1;
    unsigned int num;
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
    int altTabN;		  /* move that many clients forward */
    int nTabs;			  /* number of active clients in tag */
    unsigned int isfullscreen   : 1;     /* toggle fullscr vs reg fullscr */
    unsigned int showbar        : 1;
    unsigned int oshowbar       : 1;
    unsigned int topbar         : 1;
    unsigned int showpreview    : 1;

    unsigned int lyt;   /*    layout    */
    unsigned int olyt;  /*  old layout  */
    unsigned int seltags;
    unsigned int sellt;
    unsigned int tagset[2];
    unsigned int cc; /* client counter */
    Client *clients, *clast;
    Client *stack, *slast;
    Client *sel;
    Client **altsnext; /* array of all clients in the tag */
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
void alttab(void);
void alttabend(void);
void applyrules(Client *c);
int  applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
void arrange(Monitor *m);
void arrangemon(Monitor *m);
void attach(Client *c);
void attachstack(Client *c);
void buttonpress(XEvent *e);
void checkotherwm(void);
void cleanup(void);
void cleanupmon(Monitor *mon);
void cleanupsbar(Monitor *m);
void cleanuptagpreview(Monitor *m);
int  clientdocked(Client *c);
void clientmessage(XEvent *e);
void configure(Client *c);
void configurenotify(XEvent *e);
void configurerequest(XEvent *e);
Monitor *createmon(void);
void destroynotify(XEvent *e);
void detach(Client *c);
void detachstack(Client *c);
Monitor *dirtomon(int dir);
void drawalttab(int nwins, int first, Monitor *m);
void drawbar(Monitor *m);
void drawbars(void);
void drawbartabs(Monitor *m, int x, int y, int maxw, int height);
void enternotify(XEvent *e);
void expose(XEvent *e);
void focus(Client *c);
void focusin(XEvent *e);
void focusstack(int SHIFT_TYPE);
void freeicon(Client *c);
Atom getatomprop(Client *c, Atom prop);
int  getwinpid(Window window);
uint32_t prealpha(uint32_t p);
Picture geticonprop(Window win, unsigned int *picw, unsigned int *pich);
int  getrootptr(int *x, int *y);
long getstate(Window w);
int  gettextprop(Window w, Atom atom, char *text, unsigned int size);
void grabbuttons(Client *c, int focused);
void grabkeys(void);
void grid(Monitor *m);
void keypress(XEvent *e);
void keyrelease(XEvent *e);
void killclient(Client *c, int type);
void manage(Window w, XWindowAttributes *wa);
void mappingnotify(XEvent *e);
void maprequest(XEvent *e);
void maximize(int x, int y, int w, int h);
void monocle(Monitor *m);
void motionnotify(XEvent *e);
void movstack(Client *c, int SHIFT_TYPE);
Client *nexttiled(Client *c);
void pop(Client *c);
void propertynotify(XEvent *e);
void quit(void);
void restoresession(void);
Monitor *recttomon(int x, int y, int w, int h);
void resize(Client *c, int x, int y, int w, int h, int interact);
void resizeclient(Client *c, int x, int y, int w, int h);
void resizerequest(XEvent *e);
void restack(Monitor *m);
void restart(void);
void run(void);
void savesession(void);
void scan(void);
int  sendevent(Client *c, Atom proto);
void sendmon(Client *c, Monitor *m);
void setclientstate(Client *c, long state);
void setclientlayout(Monitor *m, int layout);
void setdesktop(void);
void setdesktopnames(void);
void setdesktopnum(void);
void setfocus(Client *c);
void setfullscreen(Client *c, int fullscreen);
void setshowbar(Monitor *m, int show);
void setup(void);
void setupatom(void);
void setupcur(void);
void setuppool(void);
void setuptags(void);
void seturgent(Client *c, int urg);
void setviewport(void);
void showhide(Client *c);
void showtagpreview(unsigned int i);
void sigchld();
void sighup();
void sigterm();
int  stackpos(int SHIFT_TYPE);
void takepreview(void);
void tile(Monitor *m);
void unfocus(Client *c, int setfocus);
void unmanage(Client *c, int destroyed);
void unmapnotify(XEvent *e);
void updatebarpos(Monitor *m);
void updatebars(void);
void updateclientlist(void);
void updatedesktop(void);
int  updategeom(void);
void updatemotifhints(Client *c);
void updatenumlockmask(void);
void updatesizehints(Client *c);
void updatestatus(void);
void updatetitle(Client *c);
void updateicon(Client *c);
void updatewindowtype(Client *c);
void updatewmhints(Client *c);
void visiblitynotify(XEvent *e);
void winsetstate(Window win, long state);
void winmap(Client *c, int deiconify);
void winunmap(Window win, Window winroot, int iconify);
Client  *wintoclient(Window w);
Monitor *wintomon(Window w);

/* yes these shadow dpy; no dont change them */
int  xerror(Display *dpy, XErrorEvent *ee);
int  xerrordummy(Display *dpy, XErrorEvent *ee);
int  xerrorstart(Display *dpy, XErrorEvent *ee);

/* variables */
Client *lastfocused = NULL;
Pool *pl = NULL;
char stext[256];     /* status WM_NAME text */
int screen;
int sw, sh;          /* X display screen geometry width, height */
int bh;              /* bar height */
int lrpad;           /* sum of left and right padding for text */
int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask = 0;
int tagsel;
void (*handler[LASTEvent]) (XEvent *) =
{
    /* Input */
    [ButtonPress] = buttonpress,
    [KeyPress] = keypress,
    [KeyRelease] = keyrelease,
    /* client */
    [ClientMessage] = clientmessage,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
    [FocusIn] = focusin,
    [ResizeRequest] = resizerequest,
    /* properties */
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    /* mapping */
    [Expose] = expose,
    [MapRequest] = maprequest,
    [VisibilityNotify] = visiblitynotify,
    [MappingNotify] = mappingnotify,
    [UnmapNotify] = unmapnotify
};
Atom wmatom[WMLast], netatom[NetLast], motifatom;
int running = 1;
int RESTART = 0;
Cur *cursor[CurLast];
Clr **scheme;
Clr **tagscheme;
Display *dpy;
Drw *drw;
Monitor *mons, *selmon;
Window root, wmcheckwin;
/* ACC */
unsigned int accnum; /* Active client counter Number */

#endif
