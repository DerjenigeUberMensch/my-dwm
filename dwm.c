/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 *
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h> //config.h multimedia keys
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include <time.h> /* ALT TAB */

#include "drw.h"
#include "util.h" /* Include near dpy defintions due to conflicting variables */
/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
        * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define OPAQUE                  0xffU
#define SESSION_FILE            "/tmp/dwm-session"        
/* motif window decoration */
#define MWM_HINTS_FLAGS_FIELD       0
#define MWM_HINTS_DECORATIONS_FIELD 2
#define MWM_HINTS_DECORATIONS       (1 << 1)
#define MWM_DECOR_ALL               (1 << 0)
#define MWM_DECOR_BORDER            (1 << 1)
#define MWM_DECOR_TITLE             (1 << 3)
/* enums */
/* cursor */
enum { CurNormal, CurResize, CurMove, CurLast };

/* color schemes */
enum {
    SchemeNorm, SchemeSel,                      /* default */
    SchemeUrgent, SchemeWarn,                   /* signals */
    SchemeAltTab, SchemeAltTabSelect,           /* alt tab */
    SchemeBarTabActive, SchemeBarTabInactive,   /* bar tab */
    SchemeTagActive,                            /*  tags   */
};

/* EWMH atoms */
/* when adding new properties make sure NetLast is indeed last to allocate enough memory to store all Nets in an array */
enum { NetSupported, NetWMName, NetWMIcon, NetWMState, NetWMCheck,
       NetWMFullscreen, NetWMAlwaysOnTop,NetActiveWindow, 
       NetWMWindowType, NetWMWindowTypeDialog, NetClientList,
       NetWMWindowsOpacity,
       NetLast, 
     };

/* default atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };

/* clicks */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast
     };
/* stack shifting */
enum {  BEFORE, PREVSEL, NEXT,
        FIRST, SECOND, THIRD, LAST
};
/* layouts */
enum 
{
    TILED ,FLOATING, MONOCLE, GRID
};

/* kill client */
enum
{
    GRACEFUL, SAFEDESTROY, DESTROY
};
typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

typedef struct {
    int type;
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

typedef struct {
    unsigned int click;
    unsigned int mask;
    unsigned int button;
    void (*func)(const Arg *arg);
    const Arg arg;
} Button;
typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
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
    unsigned int oldstate;
    /* icon */
    unsigned int icw;
    unsigned int ich;
    Picture icon;   /* ulong */
    Client *next;   /* next client in linked list disregarding monitor stack */
    Client *snext;  /* stack next (stack is basically current monitor stack) */
    Monitor *mon;
    Window win; /* ulong */
};

typedef struct {
    const char *symbol;
    void (*arrange)(Monitor *);
} Layout;

struct Monitor {
    char ltsymbol[16];
    float mfact;
    int nmaster;
    int num;
    int by;               /* bar geometry */
    int mx, my, mw, mh;   /* screen size */
    int wx, wy, ww, wh;   /* window area  */
    int altTabN;		  /* move that many clients forward */
    int nTabs;			  /* number of active clients in tag */
    unsigned int isfullscreen   : 1;     /* toggle fullscr vs reg fullscr */
    unsigned int showbar        : 1;
    unsigned int topbar         : 1;


    unsigned short layout;
    unsigned int seltags;
    unsigned int sellt;
    unsigned int tagset[2];
    Client *clients;
    Client *sel;
    Client *stack;
    Client ** altsnext; /* array of all clients in the tag */
    Monitor *prev;
    Monitor *next;
    Window barwin;      /* ulong */
    Window tabwin;      /* ulong */
    const Layout *lt[2];
};

typedef struct {
    const char *class;
    const char *instance;
    const char *title;
    unsigned int tags;
    int isfloating;
    int monitor;
} Rule;

/* function declarations */
static void applyrules(Client *c);
static int  applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void cleanupsbar(Monitor *m);
static void clientmessage(XEvent *e);
static void crashhandler();
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static int  dockablewindow(Client *c);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawbartabs(Monitor *m, int x, int y, int maxw, int height);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusstack(int SHIFT_TYPE);
static Atom getatomprop(Client *c, Atom prop);
static int  getwinpid(Window window);
static uint32_t prealpha(uint32_t p);
static Picture geticonprop(Window win, unsigned int *picw, unsigned int *pich);
static int  getrootptr(int *x, int *y);
static long getstate(Window w);
static int  gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void grid(Monitor *m);
static void keyhandler(XEvent *e);
static void killclient(Client *c, int type);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(void);
static void movstack(Client *c, int SHIFT_TYPE);
static void savesession(void);
static void restoresession(void);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void restack(Monitor *m);
static void restart(void);
static void run(void);
static void scan(void);
static int  sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setclientlayout(Monitor *m, int layout);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld();
static void sighup();
static void sigterm();
static int  stackpos(int SHIFT_TYPE);
static void maximize(int x, int y, int w, int h);
static void tile(Monitor *m);
static void freeicon(Client *c);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int  updategeom(void);
static void updatemotifhints(Client *c);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updateicon(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);

static void winsetstate(Window win, long state);
static void winmap(Client *c, int deiconify);
static void winunmap(Window win, Window winroot, int iconify);
static Client  *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int  xerror(Display *display, XErrorEvent *ee);
static int  xerrordummy(Display *display, XErrorEvent *ee);
static int  xerrorstart(Display *display, XErrorEvent *ee);


static void alttab();
static void drawtab(int nwins, int first, Monitor *m);
static void alttabend();
/* variables */
static Client *lastfocused = NULL;
static const char broken[] = "broken";
static char stext[256];      /* status WM_NAME text */
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
    [ButtonPress] = buttonpress,
    [ClientMessage] = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify, //Comment to disable focuschangeOnMouseMove Mouse focus
    [Expose] = expose,
    [FocusIn] = focusin,
    [KeyPress] = keyhandler,
    [KeyRelease] = keyhandler,
    [MappingNotify] = mappingnotify,
    [MapRequest] = maprequest,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    [UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], motifatom;
static int running = 1;
static int RESTART = 0;
static Cur *cursor[CurLast];
static Clr **scheme;
static Clr **tagscheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
                    /* ACC */
static unsigned int accnum; /* number of active clients (alive) */
/* configuration, allows nested code to access above variables */
#include "toggle.h"
#include "config.h"
#include "keybinds.h"
#include "toggle.c"
/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags {
    char limitexceeded[LENGTH(tags) > 31 ? -1 : 1];
};

/* function implementations */
void
applyrules(Client *c)
{
    const char *class, *instance;
    unsigned int i;
    const Rule *r;
    Monitor *m;
    XClassHint ch = { NULL, NULL };

    /* rule matching */
    c->isfloating = 0;
    c->tags = 0;
    XGetClassHint(dpy, c->win, &ch);
    class    = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name  ? ch.res_name  : broken;

    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
                && (!r->class || strstr(class, r->class))
                && (!r->instance || strstr(instance, r->instance)))
        {
            c->isfloating = r->isfloating;
            c->tags |= r->tags;
            for (m = mons; m && m->num != r->monitor; m = m->next);
            if (m)
                c->mon = m;
        }
    }
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
    c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
    int baseismin;
    Monitor *m = c->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if (interact) {
        if (*x > sw)
            *x = sw - WIDTH(c);
        if (*y > sh)
            *y = sh - HEIGHT(c);
        if (*x + *w + 2 * c->bw < 0)
            *x = 0;
        if (*y + *h + 2 * c->bw < 0)
            *y = 0;
    } else {
        if (*x >= m->wx + m->ww)
            *x = m->wx + m->ww - WIDTH(c);
        if (*y >= m->wy + m->wh)
            *y = m->wy + m->wh - HEIGHT(c);
        if (*x + *w + 2 * c->bw <= m->wx)
            *x = m->wx;
        if (*y + *h + 2 * c->bw <= m->wy)
            *y = m->wy;
    }
    if (*h < bh)
        *h = bh;
    if (*w < bh)
        *w = bh;
    if (CFG_RESIZE_HINTS || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
        if (!c->hintsvalid)
            updatesizehints(c);
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = c->basew == c->minw && c->baseh == c->minh;
        if (!baseismin) { /* temporarily remove base dimensions */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for aspect limits */
        if (c->mina > 0 && c->maxa > 0) {
            if (c->maxa < (float)*w / *h)
                *w = *h * c->maxa + 0.5;
            else if (c->mina < (float)*h / *w)
                *h = *w * c->mina + 0.5;
        }
        if (baseismin) { /* increment calculation requires this */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for increment value */
        if (c->incw)
            *w -= *w % c->incw;
        if (c->inch)
            *h -= *h % c->inch;
        /* restore base dimensions */
        *w = MAX(*w + c->basew, c->minw);
        *h = MAX(*h + c->baseh, c->minh);
        if (c->maxw)
            *w = MIN(*w, c->maxw);
        if (c->maxh)
            *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
    showhide(m->stack);
    arrangemon(m);
    restack(m);
}

void 
arrangeall()
{
    Monitor *m;

    for(m = mons; m; m = m->next)
    {
        showhide(m->stack);
        arrangemon(m);
    }
    restack(m);
}

void
arrangemon(Monitor *m)
{
    strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
    if (m->lt[m->sellt]->arrange)
        m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
    c->next = c->mon->clients;
    c->mon->clients = c;
}

void
attachstack(Client *c)
{
    c->snext = c->mon->stack;
    c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
    unsigned int i, x, click;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    if (ev->window == selmon->barwin) {
        i = x = 0;
        do
            x += TEXTW(tags[i]);
        while (ev->x >= x && ++i < LENGTH(tags));
        if (i < LENGTH(tags)) 
        {
            click = ClkTagBar;
            arg.ui = 1 << i;
        } else if (ev->x < x + TEXTW(selmon->ltsymbol))
            click = ClkLtSymbol;
        else if (ev->x > selmon->ww - (int)TEXTW(stext) * CFG_SHOW_WM_NAME)
            click = ClkStatusText;
        /* click = ClkWinTitle; */
    } else if ((c = wintoclient(ev->window))) {
        focus(c);
        restack(selmon);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < LENGTH(buttons); i++)
        if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
                && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
            buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XSync(dpy, False);
}

void
cleanup(void)
{
    Arg a = {.ui = ~0};
    Layout foo = { "", NULL };
    Monitor *m;
    size_t i;

    alttabend();
    View(&a);
    selmon->lt[selmon->sellt] = &foo;
    for (m = mons; m; m = m->next)
        while (m->stack)
            unmanage(m->stack, 0);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons)
        cleanupmon(mons);
    for (i = 0; i < CurLast; i++)
        drw_cur_free(drw, cursor[i]);
    for (i = 0; i < LENGTH(colors); i++)
        free(scheme[i]);
    free(scheme);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
    Monitor *m;

    if (mon == mons)
        mons = mons->next;
    else {
        for (m = mons; m && m->next != mon; m = m->next);
        m->next = mon->next;
    }
    cleanupsbar(mon);
    free(mon);
}

void
cleanupsbar(Monitor *m) /* status bar */
{
    XUnmapWindow(dpy, m->barwin);
    XDestroyWindow(dpy, m->barwin);
}

void
clientmessage(XEvent *e)
{
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

    if (!c)
        return;
    if (cme->message_type == netatom[NetWMState]) {
        if (cme->data.l[1] == netatom[NetWMFullscreen]
                || cme->data.l[2] == netatom[NetWMFullscreen])
            setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
                              || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
        if(cme->data.l[1] == netatom[NetWMAlwaysOnTop]
                || cme->data.l[2] == netatom[NetWMAlwaysOnTop])
        {
            c->isfloating = 1;
            c->alwaysontop = 1;
        }
    } else if (cme->message_type == netatom[NetActiveWindow]) {
        if (c != selmon->sel && !c->isurgent)
            seturgent(c, 1);
    }

}
void
crashhandler(void)
{
    if(!running) return;
    char *filename;
    FILE *f;

    filename = "/var/log/dwm.log";
    f = fopen(filename, "a");
    if(!f) fopen(filename, "w");

    /* time handler */
    time_t mytime = time(NULL);
    char * time_str = ctime(&mytime);
    time_str[strlen(time_str)-1] = '\0';
    fprintf(f, "%s %s\n", strerror(errno), time_str);

    fclose(f);

}
void
configure(Client *c)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = dpy;
    ce.event = c->win;
    ce.window = c->win;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->w;
    ce.height = c->h;
    ce.border_width = c->bw;
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
    Monitor *m;
    Client *c;
    XConfigureEvent *ev = &e->xconfigure;
    int dirty;

    /* TODO: updategeom handling sucks, needs to be simplified */
    if (ev->window == root) {
        dirty = (sw != ev->width || sh != ev->height);
        sw = ev->width;
        sh = ev->height;
        if (updategeom() || dirty) {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next) {
                for (c = m->clients; c; c = c->next)
                    if (c->isfullscreen)
                        resizeclient(c, m->mx, m->my, m->mw, m->mh);
                XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
            }
            focus(NULL);
            arrangeall();
        }
    }
}

void
configurerequest(XEvent *e)
{
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if ((c = wintoclient(ev->window))) {
        if (ev->value_mask & CWBorderWidth)
            c->bw = ev->border_width;
        else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
            m = c->mon;
            if (ev->value_mask & CWX) {
                c->oldx = c->x;
                c->x = m->mx + ev->x;
            }
            if (ev->value_mask & CWY) {
                c->oldy = c->y;
                c->y = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth) {
                c->oldw = c->w;
                c->w = ev->width;
            }
            if (ev->value_mask & CWHeight) {
                c->oldh = c->h;
                c->h = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
                c->x = m->mx + ((m->mw >> 1) - (WIDTH(c) >> 1)); /* center in x direction */
            if ((c->y + c->h) > m->my + m->mh && c->isfloating)
                c->y = m->my + ((m->mh >> 1) - (HEIGHT(c) >> 1)); /* center in y direction */
            if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
                configure(c);
            if (ISVISIBLE(c))
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        } else
            configure(c);
    } else {
        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
    XSync(dpy, False);
}

Monitor *
createmon(void)
{
    Monitor *m;

    m = ecalloc(1, sizeof(Monitor));
    m->tagset[0] = m->tagset[1] = 1;
    m->mfact = CFG_MONITOR_FACT;
    m->nmaster = CFG_MASTER_COUNT;
    m->showbar = CFG_SHOW_BAR;
    m->topbar = CFG_TOP_BAR;
    m->lt[0] = &layouts[0];
    m->lt[1] = &layouts[1 % LENGTH(layouts)];
    m->nTabs = 0;
    m->isfullscreen = 0;
    strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
    return m;
}

void
destroynotify(XEvent *e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if ((c = wintoclient(ev->window)))
        unmanage(c, 1);
}

void
detach(Client *c)
{
    Client **tc;

    for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
}

void
detachstack(Client *c)
{
    Client **tc, *t;

    for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;

    if (c == c->mon->sel) {
        for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
        c->mon->sel = t;
    }
}

Monitor *
dirtomon(int dir)
{
    Monitor *m = NULL;

    if (dir > 0) {
        if (!(m = selmon->next))
            m = mons;
    } else if (selmon == mons)
        for (m = mons; m->next; m = m->next);
    else
        for (m = mons; m->next != selmon; m = m->next);
    return m;
}

void
drawbar(Monitor *m)
{
    int x, w, tw = 0;

    int boxs = lrpad >> 4;
    int boxw = boxs + 2;
    unsigned int i, occ = 0, urg = 0;
    int tagselected;
    Client *c;

    if (!m->showbar) return;
    /* draw status first so it can be overdrawn by tags later */
    if (m == selmon) /* status is only drawn on selected monitor */
    { 
        drw_setscheme(drw, scheme[SchemeNorm]);
        if(CFG_SHOW_WM_NAME)
        {
		    tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
		    drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
        }
    }

    for (c = m->clients; c; c = c->next)
    {
        occ |= c->tags;
        if (c->isurgent) urg |= c->tags;
    }
    x = 0;
    /* drw tags */
    for (i = 0; i < LENGTH(tags); i++) {
        tagselected = m == selmon && selmon->sel && selmon->sel->tags & 1 << i;
        w = TEXTW(tags[i]);
        drw_setscheme(drw, tagscheme[i]);
        if(tagselected) 
            drw_setscheme(drw, scheme[SchemeTagActive]);
        drw_text(drw, x, 0, w, bh, lrpad >> 1, tags[i], urg & 1 << i);
        if (occ & 1 << i)
        {
            drw_rect(drw, x + boxw, MAX(bh - boxw, 0), w - ( ( boxw << 1) + 1), boxw,
                     m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
                     urg & 1 << i);
        }
        x += w;
    }
    w = TEXTW(m->ltsymbol);
    drw_setscheme(drw, scheme[SchemeNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad >> 1, m->ltsymbol, 0);

    /* alloc space for tabs */
    drw_rect(drw, x, 0, m->ww - tw - x, bh, 1, 0);
    if ((w = m->ww - tw - x) > bh) drawbartabs(m, x, 0, w, bh);
    drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawbartabs(Monitor *m, int x, int y, int maxw, int height)
{
    /* y isnt implemented yet */
    Client *c;
    unsigned int tabcnt;    /* tab count */
    unsigned int tabsz;     /* tab size */
    unsigned int iconspace;
    unsigned int boxh; /* tiny floating box indicator h */
    unsigned int boxw; /* tiny floating box indicator w */
    int curscheme; /* current scheme */
    int cc; /* client counter */
    int btpos;  /* current bartab positon x */

    btpos = 0;
    cc = 0;
    tabcnt = 0;
    boxh = 5;
    boxw = 5;
    /* get count */
    for (c = m->clients; c; c = c->next) tabcnt += !!ISVISIBLE(c);

    /* draw empty if none selected */
    if(!tabcnt)
    {
        drw_setscheme(drw, scheme[SchemeNorm]);
        drw_rect(drw, x, 0, maxw, height, 1, 1);
        return;
    }

    tabsz = maxw / tabcnt;
    if(tabsz < TEXTW("..."))
    {
        iconspace = m->sel->icon ? m->sel->icw + CFG_ICON_SPACE : lrpad >> 1;
        drw_setscheme(drw, scheme[SchemeBarTabInactive]);
        drw_text(drw, x, y, maxw, height, iconspace, m->sel->name, 0);
        if(m->sel->icon)
            drw_pic(drw, x, y + ((height - m->sel->ich ) >> 1), m->sel->icw, m->sel->ich, m->sel->icon);
        if(m->sel->isfloating)
            drw_rect(drw, x, y + boxh, boxw, boxw, m->sel->isfixed, 0);
        return;
    }

    for(c = m->clients; c; c = c->next)
    {
        if(!ISVISIBLE(c)) continue;
        btpos = cc * tabsz;
        curscheme = c == m->sel ? SchemeBarTabActive : SchemeBarTabInactive;
        iconspace = c->icon ? c->icw + CFG_ICON_SPACE : lrpad >> 1;
        drw_setscheme(drw, scheme[curscheme]);
        drw_text(drw, x + btpos, y, tabsz, height, iconspace, c->name, 0);
        /* draw icon */
        if(c->icon)
            drw_pic(drw, x + btpos + iconspace - lrpad, y + ((height - c->ich) >> 1), c->icw, c->ich, c->icon);
        /* draw tiny floating box indicator */
        if (c->isfloating)
            drw_rect(drw, x + btpos + boxw, y + boxh, boxw, boxw, c->isfixed, 0);
        ++cc;
    }

}
void
drawbars(void)
{
    Monitor *m;

    for (m = mons; m; m = m->next)
        drawbar(m);
}

void
enternotify(XEvent *e)
{
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
        return;
    if(!CFG_HOVER_FOCUS)
        return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon) 
    {
        unfocus(selmon->sel, 1);
        selmon = m;
    }
    else if (!c || c == selmon->sel)
        return;
    focus(c);
}

void
expose(XEvent *e)
{
    Monitor *m;
    XExposeEvent *ev = &e->xexpose;

    if (ev->count == 0 && (m = wintomon(ev->window)))
        drawbar(m);
}

void
focus(Client *c)
{
    if (!c || !ISVISIBLE(c))
        for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
    if (selmon->sel && selmon->sel != c)
        unfocus(selmon->sel, 0);
    if (c) {
        if (c->mon != selmon)
            selmon = c->mon;
        if (c->isurgent)
            seturgent(c, 0);
        detachstack(c);
        attachstack(c);
        grabbuttons(c, 1);
        /* set new focused border first to avoid flickering */
        XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
        /* lastfocused may be us if another window was unmanaged */
        if (lastfocused && lastfocused != c)
			XSetWindowBorder(dpy, lastfocused->win, scheme[SchemeNorm][ColBorder].pixel);
        setfocus(c);
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }

    selmon->sel = c;
    drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
}

/* fix later, usage may result in undefined behaviour */
void
focusstack(int SHIFT_TYPE)
{
    int i = stackpos(SHIFT_TYPE);
    Client *c, *p;

    if(i < 0)
        return;
    for(p = NULL, c = selmon->clients; c && (i || !ISVISIBLE(c));
            i -= ISVISIBLE(c) ? 1 : 0, p = c, c = c->next);
    focus(c ? c : p);
    restack(selmon);
}

Atom
getatomprop(Client *c, Atom prop)
{
    int di;
    unsigned long dl;
    unsigned char *p = NULL;
    Atom da, atom = None;

    if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
                           &da, &di, &dl, &dl, &p) == Success && p) {
        atom = *(Atom *)p;
        XFree(p);
    }
    return atom;
}
int
getwinpid(Window window) {
    Atom actualType;
    int format;
    unsigned long nitems, bytesAfter;
    unsigned char *propData = NULL;

    Atom atom = XInternAtom(dpy, "_NET_WM_PID", False);
    if (XGetWindowProperty(dpy, window, atom, 0, 1, False, XA_CARDINAL,
                           &actualType, &format, &nitems, &bytesAfter, &propData) != Success) return -1;

    if (propData != NULL) {
        pid_t pid = *((pid_t*)propData);
        XFree(propData);
        return pid;
    }

    return -1;
}

uint32_t 
prealpha(uint32_t p) {
	uint8_t a = p >> 24u;
	uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
	uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
	return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
}

Picture
geticonprop(Window win, unsigned int *picw, unsigned int *pich)
{
    int format;
    unsigned int bitformat = 32; 
    unsigned int unitbytecount = 16384; 
	Picture ret;
	unsigned long n, extra, *p = NULL;
	Atom real;
	if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType, 
						   &real, &format, &n, &extra, (unsigned char **)&p) != Success) { XFree(p); return None;}
	if (n == 0 || format != bitformat) { XFree(p); return None;}
	unsigned long *bstp = NULL;
	uint32_t w, h, sz;
	{
		unsigned long *i;
        const unsigned long *end = p + n;
		uint32_t bstd = UINT32_MAX, d, m;
        for (i = p; i < end - 1; i += sz) 
        {
		    if ((w = *i++) >= unitbytecount  || (h = *i++) >= unitbytecount) { XFree(p); return None; }
		    if ((sz = w * h) > end - i) break;
		    if ((m = w > h ? w : h) >= CFG_ICON_SIZE && (d = m - CFG_ICON_SIZE) < bstd) { bstd = d; bstp = i; }
	    }
		if (!bstp) {
			for (i = p; i < end - 1; i += sz) 
            {
				if ((w = *i++) >= unitbytecount || (h = *i++) >= unitbytecount ) { XFree(p); return None; }
				if ((sz = w * h) > end - i) break;
				if ((d = CFG_ICON_SIZE - (w > h ? w : h)) < bstd) { bstd = d; bstp = i; }
			}
		}
		if (!bstp) { XFree(p); return None;}
	}

	if ((w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) { XFree(p); return None;}

    uint32_t icw, ich;
    if (w <= h) {
        ich = CFG_ICON_SIZE;
        icw = w * CFG_ICON_SIZE / h;
        icw += !icw;
    }
    else 
    {
        icw = CFG_ICON_SIZE;
        ich = h * CFG_ICON_SIZE / w;
        ich += !ich;
	}
	*picw = icw; *pich = ich;

	uint32_t i, *bstp32 = (uint32_t *)bstp;
	for (sz = w * h, i = 0; i < sz; ++i) bstp32[i] = prealpha(bstp[i]);

	ret = drw_picture_create_resized(drw, (char *)bstp, w, h, icw, ich);
    XFree(p);
    return ret;
}

int
getrootptr(int *x, int *y)
{
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;

    if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
                           &real, &format, &n, &extra, (unsigned char **)&p) != Success)
        return -1;
    if (n != 0)
        result = *p;
    XFree(p);
    return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
    char **list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0)
        return 0;
    text[0] = '\0';
    if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
        return 0;
    if (name.encoding == XA_STRING) {
        strncpy(text, (char *)name.value, size - 1);
    } else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
        strncpy(text, *list, size - 1);
        XFreeStringList(list);
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

void
grabbuttons(Client *c, int focused)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        if (!focused)
            XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
                        BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        for (i = 0; i < LENGTH(buttons); i++)
            if (buttons[i].click == ClkClientWin)
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabButton(dpy, buttons[i].button,
                                buttons[i].mask | modifiers[j],
                                c->win, False, BUTTONMASK,
                                GrabModeAsync, GrabModeSync, None, None);
    }
}

void
grabkeys(void)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
        KeyCode code;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for (i = 0; i < LENGTH(keys); i++)
            if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
                             True, GrabModeAsync, GrabModeAsync);
    }
}
void
grid(Monitor *m) {
	unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
	Client *c;
	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
		n++;

	/* grid dimensions */
	for(rows = 0; rows <= n/2; rows++)
		if(rows*rows >= n)
			break;
	cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

	/* window geoms (cell height/width) */
	ch = m->wh / (rows ? rows : 1);
	cw = m->ww / (cols ? cols : 1);
	for(i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
		cx = m->wx + (i / rows) * cw;
		cy = m->wy + (i % rows) * ch;
		/* adjust height/width of last row/column's windows */
		ah = ((i + 1) % rows == 0) ? m->wh - ch * rows : 0;
		aw = (i >= rows * (cols - 1)) ? m->ww - cw * cols : 0;
		resize(c, cx, cy, cw - 2 * c->bw + aw, ch - 2 * c->bw + ah, False);
		i++;
	}
}


#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
    while (n--)
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
                && unique[n].width == info->width && unique[n].height == info->height)
            return 0;
    return 1;
}
#endif /* XINERAMA */

void
keyhandler(XEvent *e)
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0); /* deprecated dont care */
    for (i = 0; i < LENGTH(keys); i++)
        if (keysym == keys[i].keysym
                && ev->type == keys[i].type
                && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
                && keys[i].func)
            keys[i].func(&(keys[i].arg));
}


/* handle new windows */
/* NOTE: most of these values are uninitialized so using them is unwise, Set them First! */
void
manage(Window w, XWindowAttributes *wa)
{
    ++accnum;
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    /* alloc enough memory for new client struct */
    c = ecalloc(1, sizeof(Client));
    c->win = w;
    /* initialize geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;
    updateicon(c);
    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) 
    {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = selmon;
        applyrules(c);
    }

    if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
        c->x = c->mon->wx + c->mon->ww - WIDTH(c);
    if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
        c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->wx);
    c->y = MAX(c->y, c->mon->wy);
    c->bw = CFG_BORDER_PX;

    wc.border_width = c->bw;
    XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    updatemotifhints(c);
    XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    grabbuttons(c, 0);
    c->wasfloating = 0;
    c->ismax = 0;
    if (!c->isfloating)
        c->isfloating = c->oldstate = trans != None || c->isfixed;
    /* set floating if always on top */
    c->isfloating = c->isfloating || c->alwaysontop;
    if (c->isfloating)
        XRaiseWindow(dpy, c->win);

    /* check if selmon->fullscreen */
    setfullscreen(c, selmon->isfullscreen);
    attach(c);
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
    setclientstate(c, NormalState);
    if (c->mon == selmon)
        unfocus(selmon->sel, 0);
    c->mon->sel = c;
    /* destroy any new clients if we past our client limit */
    if(CFG_MAX_CLIENT_COUNT < accnum) 
    {
        XMapWindow(dpy, c->win);
        unmanage(c, 0); 
        killclient(c, SAFEDESTROY);
        return;
    }
    arrange(c->mon);
    XMapWindow(dpy, c->win);
    focus(NULL);
}

void
mappingnotify(XEvent *e)
{
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if (ev->request == MappingKeyboard)
        grabkeys();
}
/* handle new client request */
void
maprequest(XEvent *e)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;

    if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
        return;
    if (!wintoclient(ev->window))
        manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
    unsigned int n = 0;
    Client *c;
    for (c = m->clients; c; c = c->next)
        if (ISVISIBLE(c))
            n++;
    if (n > 0) /* override layout symbol */
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
        resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

void
motionnotify(XEvent *e)
{
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if (ev->window != root)
        return;
    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    mon = m;
}

Client *
nexttiled(Client *c)
{
    for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
    return c;
}

void
pop(Client *c)
{
    detach(c);
    attach(c);
    focus(c);
    arrange(c->mon);
}
/* when a window changes properties for what ever reason this is called */
void
propertynotify(XEvent *e)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;
    /* notify */
    int nfyname;
    int nfyicon;
    int nfytype;
    int nfymotif;
    int nfybar;

    if ((ev->window == root) && ev->atom == XA_WM_NAME)
        updatestatus();
    else if (ev->state == PropertyDelete)
        return; /* ignore */
    else if ((c = wintoclient(ev->window))) {
        switch(ev->atom) 
        {case XA_WM_TRANSIENT_FOR:
            if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
                    (c->isfloating = (wintoclient(trans)) != NULL))
                arrange(c->mon);
            break;
        case XA_WM_NORMAL_HINTS:c->hintsvalid = 0;              break;
        case XA_WM_HINTS:       updatewmhints(c); drawbars();   break;
        default: break;
        }
        nfyname = ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName];
        nfyicon = ev->atom == netatom[NetWMIcon];
        nfytype = ev->atom == netatom[NetWMWindowType];
        nfymotif= ev->atom == motifatom;

        nfybar = nfyname || nfyicon || nfytype || nfymotif;
        if (nfyname)  updatetitle(c);
        if (nfyicon)  updateicon(c);
        if (nfytype)  updatewindowtype(c);
        if (nfymotif) updatemotifhints(c);
        if (nfybar)   drawbar(c->mon);
    }
}

void 
movstack(Client *sel, int SHIFT_TYPE) {
    /* shifts the stack instead of moving it around
    * SHIFT_TYPE:  
    * {PREVSEL, BEFORE, NEXT, FIRST, SECOND, THIRD, LAST} 
    */
	int i = stackpos(SHIFT_TYPE);
	Client *c, *p;

	if(i < 0)
		return;
	else if(!i) {
		detach(sel);
		attach(sel);
	}
	else {
		for(p = NULL, c = selmon->clients; c; p = c, c = c->next)
			if(!(i -= (ISVISIBLE(c) && c != sel)))
			    break;
		c = c ? c : p;
		detach(sel);
		sel->next = c->next;
		c->next = sel;
    }
}


void
savesession(void)
{
	FILE *fw = fopen(SESSION_FILE, "w");
	for (Client *c = selmon->clients; c != NULL; c = c->next) { // get all the clients with their tags and write them to the file
		fprintf(fw, 
               "%lu %u \
                %d %d %d %d \
                %d %d %d %d \
                %d %d %d %d \
                %d %d %d %d \
                %d %d \
                %u %d \
                \n", 
                c->win, c->tags,
                c->oldx, c->oldy, c->x, c->y,
                c->oldw, c->oldh, c->w, c->h,
                c->alwaysontop, c->hintsvalid, c->ismax, c->wasfloating,
                c->isfixed, c->isfloating, c->isurgent, c->neverfocus,
                c->isfullscreen, c->oldstate,
                c->mon->layout, !!(selmon->sel == c)
                );
	}
	fclose(fw);
}

void
restoresession(void)
{
    const int MAX_LENGTH = 1024;
    const int CHECK_SUM =  22; /* equal to number of sscanf elements so %d %d %d %d would be a checksum of 4 (type doesnt matter ) */ 
    int check;
	FILE *fr = fopen(SESSION_FILE, "r");
    Client *c;
    Client *selc = NULL; /* selected client */
	if (!fr) return;

    long unsigned int winId;
	unsigned int tagsForWin;/* client tags */
    /* do note some of these are REDUDANT but its mostly to the window manager "remembered" no so much so open windows forget */
    int oldx, oldy, cx, cy; /* client pos */
    int x, y;
    int w, h;
    int oldw, oldh;
    int alwaysontop;
    int hintsvalid;
    int ismax;
    int wasfloating;
    int isfixed;
    int isfloating;
    int isurgent;
    int neverfocus;
    int isfullscreen;
    int oldstate;
    int selcpos; /* layout type; c == selmon->sel */
    unsigned int winlayout;

    /* malloc enough for input*/
	char *str = malloc(MAX_LENGTH * sizeof(char));
    /* malloc for floating window clients to set floating later.. */
	while (fscanf(fr, "%[^\n] ", str) != EOF) 
    {
        check = sscanf(str, 
               "%lu %u \
                %d %d %d %d \
                %d %d %d %d \
                %d %d %d %d \
                %d %d %d %d \
                %d %d \
                %u %d \
                \n", 
                &winId, &tagsForWin,
                &oldx, &oldy, &x, &y,
                &oldw, &oldh, &w, &h,
                &alwaysontop, &hintsvalid, &ismax, &wasfloating,
                &isfixed, &isfloating, &isurgent, &neverfocus,
                &isfullscreen, &oldstate,
                &winlayout, &selcpos
                );
		if (check != CHECK_SUM) continue;
		for (c = selmon->clients; c ; c = c->next) 
        { 
			if (c->win == winId) 
            {
                /* assign as selmon->sel */
                if(selcpos) selc = c;
                /* assign tags */
				c->tags         = tagsForWin;
                c->x            = x; c->y = y;
                c->w            = w; c->h = h;
                c->oldx         = oldx; c->oldy = oldy;
                c->oldw         = oldw; c->oldh = oldh;
                c->alwaysontop  = alwaysontop;
                c->hintsvalid   = hintsvalid;
                c->ismax        = ismax;
                c->wasfloating  = wasfloating;
                c->isfixed      = isfixed;
                c->isfloating   = isfloating;
                c->isurgent     = isurgent;
                c->neverfocus   = neverfocus;
                c->isfullscreen = isfullscreen;
                c->oldstate     = oldstate;
                if(c->isfloating) resize(c, c->x, c->y, c->w, c->h, 0);
                else setclientlayout(c->mon, winlayout);
                break;
            }
        }

    }
    focus(selc); /* focus NULL if no client was selected, ie dont focus any clients */
    for (Monitor *m = selmon; m; m = m->next) arrange(m);
    free(str);
    fclose(fr);
    // delete a file
    remove(SESSION_FILE);
}

void 
restart(void)
{
    RESTART = 1;
    quit();
}

void
quit(void)
{
    running = 0;
}

int
stackpos(int SHIFT_TYPE) 
{
    int n, i;
    Client *c, *l;

    if(!selmon->clients)
        return -1;
    
    switch(SHIFT_TYPE)
    {case BEFORE:
            if(!selmon->sel)
                return -1;
            for(i = 0, c = selmon->clients; c != selmon->sel; i += !!ISVISIBLE(c), c = c->next);
            for(n = i; c; n += !!ISVISIBLE(c), c = c->next);
            return MOD(i - 1, n);
    case PREVSEL:
            for(l = selmon->stack; l && (!ISVISIBLE(l) || l == selmon->sel); l = l->snext);
            if(!l)
                return -1;
            for(i = 0, c = selmon->clients; c != l; i += !!ISVISIBLE(c), c = c->next);
            return i;
    case NEXT:
            if(!selmon->sel)
                return -1;
            for(i = 0, c = selmon->clients; c != selmon->sel; i += !!ISVISIBLE(c), c = c->next);
            for(n = i; c; n += !!ISVISIBLE(c), c = c->next);
            return MOD(i + 1, n);
    case FIRST:  return 0;
    case SECOND: return 1;
    case THIRD:  return 2;
    case LAST:   for(i = 0, c = selmon->clients; c; i += !!ISVISIBLE(c), c = c->next){}; 
                 return MAX(i - 1, 0);
    default:     return 0;
    }
}

Monitor *
recttomon(int x, int y, int w, int h)
{
    Monitor *m, *r = selmon;
    int a, area = 0;

    for (m = mons; m; m = m->next)
        if ((a = INTERSECT(x, y, w, h, m)) > area) {
            area = a;
            r = m;
        }
    return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
    if (applysizehints(c, &x, &y, &w, &h, interact))
        resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
    XWindowChanges wc;

    c->oldx = c->x;
    c->x = wc.x = x;
    c->oldy = c->y;
    c->y = wc.y = y;
    c->oldw = c->w;
    c->w = wc.width = w;
    c->oldh = c->h;
    c->h = wc.height = h;
    wc.border_width = c->bw;
    XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
    configure(c);
    XSync(dpy, False);
}

void
restack(Monitor *m)
{
    /* fix later slow when restacking many clients */
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawbar(m);
    if (!m->sel) return;
    if(m->sel->isfloating || m->sel->alwaysontop) XRaiseWindow(dpy, m->sel->win);
    if (m->lt[m->sellt]->arrange) 
    {
        wc.stack_mode = Below;
        wc.sibling = m->barwin;
        for (c = m->stack; c; c = c->snext)
        {
            if(!ISVISIBLE(c)) continue;
            if(c->isfloating)  XRaiseWindow(dpy, c->win);
            else
            {
                XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
                wc.sibling = c->win;
            }
        }
        /* quick patch for alwaysontop windows fix later */
        for(c = m->stack; c; c = c->snext) if(ISVISIBLE(c) && c->alwaysontop) XRaiseWindow(dpy, c->win);
    }
    XSync(dpy, False);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
    XEvent ev;
    /* main event loop */
    XSync(dpy, False);

    while (running && !XNextEvent(dpy, &ev))
    {
        if (handler[ev.type]) handler[ev.type](&ev); /* call handler */
    }
}
/* scan for new clients */
void
scan(void)
{
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
        for (i = 0; i < num; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                    || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
                continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
                manage(wins[i], &wa);
        }
        for (i = 0; i < num; i++) { /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa))
                continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                    && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
                manage(wins[i], &wa);
        }
        if (wins)
            XFree(wins);
    }
}

void
sendmon(Client *c, Monitor *m)
{
    if (c->mon == m)
        return;
    unfocus(c, 1);
    detach(c);
    detachstack(c);
    c->mon = m;
    c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    attach(c);
    attachstack(c);
    focus(NULL);
    arrangeall();
}

void
setclientstate(Client *c, long state)
{
    long data[] = { state, None };

    XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Client *c, Atom proto)
{
    int n;
    Atom *protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
        while (!exists && n--)
            exists = protocols[n] == proto;
        XFree(protocols);
    }
    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &ev);
    }
    return exists;
}
void
setclientlayout(Monitor *m, int layout)
{
    switch(layout)
    {
        case TILED:    m->lt[selmon->sellt] = (Layout *)&layouts[0];break;
        case FLOATING: m->lt[selmon->sellt] = (Layout *)&layouts[1];break; 
        case MONOCLE:  m->lt[selmon->sellt] = (Layout *)&layouts[2];break;
        case GRID:     m->lt[selmon->sellt] = (Layout *)&layouts[3];break;
    }
    m->layout = layout;
}
void
setfocus(Client *c)
{
    if (!c->neverfocus) {
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *) &(c->win), 1);
    }
    sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
    if (fullscreen && !c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
        c->isfullscreen = 1;
        c->oldstate = c->isfloating;
        c->oldbw = c->bw;
        c->bw = 0;
        c->isfloating = 1;
        resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
        XRaiseWindow(dpy, c->win);
    } else if (!fullscreen && c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)0, 0);
        c->isfullscreen = 0;
        c->isfloating = c->oldstate;
        c->bw = c->oldbw;
        c->x = c->oldx;
        c->y = c->oldy;
        c->w = c->oldw;
        c->h = c->oldh;
        resizeclient(c, c->x, c->y, c->w, c->h);
        arrange(c->mon);
    }
}

void
setup(void)
{
    int i; //this is mental
    XSetWindowAttributes wa;
    Atom utf8string;

    /* clean up any zombies immediately */
    sigchld(0);
    signal(SIGHUP, sighup);
	signal(SIGTERM, sigterm);
    /* init screen */
    screen = DefaultScreen(dpy);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);
    root = RootWindow(dpy, screen);
    drw = drw_create(dpy, screen, root, sw, sh);
    if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
        die("FATAL ERROR: NO FONTS LOADED.");
    lrpad = drw->fonts->h;
    bh = drw->fonts->h + 2;
    updategeom();
    /* init atoms */
    utf8string = XInternAtom(dpy, "UTF8_STRING", False);
    wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
    netatom[NetWMIcon] = XInternAtom(dpy, "_NET_WM_ICON", False);
    netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMAlwaysOnTop] = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    netatom[NetWMWindowsOpacity] = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);

    motifatom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
    /* init cursors */
    cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResize] = drw_cur_create(drw, XC_sizing);
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
    /* init appearance */
    if (LENGTH(tags) > LENGTH(tagsel)) die("too few color schemes for the tags");
    scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
    for (i = 0; i < LENGTH(colors); i++)
        scheme[i] = drw_scm_create(drw, colors[i], 3);
    tagscheme = ecalloc(LENGTH(tagsel), sizeof(Clr *));
	for (i = 0; i < LENGTH(tagsel); i++) tagscheme[i] = drw_scm_create(drw, (char **)tagsel[i], 2);
    /* init bars */
    updatebars();
    updatestatus();
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                    PropModeReplace, (unsigned char *) WM_NAME, LENGTH(WM_NAME));
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor = cursor[CurNormal]->cursor;
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
                    |ButtonPressMask|PointerMotionMask|EnterWindowMask
                    |LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}

void
seturgent(Client *c, int urg)
{
    XWMHints *wmh;

    c->isurgent = urg;
    if (!(wmh = XGetWMHints(dpy, c->win)))
        return;
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, c->win, wmh);
    XFree(wmh);
}

void
showhide(Client *c)
{
    if (!c)
        return;
    if (ISVISIBLE(c)) {
        /* show clients top down */
        winmap(c, 1);
    } else {
        /* hide clients bottom up */
        showhide(c->snext);
        winunmap(c->win, root, 1);
    }
}

void
winsetstate(Window win, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char*)data, 2);
}

void
winmap(Client *c, int deiconify)
{
	Window win = c->win;

	if (deiconify)
		winsetstate(win, NormalState);

	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
	XMapWindow(dpy, win);
}

void
winunmap(Window win, Window winroot, int iconify)
{
	static XWindowAttributes ca, ra;

	XGrabServer(dpy);
	XGetWindowAttributes(dpy, winroot, &ra);
	XGetWindowAttributes(dpy, win, &ca);

	/* Prevent UnmapNotify events */
	XSelectInput(dpy, winroot, ra.your_event_mask & ~SubstructureNotifyMask);
	XSelectInput(dpy, win, ca.your_event_mask & ~StructureNotifyMask);

	XUnmapWindow(dpy, win);

	if (iconify)
		winsetstate(win, IconicState);

	XSelectInput(dpy, winroot, ra.your_event_mask);
	XSelectInput(dpy, win, ca.your_event_mask);
	XUngrabServer(dpy);
}

void
killclient(Client *c, int type)
{
    XEvent ev;
    int wmdelete;

    if(!c) return;

    wmdelete = sendevent(c, wmatom[WMDelete]);
    if(!wmdelete)
    {
        switch(type)
        {
            case GRACEFUL:
                XGrabServer(dpy);
                XSetErrorHandler(xerrordummy);
                XSetCloseDownMode(dpy, DestroyAll);
                XKillClient(dpy, c->win);
                XSync(dpy, False);
                XSetErrorHandler(xerror);
                XUngrabServer(dpy);
                break;
            case DESTROY:
                XGrabServer(dpy);
                XSetErrorHandler(xerrordummy);
                XSetCloseDownMode(dpy, DestroyAll);
                XDestroyWindow(dpy, c->win);
                XSync(dpy, False);
                XSetErrorHandler(xerror);
                XUngrabServer(dpy);
                break;
            case SAFEDESTROY:
                XGrabServer(dpy);
                XSetErrorHandler(xerrordummy);
                XSetCloseDownMode(dpy, DestroyAll);
                /* get window */
                XSelectInput(dpy, c->win, StructureNotifyMask);
                XKillClient(dpy, c->win);
                XSync(dpy, False);
                /* check if received DestroyNotify */
                XWindowEvent(dpy, c->win, StructureNotifyMask, &ev);
                if(ev.type != DestroyNotify) 
                { XDestroyWindow(dpy, c->win); XSync(dpy, False); }
                XSetErrorHandler(xerror);
                XUngrabServer(dpy);
                break;
        }
    }

}

void
sigchld()
{
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("FATAL ERROR: CANNOT INSTALL SIGCHLD HANDLER:");

    while (0 < waitpid(-1, NULL, WNOHANG));
}

void
sighup()
{
    restart();
}

void
sigterm()
{
    quit();
}

int
dockablewindow(Client *c) /* selmon->sel selmon=monitor;sel=currentwindowselected; */
{
    int wx; /* Window  X */
    int wy; /* Window  Y */
    int mx; /* Monitor X */
    int my; /* Monitor Y */

    int ww; /* Window Width   */
    int wh; /* Window Height  */
    int mw; /* Monitor Width  */
    int mh; /* Monitor Height */

    int isfloating;

    wx = c->x;
    wy = c->y;
    mx = c->mon->wx;
    my = c->mon->wy;

    ww = WIDTH(c);
    wh = HEIGHT(c);
    mw = c->mon->ww;
    mh = c->mon->wh;

    isfloating = c->isfloating;

    /* Check if dockable -> same height width, location, as monitor */
    return !((mx != wx) + (my != wy) + (mw != ww) + (mh != wh)) * isfloating;
}

void
maximize(int x, int y, int w, int h) {
    XEvent ev;
    Client *c;
    c = selmon->sel;

    if(!c || c->isfixed || c->isfullscreen)
        return;

    XRaiseWindow(dpy, c->win);
    if(!c->ismax) 
    {
        if(!selmon->lt[selmon->sellt]->arrange || c->isfloating)
            c->wasfloating = 1;
        else 
        {
            if(!c->isfloating)
            {
                c->isfloating = !c->isfloating;
                resize(c, c->x, c->y, c->w, c->h, 0);
            }
            arrange(c->mon);
            c->wasfloating = 0;
        }

        c->oldx = c->x;
        c->oldy = c->y;
        c->oldw = c->w;
        c->oldh = c->h;

        resize(c, x, y, w, h, 1);
        c->ismax = 1;
    }
    else 
    {
        resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 1);
        if(!c->wasfloating)
        {
            if(!c->isfloating)
            {
                c->isfloating = !c->isfloating;
                resize(c, c->x, c->y, c->w, c->h, 0);
            }
            arrange(c->mon);
        }
        c->ismax = 0;
    }
    restack(selmon);
    while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}


void
alttab()
{
    Monitor *m;
    Client *c;
    Client *altsnext;
    m = selmon;
    c = selmon->sel;
    /* move to next window */
    if (c && c->snext) 
    {
        ++m->altTabN;
        if (m->altTabN >= m->nTabs) m->altTabN = 0; /* reset altTabN */
        altsnext = m->altsnext[m->altTabN];
        if(m->layout == MONOCLE)
        {
            if(CFG_ALT_TAB_MAP_WINDOWS)
            {
                winunmap(altsnext->win, root, 1);
                /* prevents wierd artifacts from occuring because of unmaping */
                winmap(altsnext, 1);
            }
            detach(altsnext);
            attach(altsnext);
        } /* we have to attach/detach when MONOCLE else we can get undefined behaviour */
        else if(!CFG_ALT_TAB_FIXED_TILE)
        {
            detach(altsnext);
            attach(altsnext);
        }
        focus(altsnext);
        if(CFG_ALT_TAB_SHOW_PREVIEW) arrange(m);

    }
    /* redraw tab */
    drawtab(m->nTabs, 0, m);
    XRaiseWindow(dpy, m->tabwin);
}

void
drawtab(int nwins, int first, Monitor *m)
{
    Client *c;
    int maxhNeeded; /*max height needed to draw alt-tab*/
    int maxwNeeded; /*see above (width)*/
    int txtpad;     /*text padding */

    maxwNeeded = 0;
    maxhNeeded = MIN(lrpad * nwins, CFG_ALT_TAB_MAX_HEIGHT); /* breaks with too many clients MAX is not recommended */
    txtpad = 0;

    int namewpxl[nwins]; /* array of each px width for every tab name*/
    char *cnames[nwins]; /* client names */

    for(int i = 0; i < nwins; ++i)
    {
        cnames[i]   = m->altsnext[i]->name; 
        namewpxl[i] = TEXTW(cnames[i]) - lrpad;
        maxwNeeded  = MAX(namewpxl[i], maxwNeeded);
    }
    maxwNeeded = MIN(maxwNeeded + CFG_ALT_TAB_MIN_WIDTH, CFG_ALT_TAB_MAX_WIDTH);

    if (first)
    {
        m = selmon;
        XSetWindowAttributes wa =
        {
            .override_redirect = True,
            .background_pixmap = ParentRelative,
            .event_mask = ButtonPressMask|ExposureMask
        };

        /* decide position of tabwin */
        int posx = selmon->mx;
        int posy = selmon->my;

        switch(CFG_ALT_TAB_POS_X)
        {case 0: posx += 0;                                     break;
         case 1: posx += (selmon->mw >> 1) - (maxwNeeded >> 1); break;
         case 2: posx += selmon->mw - maxwNeeded;               break;
         default:posx += 0;                                     break;
        }
        switch(CFG_ALT_TAB_POS_Y)
        {case 0: posy += selmon->mh - CFG_ALT_TAB_MAX_HEIGHT;              break;
         case 1: posy += (selmon->mh >> 1) - (CFG_ALT_TAB_MAX_HEIGHT >> 1);break;
         case 2: posy += 0;                                                break;
         default:posy += selmon->mh - CFG_ALT_TAB_MAX_HEIGHT;              break;
        }
        if (m->showbar)
            posy+=bh;

        /* XCreateWindow(display, parent, x, y, width, height, border_width, depth, class, visual, valuemask, attributes); just reference */
        m->tabwin = XCreateWindow(dpy, root, posx, posy, maxwNeeded, maxhNeeded, 2, DefaultDepth(dpy, screen),
                                  CopyFromParent, DefaultVisual(dpy, screen),
                                  CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa); /* create tabwin */
        XMapRaised(dpy, m->tabwin);
        XDefineCursor(dpy, m->tabwin, cursor[CurNormal]->cursor);
    }
    int y = 0;
    int schemecol;
    maxhNeeded/=nwins; /* trunc if not enough*/
    if(maxhNeeded == 0) return; /* too small to render */
    
    for (int i = 0; i < nwins; i++) { /* draw all clients into tabwin */
        c = m->altsnext[i];
        if(!ISVISIBLE(c)) continue;
        schemecol = !(c == selmon->sel) ? SchemeAltTab : SchemeAltTabSelect;
        drw_setscheme(drw, scheme[schemecol]);
        switch(CFG_ALT_TAB_TEXT_POS_X)
        {case 0: txtpad = 0;                                    break;
         case 1: txtpad = MAX(maxwNeeded - namewpxl[i], 0) >> 1;break;
         case 2: txtpad = MAX(maxwNeeded - namewpxl[i], 0);     break;
        }
        drw_text(drw, 0, y, CFG_ALT_TAB_MAX_WIDTH, maxhNeeded, txtpad, cnames[i], 0);
        y += maxhNeeded;
    }
    drw_setscheme(drw, scheme[SchemeNorm]); /* set scheme back to normal */
    drw_map(drw, m->tabwin, 0, 0, CFG_ALT_TAB_MAX_WIDTH, CFG_ALT_TAB_MAX_HEIGHT);
}

void
alttabend()
{
    Monitor *m;

    m = selmon;

    if (selmon->nTabs > 1)
    {
        if (m->altTabN)
        { 
            Client *buff = selmon->altsnext[selmon->altTabN];
            if (selmon->altTabN > 1)
                for (int i = selmon->altTabN; i > 0; i--)
                    selmon->altsnext[i] = selmon->altsnext[i - 1];
            else /* swap em if just 2 clients */
                selmon->altsnext[selmon->altTabN] = selmon->altsnext[0];
            selmon->altsnext[0] = buff;
            /* restacking */
        }
        for (int i = selmon->nTabs - 1; i >= 0; i--) focus(selmon->altsnext[i]);
        arrange(selmon); 
    }

    /* turn off/destroy the window */
    selmon->nTabs = 0;
    free(selmon->altsnext); /* free list of clients */
    XUnmapWindow(dpy, selmon->tabwin);
    XDestroyWindow(dpy, selmon->tabwin);
}


void
tile(Monitor *m)
{
    unsigned int i, n, h, mw, my, ty;
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
    if(!n) return;
    if (n > m->nmaster)
        mw = m->nmaster ? m->ww * m->mfact : 0;
    else
        mw = m->ww;
    for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->snext), i++)
        if (i < m->nmaster) 
        {
            h = (m->wh - my) / (MIN(n, m->nmaster) - i);
            resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
            if (my + HEIGHT(c) < m->wh)
                my += HEIGHT(c);
        } 
        else 
        {
            h = (m->wh - ty) / (n - i);
            resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
            if (ty + HEIGHT(c) < m->wh)
                ty += HEIGHT(c);
        }
}

void
freeicon(Client *c)
{
	if (c->icon)
    {
		XRenderFreePicture(dpy, c->icon);
		c->icon = None;
	}
}

void
unfocus(Client *c, int setfocus)
{
    if (!c) return;
    grabbuttons(c, 0);
    lastfocused = c;
    if (setfocus) {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void
unmanage(Client *c, int destroyed)
{
    --accnum;
    Monitor *m = c->mon;
    XWindowChanges wc;

    detach(c);
    detachstack(c);
    freeicon(c);
    if (!destroyed) {
        wc.border_width = c->oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XSelectInput(dpy, c->win, NoEventMask);
        XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        setclientstate(c, WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    if (lastfocused == c) lastfocused = NULL;
    free(c);
    focus(NULL);
    updateclientlist();
    arrange(m);
}

void
unmapnotify(XEvent *e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if ((c = wintoclient(ev->window))) {
        if (ev->send_event)
            setclientstate(c, WithdrawnState);
        else
            unmanage(c, 0);
    }
}

void
updatebars(void)
{
    Monitor *m;
    XSetWindowAttributes wa = {
        .override_redirect = True, /*patch */
        .background_pixmap = ParentRelative,
        .event_mask = ButtonPressMask|ExposureMask,
    };
    XClassHint ch = {WM_NAME, WM_NAME};
    for (m = mons; m; m = m->next) 
    {
        if (m->barwin) continue;
         m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
                                  CopyFromParent, DefaultVisual(dpy, screen),
                                  CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
        XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
        XMapRaised(dpy, m->barwin);
        XSetClassHint(dpy, m->barwin, &ch);
    }
}

void
updatebarpos(Monitor *m)
{
    m->wy = m->my;
    m->wh = m->mh;
    if (m->showbar) {
        m->wh -= bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh;
        m->wy = m->topbar ? m->wy + bh : m->wy;
    } else m->by = -bh;
}

void
updateclientlist()
{
    Client *c;
    Monitor *m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            XChangeProperty(dpy, root, netatom[NetClientList],
                            XA_WINDOW, 32, PropModeAppend,
                            (unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
    int dirty = 0;
#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int i, j, n, nn;
        Client *c;
        Monitor *m;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo *unique = NULL;

        for (n = 0, m = mons; m; m = m->next, n++);
        /* only consider unique geometries as separate screens */
        unique = ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; i++)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
        XFree(info);
        nn = j;

        /* new monitors if nn > n */
        for (i = n; i < nn; i++) {
            for (m = mons; m && m->next; m = m->next);
            if (m)
                m->next = createmon();
            else
                mons = createmon();
        }
        for (i = 0, m = mons; i < nn && m; m = m->next, i++)
            if (i >= n
                    || unique[i].x_org != m->mx || unique[i].y_org != m->my
                    || unique[i].width != m->mw || unique[i].height != m->mh)
            {
                dirty = 1;
                m->num = i;
                m->mx = m->wx = unique[i].x_org;
                m->my = m->wy = unique[i].y_org;
                m->mw = m->ww = unique[i].width;
                m->mh = m->wh = unique[i].height;
                updatebarpos(m);
            }
        /* removed monitors if n > nn */
        for (i = nn; i < n; i++) {
            for (m = mons; m && m->next; m = m->next);
            while ((c = m->clients)) {
                dirty = 1;
                m->clients = c->next;
                detachstack(c);
                c->mon = mons;
                attach(c);
                attachstack(c);
            }
            if (m == selmon)
                selmon = mons;
            cleanupmon(m);
        }
        free(unique);
    } else
#endif /* XINERAMA */
    {   /* default monitor setup */
        if (!mons)
            mons = createmon();
        if (mons->mw != sw || mons->mh != sh) {
            dirty = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty) {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void
updatemotifhints(Client *c)
{
	Atom real;
	int format;
	unsigned char *p = NULL;
	unsigned long n, extra;
	unsigned long *motif;
	int width, height;

	if (!CFG_DECOR_HINTS) return;

	if (XGetWindowProperty(dpy, c->win, motifatom, 0L, 5L, False, motifatom,
	                       &real, &format, &n, &extra, &p) == Success && p != NULL) {
		motif = (unsigned long*)p;
		if (motif[MWM_HINTS_FLAGS_FIELD] & MWM_HINTS_DECORATIONS) {
			width = WIDTH(c);
			height = HEIGHT(c);

			if (motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_ALL ||
			    motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_BORDER ||
			    motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_TITLE)
				c->bw = c->oldbw = CFG_BORDER_PX;
			else
				c->bw = c->oldbw = 0;

			resize(c, c->x, c->y, width - (2*c->bw), height - (2*c->bw), 0);
		}
		XFree(p);
	}
}

void
updatenumlockmask(void)
{
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PBaseSize) {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    } else
        c->basew = c->baseh = 0;
    if (size.flags & PResizeInc) {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    } else
        c->incw = c->inch = 0;
    if (size.flags & PMaxSize) {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    } else
        c->maxw = c->maxh = 0;
    if (size.flags & PMinSize) {
        c->minw = size.min_width;
        c->minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        c->minw = size.base_width;
        c->minh = size.base_height;
    } else
        c->minw = c->minh = 0;
    if (size.flags & PAspect) {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else
        c->maxa = c->mina = 0.0;
    c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
    c->hintsvalid = 1;
}

void
updatestatus(void)
{
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
    {
        strcpy(stext, WM_NAME);
    }
    drawbar(selmon);
}

void
updatetitle(Client *c)
{
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if (c->name[0] == '\0') /* hack to mark broken clients */
        strcpy(c->name, broken);
}

void
updateicon(Client *c)
{
	freeicon(c);
	c->icon = geticonprop(c->win, &c->icw, &c->ich);
}


void
updatewindowtype(Client *c)
{
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);
    if (state == netatom[NetWMAlwaysOnTop])
    {
        c->alwaysontop = 1;
        c->isfloating = 1;
    }
    if (state == netatom[NetWMFullscreen])
        setfullscreen(c, 1);
    if (wtype == netatom[NetWMWindowTypeDialog])
        c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, c->win))) {
        if (c == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        } else
            c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        if (wmh->flags & InputHint)
            c->neverfocus = !wmh->input;
        else
            c->neverfocus = 0;
        XFree(wmh);
    }
}



Client *
wintoclient(Window w)
{
    Client *c;
    Monitor *m;

    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            if (c->win == w)
                return c;
    return NULL;
}

Monitor *
wintomon(Window w)
{
    int x, y;
    Client *c;
    Monitor *m;

    if (w == root && getrootptr(&x, &y))
        return recttomon(x, y, 1, 1);
    for (m = mons; m; m = m->next)
        if (w == m->barwin)
            return m;
    if ((c = wintoclient(w)))
        return c->mon;
    return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *display, XErrorEvent *ee)
{
    if (ee->error_code == BadWindow
            || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
            || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
            || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
            || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
            || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
            || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
            || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
            || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return xerrorxlib(display, ee); /* may call exit */
}

int
xerrordummy(Display *display, XErrorEvent *ee)
{
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *display, XErrorEvent *ee)
{
    die("WARN: another window manager is already running");
    return -1;
}

int
main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp("-v", argv[1]))
        die(WM_NAME);
    else if (argc != 1)
        die("usage: args[-v]");
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("WARN: no locale support\n", stderr);
    if (!(dpy = XOpenDisplay(NULL)))
        die("FATAL ERROR: CANNOT OPEN DISPLAY");
    checkotherwm();
    setup();

#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1)
        die("pledge");
#endif
    scan();
    restoresession();
    run(); /* main event loop */
    if(RESTART) execvp(argv[0], argv);
    cleanup();
    XCloseDisplay(dpy);
    atexit(crashhandler);
    return EXIT_SUCCESS;
}
/* Screen
 * screen_num = DefaultScreen(display);
 * screen_width = DisplayWidth(display, screen_num);
 * screen_height = DisplayHeight(display, screen_num);
 * root_window_id = RootWindow(display, screen_num);
 * white_pixel_value = WhitePixel(display, screen_num);
 * black_pixel_value = BlackPixel(display, screen_num);
 */

/* Windows  (new window with "Window" )
 * Windows must be created to "exists" and to draw on screen you must map them
 * create_win = 		XCreateWindow(display, parent, x, y, width. height , border_width, depth,
							class, visual, valuemask, attributes );
 * create_simple_win = 	XCreateSimpleWindow(display, parent, x, y, width, height, border_width,
								border, background);
 * map_window = 		XMapWindow(display, win);
 */

/* Events
* XSelectInput(display, win, ExposureMask);
* { ExposureEventMask,  ,NotEventMask,
*
*
*
*
*
*
*/

/* Graphics Context (GC)
 * Gc gc; 		 					GC return handler
 * XGCValues values;  					values assigned to gc
 * unsigned long valuemask 					values in values (settings)
 *
 * gc = XCreateGC(display, win, valuemask, &values); 	creates gc
 * XSetForeground(display, gc, color);  			sets colour of foreground which is usually tied to text colour
 * XSetBackground(display, gc, color); 			sets background color
 * Useless stuff
 *
 * XSetFIllStyle(display, gc, FillType); 			sets fill style,
 * {FillSolid, FillTiled, FillStippled, FIllOpaqueStippled}
 *
 * XSetLineAttributes(display, gc, line_width, line_style, cap_style, join_style)
 * line_style { LineSolid, LineOnOffDash, LineDoubleDash }
 * cap_style { CapNotLast, CapButt, CapRound, CapProjecting }
 * join_style { JoinMiter, JoinRound, JoinBevel }
 *
 * XDrawPoint(display, win, gc, x, y);
 * XDrawLine(display, win, gc, x1, y1, x2, y2); draw from point x1 to x2; y1 to y2
 * XDrawLines(display, win, gc, points, npoints, CoordmodeType)
 * XDrawArc(display, win, gc, x, y, w, h, angle1, angle) x - (w/2) for center && y;
 * XPoint points[] =
	{
		{x, y},
	};
*  npoints = sizeof(points)/sizeof(XPoint);
* XDrawLines(display, win, gc, points, npoints, CoordmodeType)
* XDrawRectangle(display, win, gc, x, y, width, height);
 * XFillRectangle(display, win, gc, ...);
 */
