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
#define BROKEN                  "NOT_SET"
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
    CurResizeTop, CurResizeBottom, CurResizeLeft, CurResizeRight,
    CurMove,
    CurLast
};
/* color schemes */
enum SchemeType
{
    /* system */
    SchemeUrgent, SchemeWarn,                   

    /* border */
    SchemeBorder, SchemeBorderSel,         
    /* alt tab */
    SchemeAltTab, SchemeAltTabSel,           
    /* bar stuff */
    SchemeBarName, SchemeBarSymbol,
    SchemeBarTabSel, SchemeBarTab,   
    SchemeBarTagSel,                            

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
    int i;              /* i -> int             */
    unsigned int ui;    /* ui -> unsigned int   */
    float f;            /* f -> float           */
    const void *v;      /* v -> void pointer    */
};
struct Key
{
    int type;           /* KeyPress/KeyRelease  */
    unsigned int mod;   /* Modifier             */
    KeySym keysym;      /* Key symbol           */
    void (*func)(const Arg *); /* Function      */
    const Arg arg;      /* Argument             */
};
struct Button
{
    int type;           /* ButtonPress/ButtonRelease*/
    unsigned int click; /* enum ClkType             */
    unsigned int mask;  /* Modifier                 */
    unsigned int button;/* Button                   */
    void (*func)(const Arg *arg); /* Function       */
    const Arg arg;      /* Argument                 */
};

struct Client
{
    char name[256]; /* Client Name              */
    float mina;     /* Minimum Aspect           */
    float maxa;     /* Maximum Aspect           */
    int x;          /* X coordinate             */
    int y;          /* Y coordinate             */
    int w;          /* Width                    */
    int h;          /* height                   */
    int oldx;       /* Previous X coordinate    */
    int oldy;       /* Previous Y coordinate    */
    int oldw;       /* Previous Width           */
    int oldh;       /* Previous Height          */
    int basew;      /* Base Width               */
    int baseh;      /* Base Height              */
    int incw;       /* Increment Width          */
    int inch;       /* Increment Height         */
    int maxw;       /* Max Width                */
    int maxh;       /* Max Height               */
    int minw;       /* Minimum Width            */
    int minh;       /* Minimum Height           */
    int bw;         /* Border Width             */
    int oldbw;      /* Old Border Width         */

    unsigned int tags;          /* Tags for clients base 2;
                                 * 2^x;
                                 * {1, 2, 4, 8, 16, 32, 64, 128...}
                                 */
    unsigned int alwaysontop;   /* Keep Client Always on Top;
                                 * ->Flag
                                 */
    unsigned int stayontop;     /* Keep Client Above All Others ;
                                 * stayontop > alwaysontop
                                 * ->Flag
                                 */
    unsigned int hintsvalid;    /* Hints valid;
                                 * ->Flag
                                 */
    unsigned int wasfloating;   /* Was Client Floating;
                                 * ->Flag
                                 */
    unsigned int isfixed;       /* Fixed Client Dimentions;
                                 * minw == maxw;
                                 * minh == maxh
                                 */
    unsigned int isfloating;    /* Is Client Floating;
                                 * ->Flag
                                 */
    unsigned int isurgent;      /* Is Client Urgent;
                                 * ->Flag
                                 */
    unsigned int neverfocus;    /* Never Focus Client;
                                 * ->Flag
                                 */
    unsigned int isfullscreen;  /* Is Client Fullscreen;
                                 * ->Flag
                                 */
    unsigned int hidden;        /* Is the Client not shown for some reason?
                                 * ->Flag
                                 */
    int num;                    /* Client Number        */
    pid_t pid;                  /* Client Pid           */

    unsigned int icw;           /* Icon Width           */
    unsigned int ich;           /* Icon Height          */
    Picture icon;               /* Icon Picture Id      */   
    Client *prev;               /* Previous Client      */
    Client *next;               /* Next Client          */
    Client *sprev;              /* Previous Stack Client*/
    Client *snext;              /* Next Stack Client    */
    Monitor *mon;               /* Client Monitor       */
    Window win;                 /* Client Window        */
};

struct Layout
{
    const char *symbol;
    void (*arrange)(Monitor *);
};

struct Monitor
{
    char ltsymbol[16];      /* Layout symbol                */
    float mfact;            /* Monitor fact                 */
    int nmaster;            /* Master Count                 */
    int num;                /* Monitor Number               */
    int by;                 /* bar geometry                 */
    int mx;                 /* Monitor X (Screen Area)      */
    int my;                 /* Monitor Y (Screen Area)      */
    int mw;                 /* Monitor Width (Screen Area)  */
    int mh;                 /* Monitor Height (Screen Area) */
    int wx;                 /* Monitor X (Window Area)      */
    int wy;                 /* Monitor Y (Window Area)      */
    int ww;                 /* Monitor Width (Window Area)  */
    int wh;                 /* Monitor Height (Window Area) */
    unsigned int isfullscreen;  /* Monitor Fullscreen
                                 * ->Flag
                                 */
    unsigned int showbar;       /* Monitor Showbar
                                 * ->Flag
                                 */
    unsigned int oshowbar;      /* Monitor Old Show Bar 
                                 * ->Flag
                                 */
    unsigned int topbar;        /* Monitor Is Top Bar
                                 * ->Flag
                                 */
    unsigned int seltags;       /* Selected Tags tagset index               */
    unsigned int sellt;         /* Selected lt Layout                       */
    unsigned int tagset[2];     /* Previous And Current Viewable Tag Holder */
    unsigned int cc;            /* Client Counter                           */
    Client *clients;            /* First Client in linked list              */
    Client *clast;              /* Last Client in linked list               */
    Client *stack;              /* First Stack Client in linked list        */
    Client *slast;              /* Last Stack Client in linked list         */
    Client *sel;                /* Selected Client                          */
    Monitor *next;              /* Next Monitor                             */

    Window barwin;              /* Status Bar Window                        */
    Window tabwin;              /* Alt-Tab Window                           */
    const Layout *lt[2];        /* Previous and Current Layout Indexed by sellt*/
};


/* Dont know never used Rules */

struct Rule
{
    const char *class;      /* Initial Name for Window -> WM_CLASS      */
    const char *instance;   /* IDK                                      */
    const char *title;      /* Subwindows/window name changes -> WM_NAME*/
    unsigned int tags;      /* Tags to send to                          */
    int isfloating;         /* Is Floating
                             * ->Flag
                             */
    int monitor;            /* Monitor to send to                       */
};

/* Switches to the next client; Returning the starting point unless "ended" */
Client *alttab(int ended);  
/* Rearranges the original client "tabnext" and the m->sel to be together */
void alttabend(Client *tabnext);
/* Applies User specified rules to specified windows */
void applyrules(Client *c);
/* Applies size hints to specified window */
int  applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
/* Restacks and arranges client visibility in selected monitor */
void arrange(Monitor *m);
/* Restacks and arranges client visibility for all monitors */
void arrangeall(void);
/* Updates status bar symbol and arranges monitor */
void arrangemon(Monitor *m);
/* Attaches client to m->clients linked list */
void attach(Client *c);
/* attaches client to m->stack linked list */
void attachstack(Client *c);
/* Performs a SubstructureRedirectMask check to see if another Window Manager is running */
void checkotherwm(void);
/* cleanups resources when exiting */
void cleanup(void);
/* cleanups monitor resources */
void cleanupmon(Monitor *mon);
/* Cleanups Status Bar */
void cleanupsbar(Monitor *m);
/* Cleanups Alt-Tab Window */
void cleanuptabwin(Monitor *m);
/* Configures (x,y,w,h...) a clients window */
void configure(Client *c);
/* Initializes a Monitor* struct */
Monitor *createmon(void);
/* Removes specified Client from m->clients linked list */
void detach(Client *c);
/* Removes specified Client from m->stack linked list */
void detachstack(Client *c);
/* finds the next monitor based on "dir" AKA Direction */
Monitor *dirtomon(int dir);
/* Checks if a clients window fits in the Window Area */
int docked(Client *c);
/* Checks if a clients window fits in the Window Area Vertically */
int dockedvert(Client *c);
/* Checks if a clients window fits in the Window Area Horizontally */
int dockedhorz(Client *c);
/* Draws Alt-Tab on tabwin and renders it on the screen */
void drawalttab(int first, Monitor *m);
/* Redraws the Status Bar */
void drawbar(Monitor *m);
/* Draws the Status Bar WM_NAME; 
 * Returns X Position of the end of drawn object 
 */
int  drawbarname(Monitor *m);
/* Draws the Status bar on every monitor 
 * Returns X Position of the end of drawn object 
 */
void drawbars(void);
/* Draws the Status Bar Symbol [(M/F/T/G)] 
 * Returns X Position of the end of drawn object 
 */
int  drawbarsym(Monitor *m, int x);
/* Draws the Status Bar Tabs 
 * Returns X Position of the end of drawn object 
 */
int  drawbartabs(Monitor *m, int x, int maxw);
/* Draws Status Bar tags
 * Returns X Position of the end of drawn object 
 */
int  drawbartags(Monitor *m, int x);
/* Gives Focus to specified client */
void focus(Client *c);
/* Frees Picture Icon from specified Client */
void freeicon(Client *c);
/* Returns Atom Id from specified prop */
Atom getatomprop(Client *c, Atom prop);
/* returns the address of the currently selected monitor Layout;
 * getmonlyt(m) -> &layouts[LAYOUT_TYPE]
 */
const Layout *getmonlyt(Monitor *m);
/* returns the address of the previously selected monitor Layout;
 * getmonlyt(m) -> &layouts[LAYOUT_TYPE]
 */
const Layout *getmonolyt(Monitor *m);

/* Returns A Picture prop with specified w/h */
Picture geticonprop(Window win, unsigned int *picw, unsigned int *pich);
/* Assigns specified *x, *y root pointer coordinates;
 * Returns XStatus
 */
int  getrootptr(int *x, int *y);
const unsigned int gettagshift(int tagnum);
/* returns specified Window state; */
long getstate(Window w);
/* Assigns char *text with specified sizeof(text) from atom textprop;
 * Returns 1 on Success;
 * Returns 0 on Failure;
 */
int  gettextprop(Window w, Atom atom, char *text, unsigned int size);
/* Grabs a buttons from config.h (struct buttons);
 * if not "focused" grab clients buttons;
 * Updatesnumlockmask;
 */
void grabbuttons(Client *c, int focused);
/* Grabs keyboard keys from config.h (struct keys)
 * Updatesnumlockmask;
 */
void grabkeys(void);
/* Rearranges the monitor layout to grid */
void grid(Monitor *m);
/* Kills a specified client with specified type;
 * {Destroy, Safedestroy, Graceful} */
void killclient(Client *c, int type);
/* manages AKA Initializes New Clients based on specified window and PROVIDED XWindowAttributes */
Client *manage(Window w, XWindowAttributes *wa);
/* maximizes given client */
void maximize(Client *c);
/* maximizes vertically givent client */
void maximizevert(Client *c);
/* maximizes horizontally givent client */
void maximizehorz(Client *c);
/* Rearranges the monitor layout to monocle */
void monocle(Monitor *m);
/* Returns next Non-Floating and VISIBLE Client;
 * returns NULL if none found
 */
Client *nexttiled(Client *c);
/* Returns next VISIBLE Client;
 * returns NULL if non found
 */
Client *nextvisible(Client *c);
/* Moves Client to start of linked list Focuses it and arranges monitor
 * This causes Client to be master in Tiled Mode and gives focus
 */
void pop(Client *c);
/* Sets running to 0 terminating run() and exiting */
void quit(void);
/* Attempts to restore session from SESSION_FILE for all monitors */
void restoresession(void);
/* Attempts to restore session from SESSION_FILE for given monitor */
void restoremonsession(Monitor *m);
/* Searches through every monitor for a possible big enough size to fit rectangle parametors specified */
Monitor *recttomon(int x, int y, int w, int h);
/* resize a client only if specified x/y/w/h is different 
 * interact
 * {1, 0}
 * 1 -> dont confide resize to monitor dimentions 
 * 0 -> confide resize within monitor dimentions
 * */
void resize(Client *c, int x, int y, int w, int h, int interact);
/* resize a client given parametors without sizehints */
void resizeclient(Client *c, int x, int y, int w, int h);
/* Reorders(restacks) clients in current m->stack */
void restack(Monitor *m);
/* Flags RESTART and sets running to 0;
 * results in execvp(self) and "restarts"
 */
void restart(void);
/* Main event loop */
void run(void);
/* Attemps to save session in SESSION_FILE for every monitor */
void savesession(void);
/* Attemps to restore session from SESSION_FILE for specified Monitor */
void savemonsession(Monitor *m);
/* Scans for new clients on startup */
void scan(void);
/* Sends a Protocl Event to specified client */
int  sendevent(Client *c, Atom proto);
/* Moves a Client to specified Monitor */
void sendmon(Client *c, Monitor *m);
/* Sets a Clients _NET_WM_STATE based on specified state */
void setclientstate(Client *c, long state);
/* Sets the current desktop */
void setdesktop(void);
/* Sets the desktop names from "tags" in config.h */
void setdesktopnames(void);
/* Sets the Max destop number count based on LENGTH(tags) */
void setdesktopnum(void);
/* Sets a Client to be floating based on isfloating */
void setfloating(Client *c, int isfloating);
/* Sets raw focus (No checks) to specified Client */
void setfocus(Client *c);
/* Sets _NET_WM_STATE_FULLSCREEN to specified based on fullscreen */
void setfullscreen(Client *c, int fullscreen);
/* Sets monitor layout based on enum Layout;
 * {Tiled, Floating, Monocle, Grid} 
 */
void setmonlyt(Monitor *m, int layout);
/* Sets m->showbar based on specified "show" */
void setshowbar(Monitor *m, int show);
/* Sets _NET_WM_STATE_STICKY to specified Client based on "sticky" */
void setsticky(Client *c, int sticky);
/* Sets up Variables, Checks, WM specific data, etc.. */
void setup(void);
/* Setups Cursors and CursorImages */
void setupcur(void);
/* Setups Memory pool for Clients */
void setuppool(void);
/* Setups Tags and TagsColours */
void setuptags(void);
/* Setups timezone */
void setuptimezone(void);
/* Sets a Clients Urgency flag based on urg */
void seturgent(Client *c, int urg);
/* Sets Current Desktop Viewport */
void setviewport(void);
/* Moves Client offscreen if not VISIBLE;
 * Moves Client onscreen if VISIBLE;
 */
void showhide(Client *c);
/* Unmaps Client if not VISIBLE;
 * Maps Client if VISIBLE 
 */
void showhidemap(Client *c);
/* waits for childs (zombies) to die */
void sigchld(int signo);
/* Handles Signals and how we use them */
void sighandler(void);
/* Calls restart */
void sighup(int signo); 
/* Calls quit */
void sigterm(int signo); 
/* Rearranges the monitor layout to Tiled */
void tile(Monitor *m);
/* Unfocuses specified client and sets to focus to root if setfocus is true */
void unfocus(Client *c, int setfocus);
/* Unmanages Client AKA we dont tell it what todo Nor does it use our resources;
 * And perform checks based on specified "destroyed";
 * 1 -> widthdraw window;
 * 0 -> skip checks (window already destroyed)
 */
void unmanage(Client *c, int destroyed);
/* updates the Status Bar Position from given monitor */
void updatebarpos(Monitor *m);
/* Redraws all bars in all monitors */
void updatebars(void);
/* Updates _NET_WM_CLIENT_LIST */
void updateclientlist(void);
/* Updates the Current destop */
void updatedesktop(void);
/* Updates Geometry for external monitors based on if they have different geometry */
int  updategeom(void);
/* Updates Motif hints from specified Client */
void updatemotifhints(Client *c);
/* checks and updates mask if numlock is active */
void updatenumlockmask(void);
/* Updates a Clients sizehints */
void updatesizehints(Client *c);
/* Updates the WM_NAME status;
 * if none found default to config.h WM_NAME
 */
void updatestatus(void);
/* Updates Client tile if we find one;
 * if none found default to dwm.h BROKEN
 */
void updatetitle(Client *c);
/* Updates the Client icon if we find one */
void updateicon(Client *c);
/* Updates Our own states based on Client state specified */
void updatewindowstate(Client *c, Atom state, int data);
/* Updates Our own states based on windowtype in Client */
void updatewindowtype(Client *c);
/* Updates WM_HINTS for specified Client */
void updatewmhints(Client *c);
/* Sets the window _WM_STATE based on state specified */
void winsetstate(Window win, long state);
/* Maps a window without sending an XEvent */
void winmap(Client *c, int deiconify);
/* Unmaps a window without sending an XEvent */
void winunmap(Window win, Window winroot, int iconify);
/* Attemps to find Client form specified window */
Client  *wintoclient(Window w);
/* Attemps to find what Monitor specified window is on */
Monitor *wintomon(Window w);

/* Xerror handler */
int  xerror(Display *display, XErrorEvent *ee);
/* Xerror Dummy handler */
int  xerrordummy(Display *display, XErrorEvent *ee);
/* Xerror startup Failure handler */
int  xerrorstart(Display *display, XErrorEvent *ee);
/* Xexit handler */
int  xexithandler(Display *display);

#endif
