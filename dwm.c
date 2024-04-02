/* Se LICENSE file for copyright and license details.
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
* ~Caveats, this version of dwm is bloated relative to the main branch of dwm -> https://git.suckless.org/dwm/
* ~So you should not expect this to be able to run as well as dwm
* ~Tested on a i7-4790 16G ddr3, integrated graphics, -> WITH an ssd <-
*
* When re-drawing using drw make sure to clear it by setting scheme + drw_rect(area you want)
* AND that the drw GC starts at 0,0 so perform your drw actions relative to 0,0
*/
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h> //config.h multimedia keys
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* threading */
#include <pthread.h>

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include <time.h> /* ALT TAB */

/* self contained */
#include "drw.h"
#include "util.h"
#include "pool.h"
#include "winutil.h"
#include "toggle.h" 
#include "events.h"
/* dwm */
#include "dwm.h"
#include "config.h"
#include "keybinds.h"

/* Window limit occurs of 23 on 15 for some reason????? */
#if CFG_GAP_PX == 15
#define CFG_GAP_PX 16
#endif

/* extern var declarations */

int running = 1;
int RESTART = 0;
char stext[256];       
int screen = 0;
int sw = 0, sh = 0;     
int bh = 0;             
int lrpad = 0;          
int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask = 0;
unsigned int accnum = 0; 
Atom wmatom[WMLast], motifatom;
Atom netatom[NetLast];
Cur *cursor[CurLast];
Clr **scheme = NULL;
Clr **tagscheme = NULL;
Display *dpy = NULL;
Drw *drw = NULL;
Monitor *mons, *selmon = NULL;
Window root = 0, wmcheckwin = 0;
Client *lastfocused = NULL;
Pool *pl = NULL;

Client *hashedwins[CFG_MAX_CLIENT_COUNT];

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags
{
    char limitexceeded[LENGTH(tags) > 31 ? -1 : 1];
};

Client *
alttab(int ended)
{
    Monitor *m = selmon;
    static Client *c = NULL, *tabnext = NULL;
    if(ended) tabnext = m->sel;
    if(ended) c = tabnext;
    if(!c) c = nextvisible(m->clients);

    /* get the next visible window */
    c = nextvisible(c ? c->next : NULL);
    if(!c) c = nextvisible(m->clients);
    focus(c);
    if(CFG_ALT_TAB_MAP_WINDOWS)
    {   arrange(m);
    }
    return tabnext;
}

void
alttabend(Client *tabnext)
{
    Client *c;
    Monitor *m;

    m = selmon;
    c = m->sel;

    if(!CFG_ALT_TAB_FIXED_TILE)
    {
        if(tabnext) 
        {
            detach(tabnext);
            attach(tabnext);
        }
        if(c)
        {
            detach(c);
            attach(c);
        }
    }
    focus(tabnext);
    focus(c);
    arrange(m);
    XUnmapWindow(dpy, selmon->tabwin);
    XDestroyWindow(dpy, selmon->tabwin);
}

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
    class    = ch.res_class ? ch.res_class : BROKEN;
    instance = ch.res_name  ? ch.res_name  : BROKEN;

    for (i = 0; i < LENGTH(rules); i++)
    {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
                && (!r->class || strstr(class, r->class))
                && (!r->instance || strstr(instance, r->instance)))
        {
            c->isfloating = r->isfloating;
            c->tags |= r->tags;
            for (m = mons; m && m->num != r->monitor; m = m->next);
            if (m) c->mon = m;
        }
    }
    if (ch.res_class) XFree(ch.res_class);
    if (ch.res_name)  XFree(ch.res_name);
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
    if (interact)
    {
        if (*x > sw) *x = sw - WIDTH(c);
        if (*y > sh) *y = sh - HEIGHT(c);
        if (*x + *w + (c->bw << 1) < 0) *x = 0;
        if (*y + *h + (c->bw << 1) < 0) *y = 0;
    }
    else
    {
        if (*x >= m->wx + m->ww) *x = m->wx + m->ww - WIDTH(c);
        if (*y >= m->wy + m->wh) *y = m->wy + m->wh - HEIGHT(c);
        if (*x + *w + (c->bw << 1) <= m->wx) *x = m->wx;
        if (*y + *h + (c->bw << 1) <= m->wy) *y = m->wy;
    }
    if (*h < bh) *h = bh;
    if (*w < bh) *w = bh;
    if (CFG_RESIZE_HINTS || c->isfloating ||  !c->mon->lt[c->mon->sellt]->arrange)
    {
        if (!c->hintsvalid) updatesizehints(c);
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = c->basew == c->minw && c->baseh == c->minh;
        /* temporarily remove base dimensions */
        if (!baseismin)
        {
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for aspect limits */
        if (c->mina > 0 && c->maxa > 0)
        {
            if (c->maxa < (float)*w / *h) *w = *h * c->maxa + 0.5;
            else if (c->mina < (float)*h / *w) *h = *w * c->mina + 0.5;
        }
        /* increment calculation requires this */
        if (baseismin)
        {
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for increment value */
        if (c->incw) *w -= *w % c->incw;
        if (c->inch) *h -= *h % c->inch;
        /* restore base dimensions */
        *w = MAX(*w + c->basew, c->minw);
        *h = MAX(*h + c->baseh, c->minh);
        if (c->maxw) *w = MIN(*w, c->maxw);
        if (c->maxh) *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
    Client *c;
    for(c = m->stack; c; c = c->snext) showhide(c);
    arrangemon(m);
    restack(m);
}

void
arrangeall(void)
{
    Monitor *m;
    Client *c;
    for(m = mons; m; m = m->next)
    {
        for(c = m->stack; c; c = c->snext) showhide(c);
        arrangemon(m);
        restack(m);
    }
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
    Monitor *restrict m = c->mon;
    c->next = m->clients;
    m->clients = c;
    if(c->next) c->next->prev = c;
    else m->clast = c;
    /* prevent circular linked list */
    c->prev = NULL;
}

void
attachstack(Client *c)
{
    Monitor *restrict m = c->mon;
    c->snext = c->mon->stack;
    c->mon->stack = c;
    if(c->snext) c->snext->sprev = c;
    else m->slast = c;
    /* prevent circular linked list */
    c->sprev = NULL;
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
    alttabend(NULL);
    View(&a);
    selmon->lt[selmon->sellt] = &foo;
    for (m = mons; m; m = m->next)
        while (m->stack)
            unmanage(m->stack, 0);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons) cleanupmon(mons);
    for (i = 0; i < CurLast; ++i) drw_cur_free(drw, cursor[i]);
    for (i = 0; i < LENGTH(colors); ++i) free(scheme[i]);
    free(scheme);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    pooldestroy(pl);
}

void
cleanupmon(Monitor *mon)
{
    Monitor *m;

    if (mon == mons) mons = mons->next;
    else
    {
        for (m = mons; m && m->next != mon; m = m->next);
        m->next = mon->next;
    }
    cleanupsbar(mon);
    cleanuptabwin(mon);
    free(mon);
}

void
cleanupsbar(Monitor *m) /* status bar */
{
    XUnmapWindow(dpy, m->barwin);
    XDestroyWindow(dpy, m->barwin);
}

void
cleanuptabwin(Monitor *m)
{
    XUnmapWindow(dpy, m->tabwin);
    XDestroyWindow(dpy, m->tabwin);
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

Monitor *
createmon(void)
{
    Monitor *m;

    m = ecalloc(1, sizeof(Monitor));
    m->tagset[0]= m->tagset[1] = 1;
    m->mfact    = CFG_MONITOR_FACT;
    m->nmaster  = CFG_MASTER_COUNT;
    m->showbar  = CFG_SHOW_BAR;
    m->oshowbar = !CFG_SHOW_BAR;
    m->topbar   = CFG_TOP_BAR;
    m->lt[0]    = &layouts[CFG_DEFAULT_LAYOUT];
    m->lt[1]    = &layouts[CFG_DEFAULT_PREV_LAYOUT];
    m->isfullscreen = 0;
    /* prevent garbage values (undefined behaviour) */
    m->clients  = NULL;
    m->stack    = NULL;
    m->next     = NULL;
    m->clast    = NULL;
    m->slast    = NULL;
    m->sel      = NULL;
    strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
    return m;
}

void
detach(Client *c)
{
    Monitor *m = c->mon;
    Client **tc;
    for (tc = &m->clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
    if(!(*tc)) 
    {
        m->clast = c->prev;
        return;
    }
    else if(c->next) 
    {   c->next->prev = c->prev;
    }
    else if(c->prev) 
    {
        m->clast = c->prev;
        c->prev->next = NULL;
    }
}

void
detachstack(Client *c)
{
    Monitor *m = c->mon;
    Client **tc, *t;

    for (tc = &m->stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;
    if(!(*tc)) m->slast = c->sprev;
    else if(c->snext) c->snext->sprev = c->sprev;
    else if(c->sprev)
    {
        m->slast = c->sprev;
        c->sprev->snext = NULL;
    }
    if (c == m->sel)
    {
        for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
        m->sel = t;
    }
}

Monitor *
dirtomon(int dir)
{
    Monitor *m = NULL;

    if (dir > 0)
    {   if (!(m = selmon->next)) m = mons;
    }
    else if (selmon == mons)
    {   for (m = mons; m->next; m = m->next);
    }
    else 
    {   for (m = mons; m->next != selmon; m = m->next);
    }
    return m;
}

inline int
docked(Client *c)
{
    /* & better inling for some reason idk */
    /* This is called alot of times specifically in restack() so performance does somewhat matter */
    return dockedvert(c) & dockedhorz(c);
}

inline int 
dockedvert(Client *c)
{
    return (c->mon->wy == c->y) & (c->mon->wh == HEIGHT(c));
}

inline int 
dockedhorz(Client *c)
{
    return (c->mon->wx == c->x) & (c->mon->ww == WIDTH(c));
}

/* Draws a specified Picture onto drw->picture */
static void
drw_pic(Drw *drwstruct, int x, int y, unsigned int w, unsigned int h, Picture pic)
{
    if (!drw)
        return;
    XRenderComposite(drw->dpy, PictOpOver, pic, None, drw->picture, 0, 0, 0, 0, x, y, w, h);
}

/* resizes the icon if too big using chatgpt (it works, mostly) */
static Picture
drw_picture_create_resized(Drw *drwstruct, char *src, unsigned int srcw, unsigned int srch, unsigned int dstw, unsigned int dsth) 
{
    Pixmap pm;
    Picture pic;
    GC gc;
    if (!(srcw <= (dstw << 1u) && srch <= (dsth << 1u)))
    {   
        double widthratio = (double)srcw / (double)dstw;
        double heightratio = (double)srch / (double)dsth;
        for(int y = 0; y < dsth; ++y)
        {
            break;
            for(int x = 0; x < dstw; ++x)
            {   
                int originalx = (char)(x * widthratio);
                int originaly = (char)(y * heightratio);

                int originalindex = originaly * srcw + originalx;
                if(srcw * srch - 1 >= y * dstw + x && originalindex <= srcw * srch - 1)
                {   src[y * dstw + x] = src[originalindex];
                }
            }
        }
    }
    XImage img =
    {
        srcw, srch, 0, ZPixmap, src,
        ImageByteOrder(drw->dpy), BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 32,
        32, 0, 32,
        0, 0, 0,
    };
    XInitImage(&img);

    pm = XCreatePixmap(drw->dpy, drw->root, srcw, srch, 32);
    gc = XCreateGC(drw->dpy, pm, 0, NULL);
    XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, srcw, srch);
    XFreeGC(drw->dpy, gc);

    pic = XRenderCreatePicture(drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32), 0, NULL);
    XFreePixmap(drw->dpy, pm);

    XRenderSetPictureFilter(drw->dpy, pic, FilterBilinear, NULL, 0);
    XTransform xf;
    xf.matrix[0][0] = (srcw << 16u) / dstw;
    xf.matrix[0][1] = 0;
    xf.matrix[1][0] = 0;
    xf.matrix[1][1] = (srch << 16u) / dsth;
    xf.matrix[1][2] = 0;
    xf.matrix[0][2] = 0;
    xf.matrix[2][0] = 0;
    xf.matrix[2][1] = 0;
    xf.matrix[2][2] = 65536;
    XRenderSetPictureTransform(drw->dpy, pic, &xf);
    return pic;
}

void
drawalttab(int first, Monitor *m)
{
    Client *c;
    int maxhNeeded; /*max height needed to draw alt-tab*/
    int maxwNeeded; /*see above (width)*/
    int txtpad;     /*text padding */
    int nwins;

    maxwNeeded = 0;
    txtpad = 0;
    nwins = 0;

    for(c = m->clients; c; c = c->next)
    {
        if(!ISVISIBLE(c)) continue;
        maxwNeeded = MAX((TEXTW(c->name) - lrpad), maxwNeeded);
        ++nwins;
    }
    if(!nwins)
    {   return;
    }

    maxwNeeded = MIN(maxwNeeded + CFG_ALT_TAB_MIN_WIDTH, CFG_ALT_TAB_MAX_WIDTH);
    maxhNeeded = MIN(lrpad * nwins, CFG_ALT_TAB_MAX_HEIGHT); /* breaks with too many clients MAX is not recommended */

    if (first)
    {
        m = selmon;
        XSetWindowAttributes wa =
        {
            .override_redirect = True,
            .border_pixel = 0,
            .backing_store = WhenMapped,
            .save_under = False,
            .event_mask = ButtonPressMask|ExposureMask
        };

        /* decide position of tabwin */
        int posx = selmon->mx;
        int posy = selmon->my;
        if(CFG_ALT_TAB_POS_X == 0) posx += 0;
        if(CFG_ALT_TAB_POS_X == 1) posx += (selmon->mw >> 1) - (maxwNeeded >> 1);
        if(CFG_ALT_TAB_POS_X == 2) posx += selmon->mw - maxwNeeded;

        if(CFG_ALT_TAB_POS_Y == 0) posy += selmon->mh - maxhNeeded;
        if(CFG_ALT_TAB_POS_Y == 1) posy += (selmon->mh >> 1) - maxhNeeded;
        if(CFG_ALT_TAB_POS_Y == 2) posy += 0;

        m->tabwin = XCreateWindow(dpy, root, posx, posy, maxwNeeded, maxhNeeded, 0, DefaultDepth(dpy, screen),
                                  None, DefaultVisual(dpy, screen),
                                  CWOverrideRedirect|CWBackPixmap|CWEventMask|CWBorderPixel|CWSaveUnder, &wa); 
        XMapRaised(dpy, m->tabwin);
        XDefineCursor(dpy, m->tabwin, cursor[CurNormal]->cursor);
    }
    int y = 0;
    int schemecol;
    int txtw = 0;
    maxhNeeded/=nwins;
    for(c = m->clients; c; c = c->next)
    {
        if(!ISVISIBLE(c)) continue;
        schemecol = c != selmon->sel ? SchemeAltTab : SchemeAltTabSel;
        /* for some reason this breaks when not using a variable and just putting in the thing */
        txtw = TEXTW(c->name) - lrpad;
        drw_setscheme(drw, scheme[schemecol]);

        if(CFG_ALT_TAB_TEXT_POS_X == 0) txtpad = 0;
        if(CFG_ALT_TAB_TEXT_POS_X == 1) txtpad = MAX(maxwNeeded - txtw, 0) >> 1;
        if(CFG_ALT_TAB_TEXT_POS_X == 2) txtpad = MAX(maxwNeeded - txtw, 0);

        drw_text(drw, 0, y, maxwNeeded, maxhNeeded, txtpad, c->name, 0);
        y += maxhNeeded;
    }
    drw_setscheme(drw, scheme[SchemeBorder]); /* set scheme back to normal */
    drw_map(drw, m->tabwin, 0, 0, CFG_ALT_TAB_MAX_WIDTH, CFG_ALT_TAB_MAX_HEIGHT);
}

void
drawbar(Monitor *m)
{
    int x, w, tw;
    x = w = tw = 0;
    if (!m->showbar) return;
    /* draw status first so it can be overdrawn by tags later */
    tw = drawbarname(m);
    x = drawbartags(m, x);
    x = drawbarsym(m, x);
    x = drawbartabs(m, x, m->ww - tw - x);
    drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

int
drawbarname(Monitor *m)
{
    const int tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
    if (CFG_SHOW_WM_NAME && m == selmon)  /* only draw on selmon */
    {
        drw_setscheme(drw, scheme[SchemeBarName]);
        drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
        return tw;
    }
    return 0;
}

void
drawbars(void)
{
    for (Monitor *m = mons; m; m = m->next) drawbar(m);
}

int
drawbarsym(Monitor *m, int x)
{
    drw_setscheme(drw, scheme[SchemeBarSymbol]);
    return drw_text(drw, x, 0, TEXTW(m->ltsymbol), bh, lrpad >> 1, m->ltsymbol, 0);
}

int
drawbartabs(Monitor *m, int x, int maxw)
{
    Client *c;
    unsigned int tabcnt;     /* tab count                    */
    unsigned int tabsz;      /* tab size                     */
    unsigned int iconspace;
    unsigned int boxh;       /* tiny floating box indicator h*/
    unsigned int boxw;       /* tiny floating box indicator w*/
    unsigned int curscheme;  /* current scheme               */
    unsigned int cc;         /* client counter               */
    unsigned int btpos;      /* current bartab positon x     */
    int y;
    int h;

    #define MAX_BUF_SIZE 256 + 1    /* clients name should NEVER exceed this amount as set in the client struct */
    static char txt[MAX_BUF_SIZE];
    int boundscheck = 0;

    btpos = cc = tabcnt = y = 0;
    h = bh;
    boxh = bh >> 2;
    boxw = bh >> 2;
    curscheme = SchemeBarTab ; /* init curscheme */
    /* set default scheme (blank canvas)*/
    drw_setscheme(drw, scheme[curscheme]);
    drw_rect(drw, x, 0, maxw, h, 1, 1);
    for(c = m->clients; c; c = c->next) tabcnt += !!ISVISIBLE(c) && !c->hidden;
    /* exit if no clients selected */
    if(!tabcnt) return x;
    tabsz = maxw / tabcnt;
    /* draw only selmon->sel if tabs to small */
    if(tabsz < (unsigned int)TEXTW("...") - lrpad)
    {
        iconspace = m->sel->icon ? m->sel->icw + CFG_ICON_SPACE : (unsigned int)lrpad >> 1;
        drw_text(drw, x, y, maxw, h, iconspace, m->sel->name, 0);
        if(m->sel->icon)
        {   drw_pic( drw, x, y + ((h- m->sel->ich) >> 1), m->sel->icw, m->sel->ich, m->sel->icon);
        }
        if(m->sel->isfloating)
        {   drw_rect(drw, x, y + boxh, boxw, boxw, m->sel->isfixed, 0);
        }
        return x + iconspace + TEXTW(m->sel->name);
    }

    /* draw rest of them */
    for(c = m->clients; c; c = c->next)
    {
        memset(txt, 0, MAX_BUF_SIZE);
        if(!ISVISIBLE(c) || c->hidden) continue;
        if(c->isurgent)
        {   curscheme = SchemeUrgent;
        }
        else if (c == m->sel)
        {   curscheme = SchemeBarTabSel;
        }
        else
        {   curscheme = SchemeBarTab;
        }
        iconspace = c->icon ? c->icw + CFG_ICON_SPACE : (unsigned int)lrpad >> 1;
        drw_setscheme(drw, scheme[curscheme]);

        /* TAGMASK is a mask of EVERY tag so we check for it to see if sticky */
        boundscheck = snprintf(txt, MAX_BUF_SIZE, "%s%s", (c->tags == TAGMASK) ? "*" : "", c->name);
        /* only print text if we pass check */
        if(!(boundscheck < 0 || boundscheck >= MAX_BUF_SIZE))
        {   drw_text(drw, x + btpos, y, tabsz, h, iconspace, txt, 0);
        }

        if(c->icon)
        {   drw_pic(drw, x + btpos, y + ((h - c->ich) >> 1), c->icw, c->ich, c->icon);
        }
        btpos += tabsz;
    }
    return x + btpos + tabsz;
}

int
drawbartags(Monitor *m, int x)
{
    Client *c;
    const int boxw = (lrpad >> 4) + 2;
    int i;
    int w;
    int occ;
    int urg;
    int tagselected;

    tagselected = w = i = occ = urg = 0;

    for (c = m->clients; c; c = c->next)
    {
        occ |= c->tags;
        if (c->isurgent) urg |= c->tags;
    }

    for (i = 0; i < LENGTH(tags); ++i)
    {
        if(CFG_SEL_TAG_INDICATOR)   /* 2^x (including 2^0) so we compare our cur tag to the tag order */
        {   tagselected = m == selmon && selmon->tagset[selmon->seltags] & (1 << i);
        }
        else
        {   tagselected = m == selmon && selmon->sel && selmon->sel->tags & (1 << i);
        }
        w = TEXTW(tags[i]);
        if(tagselected)
        {   drw_setscheme(drw, scheme[SchemeBarTagSel]);
        }
        else
        {   drw_setscheme(drw, tagscheme[i]);
        }
        drw_text(drw, x, 0, w, bh, lrpad >> 1, tags[i], urg & 1 << i);
        if (occ & (1 << i))
        {
            drw_rect(drw, x + boxw, MAX(bh - boxw, 0), w - ((boxw << 1) + 1), boxw,
                     m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
                     urg & 1 << i);
        }
        x += w;
    }
    return x;
}

void
focus(Client *c)
{
    if (!c || !ISVISIBLE(c))
        for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
    if (selmon->sel && selmon->sel != c)
        unfocus(selmon->sel, 0);
    if (c)
    {
        if (c->mon != selmon) selmon = c->mon;
        if (c->isurgent) 
        {   seturgent(c, 0);
        }
        detachstack(c);
        attachstack(c);
        grabbuttons(c, 1);
        /* set new focused border first to avoid flickering */
        XSetWindowBorder(dpy, c->win, scheme[SchemeBorderSel][ColBorder].pixel);
        /* lastfocused may be us if another window was unmanaged */
        if (lastfocused && lastfocused != c)
        {   XSetWindowBorder(dpy, lastfocused->win, scheme[SchemeBorder][ColBorder].pixel);
        }
        setfocus(c);
    }
    else
    {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }

    selmon->sel = c;
    drawbars();
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

inline const Layout *
getmonlyt(Monitor *m)
{   return m->lt[m->sellt];
}

inline const Layout *
getmonolyt(Monitor *m)
{   return m->lt[!m->sellt];
}

/* applies prealpha to Picture image data point */
static uint32_t
prealpha(uint32_t p) 
{
    uint8_t a = p >> 24u;
    uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
    uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
    return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
}

Picture
geticonprop(Window win, unsigned int *picw, unsigned int *pich)
{
    int format;
    int bitformat = 32;
    unsigned int unitbytecount = 16384;

    unsigned long n, extra;
    unsigned long *p= NULL;
    Atom real;
    if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType,
                    &real, &format, &n, &extra, (unsigned char **)&p) != Success) return None;
    if (n == 0 || format != bitformat) 
    {
        XFree(p);
        return None;
    }
    unsigned long *bstp = NULL;
    uint32_t w, h, sz;
    {
        unsigned long *i;
        const unsigned long *end = p + n;
        uint32_t bstd = UINT32_MAX, d, m;
        for (i = p; i < end - 1; i += sz)
        {
            if ((w = *i++) >= unitbytecount  || (h = *i++) >= unitbytecount)
            {
                XFree(p);
                return None;
            }
            if ((sz = w * h) > end - i) break;
            if ((m = w > h ? w : h) >= CFG_ICON_SIZE && (d = m - CFG_ICON_SIZE) < bstd) 
            {
                bstd = d;
                bstp = i;
            }
        }
        if (!bstp)
        {
            for (i = p; i < end - 1; i += sz)
            {
                if ((w = *i++) >= unitbytecount || (h = *i++) >= unitbytecount ) 
                {
                    XFree(p);
                    return None;
                }
                if ((sz = w * h) > end - i) break;
                if ((d = CFG_ICON_SIZE - (w > h ? w : h)) < bstd) 
                {
                    bstd = d;
                    bstp = i;
                }
            }
        }
        if (!bstp) 
        {
            XFree(p);
            return None;
        }
    }

    if ((w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) 
    {
        XFree(p);
        return None;
    }

    uint32_t icw = 0, ich = 0;
    if (w <= h)
    {
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
    *picw = icw;
    *pich = ich;

    uint32_t i, *bstp32 = (uint32_t *)bstp;
    for (sz = w * h, i = 0; i < sz; ++i) bstp32[i] = prealpha(bstp[i]);

    Picture ret = drw_picture_create_resized(drw, (char *)bstp, w, h, icw, ich);
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
    if (name.encoding == XA_STRING)
    {   strncpy(text, (char *)name.value, size - 1);
    }
    else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list)
    {
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
    unsigned int i, j;
    unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    if (!focused)
        XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
                BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
    for (i = 0; i < LENGTH(buttons); i++)
    {
        if (buttons[i].click == ClkClientWin)
        {
            for (j = 0; j < LENGTH(modifiers); ++j)
            {
                XGrabButton(dpy, buttons[i].button,
                        buttons[i].mask | modifiers[j],
                        c->win, False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            }
        }
    }
}

void
grabkeys(void)
{
    updatenumlockmask();
    unsigned int i, j, k;
    unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
    int start, end, skip;
    KeySym *syms;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    XDisplayKeycodes(dpy, &start, &end);
    syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);

    if (!syms) return;
    for (k = start; k <= (unsigned int)end; k++)
    {
        for (i = 0; i < LENGTH(keys); i++)
        {
            /* skip modifier codes, we do that ourselves */
            if (keys[i].keysym == syms[(k - start) * skip])
            {
                for (j = 0; j < LENGTH(modifiers); j++)
                {
                    XGrabKey(dpy, k,
                            keys[i].mod | modifiers[j],
                            root, True,
                            GrabModeAsync, GrabModeAsync);
                }
            }
        }
    }
    XFree(syms);
}


void
grid(Monitor *m) 
{
    unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
    int tmpcw;
    int tmpch;
    Client *c;
    for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
        n++;
    if(!n) 
    {   return;
    }
    /* grid dimensions */
    for(rows = 0; rows <= (n >> 1); ++rows)
        if(rows * rows >= n)
            break;

    cols = rows - !!(rows && (rows - 1) * rows >= n);
    /* window geoms (cell height/width) */
    ch = m->wh / (rows + !rows);
    cw = m->ww / (cols + !cols);
    for(i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
    {
        cx = m->wx + (i / rows) * cw;
        cy = m->wy + (i % rows) * ch;
        /* adjust height/width of last row/column's windows */
        ah = !!((i + 1) % rows) * (m->wh - ch * rows);
        aw = !!(i >= rows * (cols - 1)) * (m->ww - cw * cols);

        /* CFG_GAP_PX without fucking everything else */
        cx += CFG_GAP_PX;
        cy += CFG_GAP_PX;

        tmpcw = cw - (c->bw << 1) + aw;
        tmpch = ch - (c->bw << 1) + ah;

        tmpcw -= CFG_GAP_PX;
        tmpch -= CFG_GAP_PX;

        tmpcw -= !!aw * CFG_GAP_PX;
        tmpch -= !ah * CFG_GAP_PX;

        resize(c, cx, cy, tmpcw, tmpch, 0);
        ++i;
    }
}

void
killclient(Client *c, int type)
{
    const int probablynotsystempid = 100;
    if(!c) return;
    if(!sendevent(c, wmatom[WMDelete]))
    {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        switch(type)
        {
        case Graceful:
            XKillClient(dpy, c->win);
            break;
        case Destroy:
            XDestroyWindow(dpy, c->win);
            XSync(dpy, False);
            XSetErrorHandler(xerror);
            XUngrabServer(dpy);
#if CFG_STORE_PID && CFG_ALLOW_PID_KILL
            if(c && c->win)
            {
                if(c->pid > probablynotsystempid || (c->pid = XGetPid(dpy, c->win)) > probablynotsystempid)
                {   
                    debug("Destroy Killed: %d", c->pid);
                    kill(SIGTERM, c->pid);
                }
            }
            XSync(dpy, False);
            if(c && c->win)
            {
                if(c->pid > probablynotsystempid || (c->pid = XGetPid(dpy, c->win)) > probablynotsystempid)
                {   
                    debug("Destroy sigkilled: %d", c->pid);
                    kill(SIGKILL, c->pid);
                }
            }
#endif
            break;
        case Safedestroy:
            XKillClient(dpy, c->win);
            XSync(dpy, False);
            XSetErrorHandler(xerror);
            XUngrabServer(dpy);
#if CFG_STORE_PID && CFG_ALLOW_PID_KILL
            if(c && c->win) 
            {   XDestroyWindow(dpy, c->win);
            }
            XSync(dpy, False);
            if(c && c->win)
            {
                if(c->pid > probablynotsystempid || (c->pid = XGetPid(dpy, c->win)) > probablynotsystempid)
                {   
                    debug("Safedestroy Killed: %d", c->pid);
                    kill(SIGTERM, c->pid);
                }
            }
            XSync(dpy, False);
#endif
            return;
        }
        /* Make sure x receive the request */
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
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

Client *
manage(Window w, XWindowAttributes *wa)
{
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    /* alloc enough memory for new client struct */
    c = poolgrab(pl, accnum);
    ++accnum;
    c->win = w;
    c->num = accnum;
    /* initialize geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;
    c->next = NULL;
    c->snext = NULL;
    c->prev = NULL;
    c->sprev = NULL;
    /* destroy any new clients if we past our client limit */
    if(accnum > CFG_MAX_CLIENT_COUNT)
    {
        c->mon = selmon;
        ++c->mon->cc;
        killclient(c, Safedestroy);
        unmanage(c, 1);
        return NULL;
    }
    updateicon(c);
    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans)))
    {
        c->mon = t->mon;
        c->tags = t->tags;
    }
    else
    {
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
    XSetWindowBorder(dpy, w, scheme[SchemeBorder][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    updatemotifhints(c);
    /* events handled by dwm https://tronche.com/gui/x/xlib/events/processing-overview.html#KeymapStateMask */
    XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    grabbuttons(c, 0);
    c->wasfloating = 0;
    if (!c->isfloating)
    {   c->isfloating = c->wasfloating = trans != None;
    }
    /* set floating if always on top */
    c->isfloating = c->isfloating || c->alwaysontop || c->stayontop;
    attach(c);
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + (sw << 1), c->y, c->w, c->h); /* some windows require this */
    setclientstate(c, NormalState);
    if (c->mon == selmon)
    {   unfocus(selmon->sel, 0);
    }
    c->mon->sel = c;
    ++c->mon->cc;
    arrange(c->mon);
    /* check if selmon->fullscreen */
    setfullscreen(c, selmon->isfullscreen);
    XMapWindow(dpy, w);
    focus(NULL);

#if CFG_STORE_PID
    c->pid = XGetPid(dpy, w);
#endif
    const int index = UI64Hash(w) % CFG_MAX_CLIENT_COUNT;
    hashedwins[index] = c;
    return c;
}

void
maximize(Client *c)
{
    const int x = c->mon->wx;
    const int y = c->mon->wy;
    const int w = c->mon->ww - (CFG_BORDER_PX << 1);
    const int h = c->mon->wh - (CFG_BORDER_PX << 1);
    c->oldx = c->x;
    c->oldy = c->y;
    c->oldw = c->w;
    c->oldh = c->h;
    resize(c, x, y, w, h, 1);
}

void
maximizevert(Client *c)
{
    const int x = c->x;
    const int y = c->mon->wy;
    const int w = c->w;
    const int h = c->mon->wh - (CFG_BORDER_PX << 1);
    c->oldy = c->y;
    c->oldh = c->h;
    resize(c, x, y, w, h, 1);
}

void
maximizehorz(Client *c)
{
    const int x = c->mon->wx;
    const int y = c->y;
    const int w = c->mon->ww - (CFG_BORDER_PX << 1);
    const int h = c->h;
    c->oldx = c->x;
    c->oldw = c->w;
    resize(c, x, y, w, h, 1);
}

void
monocle(Monitor *m)
{
    int nx, ny;
    int nw, nh;
    int cbw; /* client border width */
    int cc;
    Client *c;
    nx = m->wx;
    ny = m->wy;
    cc = 0;
    for(c = m->clients; c; c = c->next) cc += !!ISVISIBLE(c);
    if(cc) snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", cc);
    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
    {
        cbw = c->bw << 1;
        nw = m->ww - cbw;
        nh = m->wh - cbw;
        resize(c, nx, ny, nw, nh, 0);
    }
}

Client *
nexttiled(Client *c)
{
    for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
    return c;
}

Client *
nextvisible(Client *c)
{
    for(; c && !ISVISIBLE(c); c = c->next);
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

void
restoresession(void)
{
    /* TODO */

    restoremonsession(selmon);
}
void
restoremonsession(Monitor *m)
{
    /* chonky function */
    Window cwin;
    unsigned int ctags;
    int cx, cy;
    int cox, coy;
    int cw, ch, cow, coh;
    int cofloating, cfloating;
    int cissel;
    int clyt;
    int curtag;

    Client *c;
    Client **clients;
    Client *csel;
    Arg arg;
    int cc;

    const int MAX_LENGTH = 1024;
    const int CHECK_SUM = 15; /* equal to number of sscanf elements so %d %d %d %d would be a checksum of 4 (type doesnt matter ) */
    int check;
    FILE *fr;
    char str[MAX_LENGTH];
    char *nl;
    int i;

    c = csel = NULL;
    fr = NULL;
    ctags = curtag = 0;
    cx = cy = cox = coy = 0;
    cw = ch = cow = coh = 0;
    cofloating = cfloating = 0;
    cissel = 0;
    clyt = 0;
    cc = i = 0;
    if(!m) return;
    if(!m->clients) return;
    if (!(fr = fopen(SESSION_FILE, "r"))) return;
    for(c = m->clients; c; c = c->next) ++cc;
    if(!(clients = calloc(cc, sizeof(Client *)))) return;
    while(1)
    {
        if(fgets(str, MAX_LENGTH, fr))
        {
            nl = strchr(str, '\n');
            if(!nl)
            {
                if(!feof(fr))
                {
                    debug("WARNING: RESTORE_SESSION_BUFFER_TOO_SMALL");
                    continue;
                }
            } /* remove new line char */
            else *nl = '\0';
            check = sscanf(str, 
                        "%lu %u"
                        " %d %d %d %d"
                        " %d %d %d %d"
                        " %d %d"
                        " %d %d"
                        " %d "
                        ,
                        &cwin, &ctags,
                        &cx, &cy, &cox, &coy,
                        &cw, &ch, &cow, &coh,
                        &cofloating, &cfloating,
                        &cissel, &clyt, 
                        &curtag
                    );
            /* XXX */
            if(check != CHECK_SUM) continue;
            c = wintoclient(cwin);
            c->tags = ctags;
            c->oldx = cox;
            c->oldy = coy;
            c->oldw = cow;
            c->oldh = coh;
            c->wasfloating = cofloating;
            if(cissel) csel = c;
            /* breaks on non floating clients */
            if(cfloating)
            {
                setfloating(c, 1);
                resize(c, cx, cy, cw, ch, 0);
            }
            setmonlyt(m, clyt);
            int cflag = 0;
            /* restack order (This is skipped if we fuck up) */
            for(i = 0; i < cc; ++i)
            {
                if(!clients[i]) 
                {
                    clients[i] = c;
                    cflag = 1;
                    break;
                }
            }
            if(!cflag) debug("WARNING: RESTORE_SESSION_TOO_MANY_CLIENTS");
        }
        else
        {
            if(ferror(fr))
            {
                debug("WARNING: RESTORE_SESSION_FGETS_FAILURE");
                goto CLEANUP;
            }
            /* EOF reached */
            break;
        }
        /* reset values */
        cx = cy = cox = coy = 0;
        cw = ch = cow = coh = 0;
        cofloating = cfloating = 0;
        ctags = 0;
        cissel = 0;
        clyt = 0;

    }
    for(i = cc - 1; i >= 0; --i)
    {
        c = clients[i];
        if(!c || !c->win) continue;
        detach(c);
        attach(c);
    }
    arrange(m);

    /* toggle tag if arent in it */
    arg.ui = curtag;
    View(&arg);

    /* focus client if we are in the same tag or if we fucked up check current tagset */
    if(csel && (csel->tags == curtag || selmon->tagset[selmon->seltags] == csel->tags)) pop(csel);

    goto CLEANUP;
CLEANUP:
    fclose(fr);
    remove(SESSION_FILE);
    free(clients);
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

Monitor *
recttomon(int x, int y, int w, int h)
{
    Monitor *m, *r = selmon;
    int a, area = 0;

    for (m = mons; m; m = m->next)
        if ((a = INTERSECT(x, y, w, h, m)) > area)
        {
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
    c->oldy = c->y;
    c->oldw = c->w;
    c->oldh = c->h;
    c->x = wc.x = x;
    c->y = wc.y = y;
    c->w = wc.width = w;
    c->h = wc.height = h;
    wc.border_width = c->bw;
    XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
    configure(c);
    XSync(dpy, False);
}
void
restack(Monitor *m)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;
    drawbar(m);
    if (!m->sel) return;
    wc.stack_mode = Below;
    wc.sibling = m->barwin;
    /* configure windows */
    for (c = m->stack; c; c = c->snext)
    {
        if(ISVISIBLE(c))
        {
            XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
            wc.sibling = c->win;
        }
        if(docked(c)) 
        {   setfloating(c, 0);
        }
    }

    if(m->sel->isfloating || m->sel->isfullscreen) XRaiseWindow(dpy, m->sel->win);

    if(!CFG_WIN10_FLOATING)
    {
        for(c = m->slast; c; c = c->sprev) 
        {   
            if(c->isfloating && ISVISIBLE(c)) 
            {   XRaiseWindow(dpy, c->win);
            }
        }
    }
    for(c = m->slast; c; c = c->sprev) 
    {
        if(c->alwaysontop && ISVISIBLE(c)) 
        {   XRaiseWindow(dpy, c->win);
        }
    }
    for(c = m->slast; c; c = c->sprev) 
    {
        if(c->stayontop && ISVISIBLE(c)) 
        {   XRaiseWindow(dpy, c->win);
        }
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
        if (handler[ev.type]) 
        {   handler[ev.type](&ev); /* call handler */
        }
        if(!ENABLE_DEBUGGING)
        {   continue;
        }
        switch (ev.type) 
        {
            case KeyPress:            debug("KeyPress");         break;
            case KeyRelease:          debug("KeyRelease");       break;
            case ButtonPress:         debug("ButtonPress");      break;
            case ButtonRelease:       debug("ButtonRelease");    break;
            //case MotionNotify:        debug("MotionNotify");     break;
            case EnterNotify:         debug("EnterNotify");      break;
            case LeaveNotify:         debug("LeaveNotify");      break;
            case FocusIn:             debug("FocusIn");          break;
            case FocusOut:            debug("FocusOut");         break;
            case KeymapNotify:        debug("KeymapNotify");     break;
            case Expose:              debug("Expose");           break;
            case GraphicsExpose:      debug("GraphicsExpose");   break;
            case NoExpose:            debug("NoExpose");         break;
            case VisibilityNotify:    debug("VisibilityNotify"); break;
            case CreateNotify:        debug("CreateNotify");     break;
            case DestroyNotify:       debug("DestroyNotify");    break;
            case UnmapNotify:         debug("UnmapNotify");      break;
            case MapNotify:           debug("MapNotify");        break;
            case MapRequest:          debug("MapRequest");       break;
            case ReparentNotify:      debug("ReparentNotify");   break;
            case ConfigureNotify:     debug("ConfigureNotify");  break;
            case ConfigureRequest:    debug("ConfigureRequest"); break;
            case GravityNotify:       debug("GravityNotify");    break;
            case ResizeRequest:       debug("ResizeRequest");    break;
            case CirculateNotify:     debug("CirculateNotify");  break;
            case CirculateRequest:    debug("CirculateRequest"); break;
            case PropertyNotify:      debug("PropertyNotify");   break;
            case SelectionClear:      debug("SelectionClear");   break;
            case SelectionRequest:    debug("SelectionRequest"); break;
            case SelectionNotify:     debug("SelectionNotify");  break;
            case ColormapNotify:      debug("ColormapNotify");   break;
            case ClientMessage:       debug("ClientMessage");    break;
            case MappingNotify:       debug("MappingNotify");    break;
        }
    }
}

void
savesession(void)
{
    /* fix later */
    savemonsession(selmon);
}

void
savemonsession(Monitor *m)
{
    Client *c;
    int lyt = -1;
    FILE *fw = fopen(SESSION_FILE, "w");
    if(!fw) return;
    while (m->lt[selmon->sellt] != &layouts[++lyt]);

    for (c = m->clients; c; c = c->next)
    {
        fprintf(fw, 
                "%lu %u"
                " %d %d %d %d"
                " %d %d %d %d"
                " %d %d"
                " %d %d"
                " %d "
                "\n"
                ,
                c->win, c->tags,
                c->x, c->y, c->oldx, c->oldy,
                c->w, c->h, c->oldw, c->oldh,
                c->wasfloating, c->isfloating,
                (m->sel == c), lyt, 
                c->mon->tagset[c->mon->seltags]
                );
    }
    fclose(fw);
}

/* scan for new clients */
void
scan(void)
{
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num))
    {
        for (i = 0; i < num; i++)
        {
            /* override_redirect only needed to be handled for old windows */
            /* X auto redirects when running wm so no need to do anything else */
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                    || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1)) continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
            {   manage(wins[i], &wa);
            }
        }
        for (i = 0; i < num; i++)
        {   /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa)) continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                    && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
            {   manage(wins[i], &wa);
            }
        }
        if (wins) XFree(wins);
    }
}

int
sendevent(Client *c, Atom proto)
{
    int n;
    Atom *protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, c->win, &protocols, &n))
    {
        while (!exists && n--)
            exists = protocols[n] == proto;
        XFree(protocols);
    }
    if (exists)
    {
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
setborderbcol(Window win, char *schemecol)
{
    //XSetWindowBorder(dpy, win, scheme[ColBorder].pixel);
}

void
setclientstate(Client *c, long state)
{
    long data[] = { state, None };

    XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

void
setdesktop(void)
{
    long data[] = { 0 };
    XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setdesktopnames(void)
{
    XTextProperty text;
    Xutf8TextListToTextProperty(dpy, (char **)tags, TAGSLENGTH, XUTF8StringStyle, &text);
    XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
}

void
setdesktopnum(void)
{
    long data[] = { TAGSLENGTH };
    XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setfocus(Client *c)
{
    if (!c->neverfocus)
    {
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *) &(c->win), 1);
    }
    sendevent(c, wmatom[WMTakeFocus]);
}

void
setfloating(Client *c, int isfloating)
{
    c->wasfloating = c->isfloating;
    c->isfloating = isfloating;
}

void
setfullscreen(Client *c, int fullscreen)
{
    if (fullscreen && !c->isfullscreen)
    {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace, (unsigned char*)&netatom[NetWMStateFullscreen], 1);
        c->isfullscreen = 1;
        c->oldbw = c->bw;
        c->bw = 0;
        resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
        XRaiseWindow(dpy, c->win);
    }
    else if (!fullscreen && c->isfullscreen)
    {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace, (unsigned char*)0, 0);
        c->isfullscreen = 0;
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
setmonlyt(Monitor *m, int layout)
{
    m->lt[!m->sellt] = m->lt[m->sellt];
    m->lt[m->sellt] = (Layout *)&layouts[layout];
}

void
setshowbar(Monitor *m, int show)
{
    m->oshowbar = m->showbar;
    m->showbar = show;
}

void
setsticky(Client *c, int sticky)
{
    if(sticky) 
    {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                PropModeReplace, (unsigned char *) &netatom[NetWMStateSticky], 1);
        /* make client tags divisible by 2^x (AKA shown on all tags) */
        c->tags = TAGMASK;
        return;
    }
    c->tags = c->mon->tagset[c->mon->seltags];
    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace, (unsigned char *)0, 0);
}

void
setup(void)
{
    /* IO handler */
    XSetIOErrorHandler(xexithandler);
    /* clean up any zombies immediately */
    sighandler();
    /* setup pool (biggest risk of failure due to calloc) */
    setuppool();
    XSetWindowAttributes wa;
    screen = DefaultScreen(dpy);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);
    root = RootWindow(dpy, screen);
    drw = drw_create(dpy, screen, root, sw, sh);
    if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
        die("FATAL: NO_FONTS_LOADED");
    lrpad = drw->fonts->h;
    bh = drw->fonts->h + 2;
    if(CFG_BAR_HEIGHT) bh = CFG_BAR_HEIGHT;
    updategeom();

    XInitAtoms(dpy);
    motifatom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

    setupcur();
    setuptags();
    updatebars();
    updatestatus();
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetSupportingWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], XInternAtom(dpy, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *) WM_NAME, LENGTH(WM_NAME));
    XChangeProperty(dpy, root, netatom[NetSupportingWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    setdesktopnum();
    setdesktop();
    setdesktopnames();
    setviewport();
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor = cursor[CurNormal]->cursor;
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
                    |ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask
                    |LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}

void
setupcur(void)
{
    cursor[CurNormal]           = drw_cur_create(drw, XC_left_ptr);


    /* resizing curs */

    /* vertical */
    cursor[CurResizeTop]        = drw_cur_create_img(drw, "size_ver");
    if(!cursor[CurResizeTop])
    {   cursor[CurResizeTop]    = drw_cur_create(drw, XC_top_side);
    }
    cursor[CurResizeBottom]     = cursor[CurResizeTop];

    /* diagonal left */
    cursor[CurResizeTopLeft]    = drw_cur_create_img(drw, "size_fdiag");
    if(!cursor[CurResizeTopLeft])
    {   cursor[CurResizeTopLeft] = drw_cur_create(drw, XC_top_left_corner);
    }
    cursor[CurResizeBottomLeft] = cursor[CurResizeTopLeft];

    /* diagonal right */
    cursor[CurResizeTopRight]   = drw_cur_create_img(drw, "size_bdiag");
    if(!cursor[CurResizeTopRight])
    {   cursor[CurResizeTopRight] = drw_cur_create(drw, XC_top_right_corner);
    }
    cursor[CurResizeBottomRight]= cursor[CurResizeTopRight];
    /* horizontal */
    cursor[CurResizeLeft]       = drw_cur_create_img(drw, "size_hor");
    if(!cursor[CurResizeLeft])
    {   cursor[CurResizeLeft] = drw_cur_create(drw, XC_sb_h_double_arrow);
    }
    cursor[CurResizeRight]      = cursor[CurResizeLeft];
    /* move curs */
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
}

void
setuppool(void)
{
    pl = poolcreate(CFG_MAX_CLIENT_COUNT + 1, sizeof(Client));
}

void
setuptags(void)
{
    size_t i;
    if (LENGTH(tags) > LENGTH(tagcols)) die("too few color schemes for the tags");
    scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
    for (i = 0; (long unsigned int)i < LENGTH(colors); i++)
        scheme[i] = drw_scm_create(drw, colors[i], 3);
    tagscheme = ecalloc(LENGTH(tagcols), sizeof(Clr *));
    for (i = 0; (long unsigned int)i < LENGTH(tagcols); i++) tagscheme[i] = drw_scm_create(drw, (char **)tagcols[i], 2);

}

void
setuptimezone(void)
{
    if(!CFG_AUTO_TIME_ZONE && CFG_TIME_ZONE)  setenv("TZ", CFG_TIME_ZONE, 1);
}

void
seturgent(Client *c, int urg)
{
    XWMHints *wmh;

    c->isurgent = urg;
    if(urg)
    {   XSetWindowBorder(dpy, c->win, scheme[SchemeUrgent][ColBorder].pixel);
    }
    else
    {   XSetWindowBorder(dpy, c->win, scheme[SchemeBorder][ColBorder].pixel);
    }
    if (!(wmh = XGetWMHints(dpy, c->win)))
        return;
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, c->win, wmh);
    XFree(wmh);
    drawbar(c->mon);
}

void
setviewport(void)
{
    long data[] = { 0, 0 };
    XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 2);
}

/* called when switching tags/workspaces */
void
showhide(Client *c)
{
    /* this is called alot in short bursts */
    int x = ISVISIBLE(c);
    x  =  !x * (c->mon->mx - (WIDTH(c) << 1));
    x += (!x * c->x);
    XMoveWindow(dpy, c->win, x, c->y);
    /*
    if(ISVISIBLE(c))
        XMoveWindow(dpy, c->win, c->x, c->y);
    else
        XMoveWindow(dpy, c->win, c->mon->mx - (WIDTH(c) << 1), c->y);
    */
}

void
showhidemap(Client *c)
{
    if (ISVISIBLE(c))
        winmap(c, 1);
    else
        winunmap(c->win, root, 1);
}

void
sigchld(int signo) /* signal */
{
    /* wait for childs (zombies) to die */
    while (0 < waitpid(-1, NULL, WNOHANG));
}

void
sighandler(void)
{
    if (signal(SIGCHLD, &sigchld) == SIG_ERR)
    {   die("FATAL: CANNOT_INSTALL_SIGCHLD_HANDLER");
    }
    sigchld(0);

    if(signal(SIGTERM, &sigterm) == SIG_ERR) 
    {   
        die("FATAL: CANNOT_INSTALL_SIGTERM_HANDLER");
        signal(SIGHUP, SIG_DFL); /* default signal */
    }

    if(signal(SIGHUP, &sighup) == SIG_ERR) 
    {   
        debug("WARNING: CANNOT_INSTALL_SIGHUP_HANDLER");
        signal(SIGHUP, SIG_DFL); /* default signal */
    }
}

void
sighup(int signo)
{
    restart();
}

void
sigterm(int signo)
{
    quit();
}

void
tile(Monitor *m)
{
    unsigned int h, mw, my, ty;
    int n, i;
    int nx, ny;
    int nw, nh;
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
    if(n == 0) return;

    if (n > m->nmaster) mw = m->nmaster ? m->ww * m->mfact : 0;
    else mw = m->ww;
    for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i)
    {
        if (i < m->nmaster)
        {
            h = (m->wh - my) / (MIN(n, m->nmaster) - i);
            nx = m->wx;
            ny = m->wy + my;
            nw = mw - (c->bw << 1);
            nh = h - (c->bw << 1);

            /* we divide nw also to get even gaps
             * if we didnt the center gap would be twices as big
             * Although this may be desired, one would simply remove the shift ">>" by 1 in nw 
             */
            nx += CFG_GAP_PX;
            ny += CFG_GAP_PX;
            nw -= CFG_GAP_PX << 1;
            nh -= CFG_GAP_PX << 1;
            resize(c, nx, ny, nw, nh, 0);
                                                                        /* spacing for windows below */
            if (my + HEIGHT(c) < (unsigned int)m->wh) my += HEIGHT(c) + CFG_GAP_PX;
        }
        else
        {
            h = (m->wh - ty) / (n - i);
            nx = m->wx + mw;
            ny = m->wy + ty;
            nw = m->ww - mw - (c->bw << 1);
            nh = h - (c->bw << 1);

            nx += CFG_GAP_PX >> 1;
            ny += CFG_GAP_PX;
            nw -= CFG_GAP_PX << 1;
            nh -= CFG_GAP_PX << 1;

            resize(c, nx, ny, nw, nh, 0);
                                                                    /* spacing for windows below */ 
            if (ty + HEIGHT(c) < (unsigned int)m->wh) ty += HEIGHT(c) + CFG_GAP_PX;
        }
    }
}

void
unfocus(Client *c, int setfocus)
{
    if (!c) return;
    grabbuttons(c, 0);
    lastfocused = c;
	XSetWindowBorder(dpy, c->win, scheme[SchemeBorder][ColBorder].pixel);
    if (setfocus)
    {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void
unmanage(Client *c, int destroyed)
{
    Monitor *m = c->mon;
    XWindowChanges wc;
    --accnum;
    --m->cc;
    detach(c);
    detachstack(c);
    freeicon(c);
    if (!destroyed)
    {
        /* This check simple reverts the windows state back to monocle view because some clients "store" their previous coordinates */
        if(!c->isfloating)
        {   
            const Layout *lyt = getmonlyt(m);
            if(lyt == &layouts[Tiled] || lyt == &layouts[Grid])
            {   resizeclient(c, m->wx, m->wy, m->ww, m->wh);
            }
        }
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
    /* -- cause we start index from 0 */
    --c->num;
    const int index = UI64Hash(c->win) % CFG_MAX_CLIENT_COUNT;
    hashedwins[index] = NULL;
    poolfree(pl, c, c->num);
    focus(NULL);
    updateclientlist();
    arrange(m);
    c = NULL;
}

void
updatebars(void)
{
    Monitor *m;
    XSetWindowAttributes wa =
    {
        .override_redirect = True, /*patch */
        .background_pixmap = ParentRelative,
        .event_mask = ButtonPressMask|ButtonReleaseMask|ExposureMask|PointerMotionMask
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
    if (m->showbar)
    {
        m->wh -= bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh;
        m->wy = m->topbar ? m->wy + bh : m->wy;
    }
    else m->by = -bh;
}

void
updateclientlist(void)
{
    Client *c;
    Monitor *m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next)
    {
        for (c = m->clients; c; c = c->next)
        {
            XChangeProperty(dpy, root, netatom[NetClientList],
                            XA_WINDOW, 32, PropModeAppend,
                            (unsigned char *) &(c->win), 1);
        }
    }
}

void
updatedesktop(void)
{
    long rawdata[] = { selmon->tagset[selmon->seltags] };
    int i=0;
    while(*rawdata >> (i + 1)) i++;
    long data[] = { i };
    XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

int
updategeom(void)
{
    int dirty = 0;
#ifdef XINERAMA
    if (XineramaIsActive(dpy))
    {
        int i, j, n, nn;
        Client *c;
        Monitor *m;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo *unique = NULL;

        for (n = 0, m = mons; m; m = m->next, ++n);
        /* only consider unique geometries as separate screens */
        unique = ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; ++i)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
        XFree(info);
        nn = j;

        /* new monitors if nn > n */
        for (i = n; i < nn; ++i)
        {
            for (m = mons; m && m->next; m = m->next);
            if (m)
                m->next = createmon();
            else
                mons = createmon();
        }
        for (i = 0, m = mons; i < nn && m; m = m->next, ++i)
            if
            (i >= n
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
        for (i = nn; i < n; ++i)
        {
            for (m = mons; m && m->next; m = m->next);
            while ((c = m->clients))
            {
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
    }
    else
#endif /* XINERAMA */
    {
        /* default monitor setup */
        if (!mons)
            mons = createmon();
        if (mons->mw != sw || mons->mh != sh)
        {
            dirty = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty)
    {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void
updatemotifhints(Client *c)
{
    if (!CFG_DECOR_HINTS) return;
    Atom real;
    int format;
    unsigned char *p = NULL;
    unsigned long n, extra;
    unsigned long *motif;
    int width, height;

    if (XGetWindowProperty(dpy, c->win, motifatom, 0L, 5L, False, motifatom,
                           &real, &format, &n, &extra, &p) == Success && p != NULL) {
        motif = (unsigned long*)p;
        if (motif[MWM_HINTS_FLAGS_FIELD] & MWM_HINTS_DECORATIONS)
        {
            width = WIDTH(c);
            height = HEIGHT(c);
            if (motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_ALL ||
                    motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_BORDER ||
                    motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_TITLE)
            {   c->bw = c->oldbw = CFG_BORDER_PX;
            }
            else
            {   c->bw = c->oldbw = 0;
            }
            resize(c, c->x, c->y, width - (c->bw << 1), height - (c->bw << 1), 0);
        }
        XFree(p);
    }
}

void
updatenumlockmask(void)
{
    unsigned int i;
    int j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; ++i)
        for (j = 0; j < modmap->max_keypermod; ++j)
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
    /* init values */
    c->basew = c->baseh = 0;
    c->incw = c->inch = 0;
    c->maxw = c->maxh = 0;
    c->minw = c->minh = 0;
    c->maxa = c->mina = 0.0;
    c->hintsvalid = 1;
    if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PMinSize)
    {
        c->minw = size.min_width;
        c->minh = size.min_height;
    }
    else if (size.flags & PBaseSize)
    {
        c->minw = size.base_width;
        c->minh = size.base_height;
    }

    if (size.flags & PBaseSize)
    {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    }
    else if(size.flags & PMinSize)
    {
        c->basew = c->minw;
        c->baseh = c->minh;
    }
    if (size.flags & PResizeInc)
    {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    }
    if (size.flags & PMinSize)
    {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    }
    if (size.flags & PAspect)
    {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    }
    c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
    if(c->alwaysontop && c->isfixed)
    {   c->alwaysontop = 0;
    }
}

void
updatestatus(void)
{
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        strcpy(stext, WM_NAME);
    drawbar(selmon);
}

void
updatetitle(Client *c)
{
    XGetWindowName(dpy, c->win, c->name, sizeof(c->name));
}

void
updateicon(Client *c)
{
    if(!CFG_ICON_SHOW) {
        c->icon = None;
        return;
    }
    freeicon(c);
    c->icon = geticonprop(c->win, &c->icw, &c->ich);
}

void
updatewindowstate(Client *c, Atom state, int data)
{
    /* possible states
     _NET_WM_STATE_MODAL, (Temporary dialog window that MUST BE done before proceeding application)
     _NET_WM_STATE_STICKY, (Remains on desktop even if we change the tag/desktop)
     _NET_WM_STATE_MAXIMIZED_VERT, (maximize the vertical aspect)
     _NET_WM_STATE_MAXIMIZED_HORZ, (maximize the horizontal aspect)
     _NET_WM_STATE_SHADED,         ("indicates that the window is shaded")
     _NET_WM_STATE_SKIP_TASKBAR,   (Is not nor will ever be a taskbar)
     _NET_WM_STATE_SKIP_PAGER,     (Dont show as list of client options when displaying them (DESKTOPS) ex: dont show in alttab)
     _NET_WM_STATE_HIDDEN,         (Minimized windows use this to "hide" themselves AKA not show)
     _NET_WM_STATE_FULLSCREEN,     (Sets fullscreen)
     _NET_WM_STATE_ABOVE,          (Moves the stacking order; This is replaced with "AlwaysOnTop")
     _NET_WM_STATE_BELOW,          (Samething as above but remain below most windows)
     _NET_WM_STATE_DEMANDS_ATTENTION, (Something important is happening in the window)
     _NET_WM_STATE_FOCUSED,        (Wheter or not the clients decorations are currently drawn (Useless))
     */
    /* 0 remove
     * 1 add
     * 2 toggle
     */
    /* this is a headache to look at fix later */
    int toggle = (data == 2);

    Monitor *m;
    Client *temp;
    
    m = c->mon;
    data = !!data;
    if (state == netatom[NetWMStateModal])
    {
        if(toggle)
        {   setfloating(c, !c->isfloating);
            seturgent(c, !c->isurgent);
        }
        else
        {   setfloating(c, data);
            seturgent(c, data);
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateAlwaysOnTop])
    {
        if(toggle)
        {   setfloating(c, !c->isfloating);
            c->alwaysontop = !c->alwaysontop;
        }
        else
        {   setfloating(c, data);
            c->alwaysontop = data;
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateStayOnTop])
    {
        if(toggle)
        {   setfloating(c, !c->isfloating);
            c->stayontop = !c->stayontop;
        }
        else
        {   setfloating(c, data);
            c->stayontop = data;
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateDemandAttention])
    {
        if(toggle)
        {   seturgent(c, !c->isurgent);
        }
        else
        {   seturgent(c, data);
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateFullscreen])
    {
        if(toggle)
        {   ToggleFullscreen(NULL);
        }
        if(data)
        {
            if(!c->mon->isfullscreen)
            {   ToggleFullscreen(NULL);
            }
        }
        else
        {
            if(c->mon->isfullscreen)
            {   ToggleFullscreen(NULL);
            }
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateMaximizedHorz])
    {
        if(toggle)
        {
            temp = c->mon->sel;
            c->mon->sel = c;
            MaximizeWindowVertical(NULL);
            c->mon->sel = temp;
        }
        else if(data)
        {   maximizevert(c);
        }
        else
        {   resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 0);
        }
        arrange(m);
    }
    else if (state == netatom[NetWMStateMaximizedVert])
    {
        if(toggle)
        {   
            temp = c->mon->sel;
            c->mon->sel = c;
            MaximizeWindowVertical(NULL);
            c->mon->sel = temp;
        }
        else if(data)
        {   maximizehorz(c);
        }
        else
        {   resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 0);
        }
        arrange(m);
    }
    /*  else if (state == netatom[NetWMAbove])          {REPLACED BY NetWMAlwaysOnTop}      */
    else if (state == netatom[NetWMStateSticky])
    {
        if(toggle)
        {   setsticky(c, !ISSTICKY(c));
        }
        else 
        {   setsticky(c, data);
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateBelow])
    {
        /* idk how to handle this */
        if(toggle)
        {
        }
        else if(data)
        {   XLowerWindow(dpy, c->win);
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateSkipTaskbar])
    {   
        if(toggle)
        {   c->hidden = !c->hidden;
        }
        else
        {   c->hidden = data;
        }
        drawbar(m);
    }
    else if (state == netatom[NetWMStateSkipPager])
    {   /* This is stupid; IGNORE */
    }
    else if (state == netatom[NetWMStateHidden])
    {   
        if(toggle)
        {   /* IGNORE */
        }
        else
        {   c->hidden = data;
        }
        restack(m);
    }
    else if (state == netatom[NetWMStateFocused])
    {
    }
}

void
updatewindowtype(Client *c)
{
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);
    updatewindowstate(c, state, 1); /* _NET_WM_STATE_ADD */
    if (wtype == netatom[NetWMWindowTypeDesktop])
    {   /* This feature is kinda dumb so we ignore it */
    }
    else if (wtype == netatom[NetWMWindowTypeDock])
    {   setfloating(c, 1);
        c->stayontop = c->alwaysontop = 1;
    }
    else if (wtype == netatom[NetWMWindowTypeToolbar])
    {   /* TODO */
    }
    else if (wtype == netatom[NetWMWindowTypeMenu])
    {   /* TODO */
    }
    else if (wtype == netatom[NetWMWindowTypeUtility])
    {   /* TODO */
    }
    else if (wtype == netatom[NetWMWindowTypeSplash])
    {   /* IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeDialog])
    {   setfloating(c, 1);
        c->alwaysontop = 1;
    }
    else if (wtype == netatom[NetWMWindowTypeDropdownMenu])
    {   /* TODO */
    }
    else if (wtype == netatom[NetWMWindowTypePopupMenu])
    {   /* override-redirect IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeTooltip])
    {   /* override-redirect IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeNotification])
    {   /* override-redirect IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeCombo])
    {   /* override-redirect IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeDnd])
    {   /* override-redirect IGNORE */
    }
    else if (wtype == netatom[NetWMWindowTypeNormal])
    {   /* This hint indicates that this window has no special properties IGNORE */
    }
}

void
updatewmhints(Client *c)
{
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, c->win)))
    {
        if (c == selmon->sel && wmh->flags & XUrgencyHint)
        {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        }
        else c->isurgent = !!(wmh->flags & XUrgencyHint);
        if (wmh->flags & InputHint)
            c->neverfocus = !wmh->input;
        else
            c->neverfocus = 0;
        XFree(wmh);
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

    if (deiconify) winsetstate(win, NormalState);

    XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
    XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
    XMapWindow(dpy, win);
}

Client *
wintoclient(Window w)
{
    Client *c;
    Monitor *m;

    c = hashedwins[UI64Hash(w) % CFG_MAX_CLIENT_COUNT];
    if(c && c->win == w)
    {   return c;
    }
    /* fall through */
    for (m = mons; m; m = m->next)
    {
        c = m->clients;
        for (; c; c = c->next )
        {   if (c->win == w) return c;
        }

    }
    return NULL;
}

Monitor *
wintomon(Window w)
{
    int x, y;
    Client *c;
    Monitor *m;

    if (w == root && getrootptr(&x, &y)) return recttomon(x, y, 1, 1);
    for (m = mons; m; m = m->next)
        if (w == m->barwin) return m;
    if ((c = wintoclient(w))) return c->mon;
    return selmon;
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

    if (iconify) winsetstate(win, IconicState);

    XSelectInput(dpy, winroot, ra.your_event_mask);
    XSelectInput(dpy, win, ca.your_event_mask);
    XUngrabServer(dpy);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. 
 * Side note: Most errors shown can be ignored leaving only the extensions 
 * and some others that can call exit(); however we tell the user about these as their still useful to know
 * */
int
xerror(Display *display, XErrorEvent *ee)
{
    switch(ee->error_code)
    {
        case Success: debug("WARNING: X_ERROR_TRIGGER_ON_SUCCESS "); break;
        case BadWindow: debug("WARNING: X_ERROR_BAD_WINDOW"); break;
        case BadAccess: debug("WARNING: X_ERROR_BAD_ACCESS"); break;
        case BadAlloc: debug("WARNING: X_ERROR_BAD_ALLOC"); break;
        case BadAtom: debug("WARNING: X_ERROR_BAD_ATOM"); break;
        case BadColor: debug("WARNING: X_ERROR_BAD_COLOR"); break;
        case BadCursor: debug("WARNING: X_ERROR_BAD_CURSOR"); break;
        case BadDrawable: debug("WARNING: X_ERROR_BAD_DRAWABLE"); break;
        case BadFont: debug("WARNING: X_ERROR_BAD_FONT"); break;
        case BadGC: debug("WARNING: X_ERROR_BAD_GC"); break;
        case BadIDChoice: debug("WARNING: X_ERROR_BAD_ID_CHOICE"); break;
        case BadImplementation: debug("WARNING: X_ERROR_BAD_IMPLEMENTATION"); break;
        case BadLength: debug("WARNING: X_ERROR_BAD_LENGTH"); break;
        case BadMatch: debug("WARNING: X_ERROR_BAD_MATCH"); break;
        case BadName: debug("WARNING: X_ERROR_BAD_NAME"); break;
        case BadPixmap: debug("WARNING: X_ERROR_BAD_PIXMAP"); break;
        case BadRequest: debug("WARNING: X_ERROR_BAD_REQUEST"); break;
        case BadValue: debug("WARNING: X_ERROR_BAD_VALUE"); break;
        default: goto ret; 
    }
    /* sometimes doesnt change binary size for some reason???
     * But its not in the assembly so Idk
     */
    if(CFG_X_VERBOSE_ERRORS)
    {
        switch(ee->request_code)
        {   
            case X_CreateWindow: debug("X_REQUEST_CREATE_WINDOW"); break;
            case X_ChangeWindowAttributes: debug("X_REQUEST_CHANGE_WINDOW_ATTRIBUTES"); break;
            case X_GetWindowAttributes: debug("X_REQUEST_GET_WINDOW_ATTRIBUTES"); break;
            case X_DestroyWindow: debug("X_REQUEST_DESTROY_WINDOW"); break;
            case X_DestroySubwindows: debug("X_REQUEST_DESTROY_SUBWINDOWS"); break;
            case X_ChangeSaveSet: debug("X_REQUEST_CHANGE_SAVE_SET"); break;
            case X_ReparentWindow: debug("X_REQUEST_REPARENT_WINDOW"); break;
            case X_MapWindow: debug("X_REQUEST_MAP_WINDOW"); break;
            case X_MapSubwindows: debug("X_REQUEST_MAP_SUBWINDOWS"); break;
            case X_UnmapWindow: debug("X_REQUEST_UNMAP_WINDOW"); break;
            case X_UnmapSubwindows: debug("X_REQUEST_UNMAP_SUBWINDOWS"); break;
            case X_ConfigureWindow: debug("X_REQUEST_CONFIGURE_WINDOW"); break;
            case X_CirculateWindow: debug("X_REQUEST_CIRCULATE_WINDOW"); break;
            case X_GetGeometry: debug("X_REQUEST_GET_GEOMETRY"); break;
            case X_QueryTree: debug("X_REQUEST_QUERY_TREE"); break;
            case X_InternAtom: debug("X_REQUEST_INTERN_ATOM"); break;
            case X_GetAtomName: debug("X_REQUEST_GET_ATOM_NAME"); break;
            case X_ChangeProperty: debug("X_REQUEST_CHANGE_PROPERTY"); break;
            case X_DeleteProperty: debug("X_REQUEST_DELETE_PROPERTY"); break;
            case X_GetProperty: debug("X_REQUEST_GET_PROPERTY"); break;
            case X_ListProperties: debug("X_REQUEST_LIST_PROPERTIES"); break;
            case X_SetSelectionOwner: debug("X_REQUEST_SET_SELECTION_OWNER"); break;
            case X_GetSelectionOwner: debug("X_REQUEST_GET_SELECTION_OWNER"); break;
            case X_ConvertSelection: debug("X_REQUEST_CONVERT_SELECTION"); break;
            case X_SendEvent: debug("X_REQUEST_SEND_EVENT"); break;
            case X_GrabPointer: debug("X_REQUEST_GRAB_POINTER"); break;
            case X_UngrabPointer: debug("X_REQUEST_UNGRAB_POINTER"); break;
            case X_GrabButton: debug("X_REQUEST_GRAB_BUTTON"); break;
            case X_UngrabButton: debug("X_REQUEST_UNGRAB_BUTTON"); break;
            case X_ChangeActivePointerGrab: debug("X_REQUEST_CHANGE_ACTIVE_POINTER_GRAB"); break;
            case X_GrabKeyboard: debug("X_REQUEST_GRAB_KEYBOARD"); break;
            case X_UngrabKeyboard: debug("X_REQUEST_UNGRAB_KEYBOARD"); break;
            case X_GrabKey: debug("X_REQUEST_GRAB_KEY"); break;
            case X_UngrabKey: debug("X_REQUEST_UNGRAB_KEY"); break;
            case X_AllowEvents: debug("X_REQUEST_ALLOW_EVENTS"); break;
            case X_GrabServer: debug("X_REQUEST_GRAB_SERVER"); break;
            case X_UngrabServer: debug("X_REQUEST_UNGRAB_SERVER"); break;
            case X_QueryPointer: debug("X_REQUEST_QUERY_POINTER"); break;
            case X_GetMotionEvents: debug("X_REQUEST_GET_MOTION_EVENTS"); break;
            case X_TranslateCoords: debug("X_REQUEST_TRANSLATE_COORDS"); break;
            case X_WarpPointer: debug("X_REQUEST_WARP_POINTER"); break;
            case X_SetInputFocus: debug("X_REQUEST_SET_INPUT_FOCUS"); break;
            case X_GetInputFocus: debug("X_REQUEST_GET_INPUT_FOCUS"); break;
            case X_QueryKeymap: debug("X_REQUEST_QUERY_KEYMAP"); break;
            case X_OpenFont: debug("X_REQUEST_OPEN_FONT"); break;
            case X_CloseFont: debug("X_REQUEST_CLOSE_FONT"); break;
            case X_QueryFont: debug("X_REQUEST_QUERY_FONT"); break;
            case X_QueryTextExtents: debug("X_REQUEST_QUERY_TEXT_EXTENTS"); break;
            case X_ListFonts: debug("X_REQUEST_LIST_FONTS"); break;
            case X_ListFontsWithInfo: debug("X_REQUEST_LIST_FONTS_WITH_INFO"); break;
            case X_SetFontPath: debug("X_REQUEST_SET_FONT_PATH"); break;
            case X_GetFontPath: debug("X_REQUEST_GET_FONT_PATH"); break;
            case X_CreatePixmap: debug("X_REQUEST_CREATE_PIXMAP"); break;
            case X_FreePixmap: debug("X_REQUEST_FREE_PIXMAP"); break;
            case X_CreateGC: debug("X_REQUEST_CREATE_GC"); break;
            case X_ChangeGC: debug("X_REQUEST_CHANGE_GC"); break;
            case X_CopyGC: debug("X_REQUEST_COPY_GC"); break;
            case X_SetDashes: debug("X_REQUEST_SET_DASHES"); break;
            case X_SetClipRectangles: debug("X_REQUEST_SET_CLIP_RECTANGLES"); break;
            case X_FreeGC: debug("X_REQUEST_FREE_GC"); break;
            case X_ClearArea: debug("X_REQUEST_CLEAR_AREA"); break;
            case X_CopyArea: debug("X_REQUEST_COPY_AREA"); break;
            case X_CopyPlane: debug("X_REQUEST_COPY_PLANE"); break;
            case X_PolyPoint: debug("X_REQUEST_POLY_POINT"); break;
            case X_PolyLine: debug("X_REQUEST_POLY_LINE"); break;
            case X_PolySegment: debug("X_REQUEST_POLY_SEGMENT"); break;
            case X_PolyRectangle: debug("X_REQUEST_POLY_RECTANGLE"); break;
            case X_PolyArc: debug("X_REQUEST_POLY_ARC"); break;
            case X_FillPoly: debug("X_REQUEST_FILL_POLY"); break;
            case X_PolyFillRectangle: debug("X_REQUEST_POLY_FILL_RECTANGLE"); break;
            case X_PolyFillArc: debug("X_REQUEST_POLY_FILL_ARC"); break;
            case X_PutImage: debug("X_REQUEST_PUT_IMAGE"); break;
            case X_GetImage: debug("X_REQUEST_GET_IMAGE"); break;
            case X_PolyText8: debug("X_REQUEST_POLY_TEXT_8"); break;
            case X_PolyText16: debug("X_REQUEST_POLY_TEXT_16"); break;
            case X_ImageText8: debug("X_REQUEST_IMAGE_TEXT_8"); break;
            case X_ImageText16: debug("X_REQUEST_IMAGE_TEXT_16"); break;
            case X_CreateColormap: debug("X_REQUEST_CREATE_COLORMAP"); break;
            case X_FreeColormap: debug("X_REQUEST_FREE_COLORMAP"); break;
            case X_CopyColormapAndFree: debug("X_REQUEST_COPY_COLORMAP_AND_FREE"); break;
            case X_InstallColormap: debug("X_REQUEST_INSTALL_COLORMAP"); break;
            case X_UninstallColormap: debug("X_REQUEST_UNINSTALL_COLORMAP"); break;
            case X_ListInstalledColormaps: debug("X_REQUEST_LIST_INSTALLED_COLORMAPS"); break;
            case X_AllocColor: debug("X_REQUEST_ALLOC_COLOR"); break;
            case X_AllocNamedColor: debug("X_REQUEST_ALLOC_NAMED_COLOR"); break;
            case X_AllocColorCells: debug("X_REQUEST_ALLOC_COLOR_CELLS"); break;
            case X_AllocColorPlanes: debug("X_REQUEST_ALLOC_COLOR_PLANES"); break;
            case X_FreeColors: debug("X_REQUEST_FREE_COLORS"); break;
            case X_StoreColors: debug("X_REQUEST_STORE_COLORS"); break;
            case X_StoreNamedColor: debug("X_REQUEST_STORE_NAMED_COLOR"); break;
            case X_QueryColors: debug("X_REQUEST_QUERY_COLORS"); break;
            case X_LookupColor: debug("X_REQUEST_LOOKUP_COLOR"); break;
            case X_CreateCursor: debug("X_REQUEST_CREATE_CURSOR"); break;
            case X_CreateGlyphCursor: debug("X_REQUEST_CREATE_GLYPH_CURSOR"); break;
            case X_FreeCursor: debug("X_REQUEST_FREE_CURSOR"); break;
            case X_RecolorCursor: debug("X_REQUEST_RECOLOR_CURSOR"); break;
            case X_QueryBestSize: debug("X_REQUEST_QUERY_BEST_SIZE"); break;
            case X_QueryExtension: debug("X_REQUEST_QUERY_EXTENSION"); break;
            case X_ListExtensions: debug("X_REQUEST_LIST_EXTENSIONS"); break;
            case X_ChangeKeyboardMapping: debug("X_REQUEST_CHANGE_KEYBOARD_MAPPING"); break;
            case X_GetKeyboardMapping: debug("X_REQUEST_GET_KEYBOARD_MAPPING"); break;
            case X_ChangeKeyboardControl: debug("X_REQUEST_CHANGE_KEYBOARD_CONTROL"); break;
            case X_GetKeyboardControl: debug("X_REQUEST_GET_KEYBOARD_CONTROL"); break;
            case X_Bell: debug("X_REQUEST_BELL"); break;
            case X_ChangePointerControl: debug("X_REQUEST_CHANGE_POINTER_CONTROL"); break;
            case X_GetPointerControl: debug("X_REQUEST_GET_POINTER_CONTROL"); break;
            case X_SetScreenSaver: debug("X_REQUEST_SET_SCREEN_SAVER"); break;
            case X_GetScreenSaver: debug("X_REQUEST_GET_SCREEN_SAVER"); break;
            case X_ChangeHosts: debug("X_REQUEST_CHANGE_HOSTS"); break;
            case X_ListHosts: debug("X_REQUEST_LIST_HOSTS"); break;
            case X_SetAccessControl: debug("X_REQUEST_SET_ACCESS_CONTROL"); break;
            case X_SetCloseDownMode: debug("X_REQUEST_SET_CLOSE_DOWN_MODE"); break;
            case X_KillClient: debug("X_REQUEST_KILL_CLIENT"); break;
            case X_RotateProperties: debug("X_REQUEST_ROTATE_PROPERTIES"); break;
            case X_ForceScreenSaver: debug("X_REQUEST_FORCE_SCREEN_SAVER"); break;
            case X_SetPointerMapping: debug("X_REQUEST_SET_POINTER_MAPPING"); break;
            case X_GetPointerMapping: debug("X_REQUEST_GET_POINTER_MAPPING"); break;
            case X_SetModifierMapping: debug("X_REQUEST_SET_MODIFIER_MAPPING"); break;
            case X_GetModifierMapping: debug("X_REQUEST_GET_MODIFIER_MAPPING"); break;
            case X_NoOperation: debug("X_REQUEST_NO_OPERATION"); break;
            default: goto ret; 
        }
    }
    else
    {
        debug("WARNING: X_ERROR_CODE [%d] X_REQUEST_CODE [%d] X_MINOR_CODE [%d] ****", ee->error_code, ee->request_code, ee->minor_code);
    }
    return 0;
ret:
    debug("FATAL: X_ERROR_CODE [%d] X_REQUEST_CODE [%d] X_MINOR_CODE [%d] ****", ee->error_code, ee->request_code, ee->minor_code);
    return xerrorxlib(dpy, ee); /* may call exit */
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
xexithandler(Display *display)
{
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp("-v", argv[1]))
        die(WM_NAME);
    else if (argc != 1)
        die("usage: args[-v]");
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("WARN: NO_LOCALE_SUPPORT\n", stderr);
    if(!XInitThreads()) die("FATAL: NO_MULTI_THREADING");
    if (!(dpy = XOpenDisplay(NULL)))
        die("FATAL: CANNOT_OPEN_DISPLAY");
    checkotherwm();
    setup();

#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1)
        die("FATAL: OPEN_BSD_PLEDGE");
#endif
    scan();
    restoresession();
    run(); /* main event loop */
    if(RESTART) execvp(argv[0], argv);
    cleanup();
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
