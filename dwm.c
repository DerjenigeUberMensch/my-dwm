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
* ~Caveats, this version of dwm is bloated relative to the main branch of dwm -> https://git.suckless.org/dwm/
* ~So you should not expect this to be able to run as well as dwm
* ~Tested on a i7-4790 16G ddr3, integrated graphics, -> WITH an ssd <-
*
* When drawing using drw make sure to clear it by setting scheme + drw_rect(area you want)
* AND that the drw GC starts at 0,0 so perform your drw actions relative to 0,0
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
#include <sys/time.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h> //config.h multimedia keys
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <Imlib2.h>

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
/* dwm */
#include "dwm.h"
#include "config.def.h"
#include "toggle.c" /* separate list of functions used by user NOT a header file */
#include "keybinds.def.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags
{
    char limitexceeded[LENGTH(tags) > 31 ? -1 : 1];
};

Client *
alttab(int ended)
{
    /* fixe later */
    Monitor *m = selmon;
    static Client *c = NULL, *tabnext = NULL;
    if(ended) tabnext = m->sel;
    if(ended) c = tabnext;
    if(!c) c = nextvisible(m->clients);

    /* get the next visible window */
    c = nextvisible(c->next);
    if(!c) c = nextvisible(m->clients);
    focus(c);
    if(m->lyt == Monocle || m->lyt == Floating)
    {
        if(CFG_ALT_TAB_MAP_WINDOWS)
        {
            if(!c->isfloating && !c->alwaysontop)
            {
                winunmap(c->win, root, 1);
                winmap(c, 1);
            }
        }
        if(CFG_ALT_TAB_SHOW_PREVIEW) arrange(m);
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
            for (m = mons; m && m->num != r->monitor; m = m->next)
                ;
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
    if (CFG_RESIZE_HINTS || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange)
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
arrangeall()
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
buttonpress(XEvent *e)
{
    unsigned int i, click;
    int x;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon)
    {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    if (ev->window == selmon->barwin)
    {
        i = x = 0;
        do x += TEXTW(tags[i]);
        while (ev->x >= x && ++i < LENGTH(tags));
        if (i < LENGTH(tags))
        {
            click = ClkTagBar;
            arg.ui = 1 << i;
            /* hide preview if we click the bar */
        }
        else if (ev->x < x + (int)TEXTW(selmon->ltsymbol))
            click = ClkLtSymbol;
        else if (ev->x > selmon->ww - (int)TEXTW(stext) * CFG_SHOW_WM_NAME)
            click = ClkStatusText;
        /* click = ClkWinTitle; */
    }
    else if ((c = wintoclient(ev->window)))
    {
        detach(c);
        attach(c);
        focus(c);
        if(c->isfloating || c->alwaysontop) XRaiseWindow(dpy, c->win);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < LENGTH(buttons); i++)
        if (click == buttons[i].click &&
                buttons[i].func &&
                buttons[i].button == ev->button &&
                CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
        {
            buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
        }
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
    free(mon->tagmap);
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

int
docked(Client *c)
{
    return c->mon->wx == c->x 
            && c->mon->wy == c->y
            && c->mon->ww == WIDTH(c)
            && c->mon->wh == HEIGHT(c);
}

void
clientmessage(XEvent *e)
{
    XClientMessageEvent *cme = &e->xclient;
    Atom msg = cme->message_type;
    long data0;
    long data1;
    long data2;
    long data3;
    long data4;
    Client *c = wintoclient(cme->window);
    if(!c) return;
    data0 = cme->data.l[0];
    data1 = cme->data.l[1];
    data2 = cme->data.l[2];
    data3 = cme->data.l[3];
    data4 = cme->data.l[4];
    if (msg == netatom[NetWMState])
    {
        updatewindowstate(c, data1, data0);
        if(data1 != data2) updatewindowstate(c, data2, data0);
    }
    else if (msg == netatom[NetActiveWindow])
    {   if (c != selmon->sel && !c->isurgent) seturgent(c, 1);
    }
    else if (msg == netatom[NetCloseWindow])
    {   killclient(c, Graceful);
    }
    else if (msg == netatom[NetMoveResizeWindow]) 
    {   resize(c, data1, data2, data3, data4, 1);
    }
    else if (msg == netatom[NetWMMinize])
    {
    }
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

    if (ev->window == root)
    {
        dirty = (sw != ev->width || sh != ev->height);
        sw = ev->width;
        sh = ev->height;
        if (updategeom() || dirty)
        {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next)
            {
                for (c = m->clients; c; c = c->next)
                    if (c->isfullscreen) resizeclient(c, m->mx, m->my, m->mw, m->mh);
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

    if ((c = wintoclient(ev->window)))
    {
        if (ev->value_mask & CWBorderWidth)
            c->bw = ev->border_width;
        else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange)
        {
            m = c->mon;
            if (ev->value_mask & CWX)
            {
                c->oldx = c->x;
                c->x = m->mx + ev->x;
            }
            if (ev->value_mask & CWY)
            {
                c->oldy = c->y;
                c->y = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth)
            {
                c->oldw = c->w;
                c->w = ev->width;
            }
            if (ev->value_mask & CWHeight)
            {
                c->oldh = c->h;
                c->h = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
            {   c->x = m->mx + ((m->mw >> 1) - (WIDTH(c) >> 1)); /* center in x direction */
            }
            if ((c->y + c->h) > m->my + m->mh && c->isfloating)
            {   c->y = m->my + ((m->mh >> 1) - (HEIGHT(c) >> 1)); /* center in y direction */
            }
            if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
            {   configure(c);
            }
            if (ISVISIBLE(c))
            {   XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            }
        }
        else
        {   configure(c);
        }
    }
    else
    {
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
    m->tagset[0]= m->tagset[1] = 1;
    m->mfact    = CFG_MONITOR_FACT;
    m->nmaster  = CFG_MASTER_COUNT;
    m->showbar  = CFG_SHOW_BAR;
    m->oshowbar = !CFG_SHOW_BAR;
    m->topbar   = CFG_TOP_BAR;
    m->lt[0]    = &layouts[CFG_DEFAULT_LAYOUT];
    m->lt[1]    = &layouts[CFG_DEFAULT_PREV_LAYOUT];
    m->tagmap   = ecalloc(LENGTH(tags), sizeof(Pixmap));
    m->isfullscreen = 0;
    m->lyt      = CFG_DEFAULT_LAYOUT;
    m->olyt     = CFG_DEFAULT_PREV_LAYOUT;
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
destroynotify(XEvent *e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;
    if ((c = wintoclient(ev->window)))
    {   unmanage(c, 1);
    }
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

        /* XCreateWindow(display, parent, x, y, width, height, border_width, depth, class, visual, valuemask, attributes); just reference */
        m->tabwin = XCreateWindow(dpy, root, posx, posy, maxwNeeded, maxhNeeded, 0, DefaultDepth(dpy, screen),
                                  None, DefaultVisual(dpy, screen),
                                  CWOverrideRedirect|CWBackPixmap|CWEventMask|CWBorderPixel|CWSaveUnder, &wa); /* create tabwin */
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
        schemecol = c != selmon->sel ? SchemeAltTab : SchemeAltTabSelect;
        /* for some reason this breaks when not using a variable and just putting in the thing */
        txtw = TEXTW(c->name) - lrpad;
        drw_setscheme(drw, scheme[schemecol]);

        if(CFG_ALT_TAB_TEXT_POS_X == 0) txtpad = 0;
        if(CFG_ALT_TAB_TEXT_POS_X == 1) txtpad = MAX(maxwNeeded - txtw, 0) >> 1;
        if(CFG_ALT_TAB_TEXT_POS_X == 2) txtpad = MAX(maxwNeeded - txtw, 0);

        drw_text(drw, 0, y, maxwNeeded, maxhNeeded, txtpad, c->name, 0);
        y += maxhNeeded;
    }
    drw_setscheme(drw, scheme[SchemeNorm]); /* set scheme back to normal */
    drw_map(drw, m->tabwin, 0, 0, CFG_ALT_TAB_MAX_WIDTH, CFG_ALT_TAB_MAX_HEIGHT);
}

void
drawbar(Monitor *m)
{
    int x, w, tw = 0;
    int boxw = (lrpad >> 4) + 2;
    unsigned int i, occ = 0, urg = 0;
    int tagselected;
    Client *c;
    if (!m->showbar) return;
    /* draw status first so it can be overdrawn by tags later */
    if (m == selmon)  /* only draw on selmon */
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
    for (i = 0; i < LENGTH(tags); ++i)
    {
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
    Client *c;
    unsigned int tabcnt;     /* tab count                    */
    unsigned int tabsz;      /* tab size                     */
    unsigned int iconspace;
    unsigned int boxh;       /* tiny floating box indicator h*/
    unsigned int boxw;       /* tiny floating box indicator w*/
    unsigned int curscheme;  /* current scheme               */
    unsigned int cc;         /* client counter               */
    unsigned int btpos;      /* current bartab positon x     */

    btpos = 0;
    cc = 0;
    tabcnt = 0;
    boxh = bh >> 2;
    boxw = bh >> 2;
    curscheme = SchemeBarTabInactive; /* init curscheme */
    /* set default scheme (blank canvas)*/
    drw_setscheme(drw, scheme[curscheme]);
    drw_rect(drw, x, 0, maxw, height, 1, 1);
    for(c = m->clients; c; c = c->next) tabcnt += !!ISVISIBLE(c);
    /* exit if no clients selected */
    if(!tabcnt) return;
    tabsz = maxw / tabcnt;
    /* draw only selmon->sel if tabs to small */
    if(tabsz < (unsigned int)TEXTW("..."))
    {
        iconspace = m->sel->icon ? m->sel->icw + CFG_ICON_SPACE : (unsigned int)lrpad >> 1;
        drw_text(drw, x, y, maxw, height, iconspace, m->sel->name, 0);
        if(m->sel->icon)
            drw_pic( drw, x, y + ((height - m->sel->ich) >> 1), m->sel->icw, m->sel->ich, m->sel->icon);
        if(m->sel->isfloating)
            drw_rect(drw, x, y + boxh, boxw, boxw, m->sel->isfixed, 0);
        return;
    }

    for(c = m->clients; c; c = c->next)
    {
        if(!ISVISIBLE(c)) continue;
        btpos = cc * tabsz;
        curscheme = c == m->sel ? SchemeBarTabActive  : SchemeBarTabInactive;
        iconspace = c->icon ? c->icw + CFG_ICON_SPACE : (unsigned int)lrpad >> 1;
        drw_setscheme(drw, scheme[curscheme]);
        drw_text(drw, x + btpos, y, tabsz, height, iconspace, c->name, 0);
        /* draw icon */
        if(c->icon)
            drw_pic(drw, x + btpos, y + ((height - c->ich) >> 1), c->icw, c->ich, c->icon);
        ++cc;
    }
}
void
drawbars(void)
{
    for (Monitor *m = mons; m; m = m->next) drawbar(m);
}

void
enternotify(XEvent *e)
{
    if(!CFG_HOVER_FOCUS) return;
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
        return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon)
    {
        unfocus(selmon->sel, 1);
        selmon = m;
    }
    else if (!c || c == selmon->sel) return;
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
    if (c)
    {
        if (c->mon != selmon) selmon = c->mon;
        if (c->isurgent) seturgent(c, 0);
        detachstack(c);
        attachstack(c);
        grabbuttons(c, 1);
        /* set new focused border first to avoid flickering */
        XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
        /* lastfocused may be us if another window was unmanaged */
        if (lastfocused && lastfocused != c)
            XSetWindowBorder(dpy, lastfocused->win, scheme[SchemeNorm][ColBorder].pixel);
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

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
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

pid_t
getwinpid(Window window)
{
    Atom actualType;
    int format;
    unsigned long nitems, bytesAfter;
    unsigned char *propData = NULL;

    Atom atom = XInternAtom(dpy, "_NET_WM_PID", False);
    if (XGetWindowProperty(dpy, window, atom, 0, 1, False, XA_CARDINAL,
                           &actualType, &format, &nitems, &bytesAfter, &propData) != Success) return -1;
    if (propData != NULL)
    {
        pid_t pid = *((pid_t*)propData);
        XFree(propData);
        return pid;
    }
    return -1;
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
    /* not reliable to use hints as some windows dont set them */
    /*
    XWMHints *hints;
    hints = XGetWMHints(dpy, win);
    if(!hints || !(hints->flags & IconPixmapHint))
    {
        if(hints) XFree(hints);
        return None;
    }
    XFree(hints);
    */
    if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType,
                           &real, &format, &n, &extra, (unsigned char **)&p) != Success) return None;

    if (n == 0 || format != bitformat) 
    {
        XFree(p);
        return None;
    }
    unsigned long *bstp = NULL;
    uint32_t w, h, sz;
    /* set icon size */
    {
        unsigned long *i;
        const unsigned long *end = p + n;
        uint32_t bstd = UINT32_MAX, d, m;
        for (i = p; i < end - 1; i += sz)
        {
            if ((w = *i++) >= unitbytecount  || (h = *i++) >= unitbytecount) {
                XFree(p);
                return None;
            }
            if ((sz = w * h) > end - i) break;
            if ((m = w > h ? w : h) >= CFG_ICON_SIZE && (d = m - CFG_ICON_SIZE) < bstd) {
                bstd = d;
                bstp = i;
            }
        }
        if (!bstp)
        {
            for (i = p; i < end - 1; i += sz)
            {
                if ((w = *i++) >= unitbytecount || (h = *i++) >= unitbytecount ) {
                    XFree(p);
                    return None;
                }
                if ((sz = w * h) > end - i) break;
                if ((d = CFG_ICON_SIZE - (w > h ? w : h)) < bstd) {
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

    if (XGetWindowProperty(dpy, w, netatom[WMState], 0L, 2L, False, netatom[WMState],
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
    {
        strncpy(text, (char *)name.value, size - 1);
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
    {
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
}

void
grabkeys(void)
{
    updatenumlockmask();
    {
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
}
void
grid(Monitor *m) 
{
    unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
    Client *c;
    for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
        n++;

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
        resize(c, cx, cy, cw - (c->bw << 1) + aw, ch - (c->bw << 1) + ah, False);
        ++i;
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
keypress(XEvent *e)
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    const int cleanstate = CLEANMASK(ev->state);
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0); /* deprecated dont care */
    for (i = 0; i < LENGTH(keys); ++i)
    {
        /* idk (x & 1) no */
        switch(keys[i].type)
        {
        case KeyPress:
            if (keysym == keys[i].keysym
                    && CLEANMASK(keys[i].mod) == cleanstate
                    && keys[i].func) {
                keys[i].func(&(keys[i].arg));
            }
            break;
        }
    }
}

void
keyrelease(XEvent *e)
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    const int cleanstate = CLEANMASK(ev->state);
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0); /* deprecated dont care */
    for (i = 0; i < LENGTH(keys); ++i)
    {
        /* idk (x & 1) no */
        switch(keys[i].type)
        {
        case KeyRelease:
            if (keysym == keys[i].keysym
                    && CLEANMASK(keys[i].mod) == cleanstate
                    && keys[i].func) {
                keys[i].func(&(keys[i].arg));
            }
            break;
        }
    }
}

/* handle new windows */
void
manage(Window w, XWindowAttributes *wa)
{
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    /* alloc enough memory for new client struct */
    c = poolgrab(pl, accnum);
    ++accnum;
    c->num = accnum;
    c->win = w;
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
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    updatemotifhints(c);
    XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    grabbuttons(c, 0);
    c->wasfloating = 0;
    if (!c->isfloating)
        c->isfloating = c->wasfloating = trans != None || c->isfixed;
    /* set floating if always on top */
    c->isfloating = c->isfloating || c->alwaysontop;
    c->pid = XGetPid(dpy, c->win);
    attach(c);
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + (sw << 1), c->y, c->w, c->h); /* some windows require this */
    setclientstate(c, NormalState);
    if (c->mon == selmon)
        unfocus(selmon->sel, 0);
    c->mon->sel = c;
    ++c->mon->cc;
    /* destroy any new clients if we past our client limit */
    if(accnum > CFG_MAX_CLIENT_COUNT)
    {
        XMapWindow(dpy, c->win);
        killclient(c, Safedestroy);
        unmanage(c, 1);
        return;
    }
    arrange(c->mon);
    /* check if selmon->fullscreen */
    setfullscreen(c, selmon->isfullscreen);
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
    int nx, ny;
    int nw, nh;
    int cbw; /* client border width */
    Client *c;
    int cc;
    nx = m->wx;
    ny = m->wy;
    cc = 0;
    for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
    {
        cbw = c->bw << 1;
        nw = m->ww - cbw;
        nh = m->wh - cbw;
        resize(c, nx, ny, nw, nh, 0);
        ++cc;
    }
    if(cc) snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", cc);
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

uint32_t
prealpha(uint32_t p) {
    uint8_t a = p >> 24u;
    uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
    uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
    return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
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
    else if ((c = wintoclient(ev->window)))
    {
        switch(ev->atom)
        {
        case XA_WM_TRANSIENT_FOR:
            if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
                    (c->isfloating = (wintoclient(trans)) != NULL))
                arrange(c->mon);
            break;
        case XA_WM_NORMAL_HINTS:
            c->hintsvalid = 0;
            break;
        case XA_WM_HINTS:
            updatewmhints(c);
            drawbars();
            break;
        default:
            break;
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
restoresession(void)
{
    /* restore stuff */
    Window cwin;
    unsigned int ctags;
    int cx, cy;
    int cox, coy;
    int cw, ch, cow, coh;
    int cofloating, cfloating;
    int cissel;
    int clyt;
    /* cleanup */
    Monitor *m;
    Client *c;
    Client **clients;
    Client *csel;
    Arg arg;
    int cc;

    const int MAX_LENGTH = 1024;
    const int CHECK_SUM = 14; /* equal to number of sscanf elements so %d %d %d %d would be a checksum of 4 (type doesnt matter ) */
    int check;
    FILE *fr;
    char str[MAX_LENGTH];
    char *nl; /* new line */
    int i;

    m = NULL;
    c = csel = NULL;
    fr = NULL;
    cx = cy = cox = coy = 0;
    cw = ch = cow = coh = 0;
    cofloating = cfloating = 0;
    cissel = 0;
    clyt = 0;
    i = 0;
    /* dont exit(1) as this function is non vital */
    if(!(m = selmon)) return;
    if(!m->clients) return;
    if (!(fr = fopen(SESSION_FILE, "r"))) return;
    for(c = m->clients; c; c = c->next) ++cc;
    if(!(clients = calloc(cc, sizeof(Client *)))) return;
    while(1)
    {
        /* reset values */
        cx = cy = cox = coy = 0;
        cw = ch = cow = coh = 0;
        cofloating = cfloating = 0;
        cissel = 0;
        clyt = 0;

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
                        " %d %d",
                        &cwin, &ctags,
                        &cx, &cy, &cox, &coy,
                        &cw, &ch, &cow, &coh,
                        &cofloating, &cfloating,
                        &cissel, &clyt
                    );
            /* CHECKSUM BTW IS MANUAL */
            if(check != CHECK_SUM) continue;
            for(c = m->clients; c; c = c->next) 
            {
                if(c->win == cwin) 
                {
                    c->tags = ctags;
                    c->oldx = cox;
                    c->oldy = coy;
                    c->oldw = cow;
                    c->oldh = coh;
                    c->wasfloating = cofloating;
                    if(cissel) csel = c;
                    if(cfloating)
                    {
                        c->isfloating = 1;
                        resize(c, cx, cy, cw, ch, 0);
                    }
                    setmonitorlayout(m, clyt);
                    /* restack order (This is skipped if we fuck up) */
                    int clientsFlag = 0;
                    for(i = 0; i < cc; ++i)
                    {
                        if(!clients[i]) 
                        {
                            clients[i] = c;
                            clientsFlag = 1;
                            break;
                        }
                    }
                    if(!clientsFlag) debug("WARNING: RESTORE_SESSION_TOO_MANY_CLIENTS");
                    break;
                }
            }
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
    }
    for(i = cc - 1; i >= 0; --i)
    {
        c = clients[i];
        if(!c || !c->win) continue;
        detach(c);
        attach(c);
    }
    if(csel && selmon->sel != csel) pop(csel);
    else focus(NULL);
    for(m = mons; m; m = m->next) arrange(m);

    /* toggle tag if arent in it */
    if(selmon->sel)
    {
        arg.ui = selmon->sel->tags;
        View(&arg);
    }
    goto CLEANUP;
CLEANUP:
    fclose(fr);
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
resizerequest(XEvent *e)
{
    /* popup windows sometimes need this */
    Client *c;
    XResizeRequestEvent *ev = &e->xresizerequest;
    c = wintoclient(ev->window);
    if(c) resize(c, c->x, c->y, ev->width, ev->height, 1);
    else  XResizeWindow(dpy, ev->window, ev->width, ev->height);
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
            if(c->isfloating) XRaiseWindow(dpy, c->win);
        }
        if(docked(c)) setfloating(c, 0);
    }
    if(m->sel->isfloating || m->sel->isfullscreen) XRaiseWindow(dpy, m->sel->win);
    for(c = m->slast; c; c = c->sprev) if(c->alwaysontop&& ISVISIBLE(c)) XRaiseWindow(dpy, c->win);
    for(c = m->slast; c; c = c->sprev) if(c->stayontop  && ISVISIBLE(c)) XRaiseWindow(dpy, c->win);
    for(c = m->slast; c; c = c->sprev) if(c->isurgent   && ISVISIBLE(c)) XRaiseWindow(dpy, c->win);
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

void
savesession(void)
{
    Client *c;
    FILE *fw = fopen(SESSION_FILE, "w");
    if(!fw) return;
    for (c = selmon->clients; c; c = c->next)
    {
        fprintf(fw, 
                "%lu %u"
                " %d %d %d %d"
                " %d %d %d %d"
                " %d %d"
                " %d %d"
                "\n"
                ,
                c->win, c->tags,
                c->x, c->y, c->oldx, c->oldy,
                c->w, c->h, c->oldw, c->oldh,
                c->wasfloating, c->isfloating,
                (selmon->sel == c), selmon->lyt
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
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                    || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1)) continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
                manage(wins[i], &wa);
        }
        for (i = 0; i < num; i++)
        {   /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa)) continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                    && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
                manage(wins[i], &wa);
        }
        if (wins) XFree(wins);
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

    XChangeProperty(dpy, c->win, netatom[WMState], netatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
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
        ev.xclient.message_type = netatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &ev);
    }
    return exists;
}

void
setdesktop()
{
    long data[] = { 0 };
    XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setdesktopnames()
{
    XTextProperty text;
    Xutf8TextListToTextProperty(dpy, (char **)tags, TAGSLENGTH, XUTF8StringStyle, &text);
    XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
}

void
setdesktopnum()
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
    sendevent(c, netatom[WMTakeFocus]);
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
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
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
setmonitorlayout(Monitor *m, int layout)
{
    m->lt[selmon->sellt] = (Layout *)&layouts[layout];
    m->olyt= m->lyt;
    m->lyt = layout;
}

void
setshowbar(Monitor *m, int show)
{
    m->oshowbar = m->showbar;
    m->showbar = show;
}

void
setup(void)
{
    XSetWindowAttributes wa;
    /* clean up any zombies immediately */
    sigchld(0);
    signal(SIGHUP, sighup);
    signal(SIGTERM, sigterm);
    /* setup pool (biggest risk of failure due to calloc) */
    setuppool();
    /* init screen */
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
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], XInternAtom(dpy, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *) WM_NAME, LENGTH(WM_NAME));
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) netatom, NetLast);
    setdesktopnum();
    setdesktop();
    setdesktopnames();
    setviewport();
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
setupcur()
{
    cursor[CurNormal]           = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResizeTopLeft]    = drw_cur_create_img(drw, "size_fdiag");
    cursor[CurResizeTopRight]   = drw_cur_create_img(drw, "size_bdiag");
    cursor[CurResizeBottomLeft] = drw_cur_create_img(drw, "size_fdiag");
    cursor[CurResizeBottomRight]= drw_cur_create_img(drw, "size_bdiag");
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
}

void
setuppool()
{
    pl = poolcreate(CFG_MAX_CLIENT_COUNT + 1, sizeof(Client));
}

void
setuptags()
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
setuptimezone()
{
    if(!CFG_AUTO_TIME_ZONE && CFG_TIME_ZONE)  setenv("TZ", CFG_TIME_ZONE, 1);
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
setviewport()
{
    long data[] = { 0, 0 };
    XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 2);
}

/* called when switching tags/workspaces */
void
showhide(Client *c)
{
    if (ISVISIBLE(c))
        XMoveWindow(dpy, c->win, c->x, c->y);
    else
        XMoveWindow(dpy, c->win, c->mon->mx - (WIDTH(c) << 1), c->y);
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
sigchld()
{
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("FATAL: CANNOT_INSTALL_SIGCHLD_HANDLER");

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

void
killclient(Client *c, int type)
{
    XEvent ev;
    pid_t pid;
    if(!c) return;
    if(!sendevent(c, netatom[WMDelete]))
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
            break;
        case Safedestroy:
            /* get window */
            XSelectInput(dpy, c->win, StructureNotifyMask);
            XKillClient(dpy, c->win);
            XUngrabServer(dpy);
            /* check if received DestroyNotify */
            XWindowEvent(dpy, c->win, StructureNotifyMask, &ev);
            XGrabServer(dpy);
            /* if it didnt it likely is frozen, Destroy it */
            if(ev.type != DestroyNotify) XDestroyWindow(dpy, c->win);
            break;
        }
        /* Make sure x receive the request */
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }

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
    c->oldx = c->x;
    c->oldy = c->y;
    c->oldw = c->w;
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
    c->oldy = c->y;
    c->oldw = c->w;
    c->oldh = c->h;
    resize(c, x, y, w, h, 1);
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
            resize(c, nx, ny, nw, nh, 0);
            if (my + HEIGHT(c) < (unsigned int)m->wh) my += HEIGHT(c);
        }
        else
        {
            h = (m->wh - ty) / (n - i);
            nx = m->wx + mw;
            ny = m->wy + ty;
            nw = m->ww - mw - (c->bw << 1);
            nh = h - (c->bw << 1);
            resize(c, nx, ny, nw, nh, 0);
            if (ty + HEIGHT(c) < (unsigned int)m->wh) ty += HEIGHT(c);
        }
    }
}

void
unfocus(Client *c, int setfocus)
{
    if (!c) return;
    grabbuttons(c, 0);
    lastfocused = c;
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
    poolfree(pl, c, c->num);
    focus(NULL);
    updateclientlist();
    arrange(m);
}

void
unmapnotify(XEvent *e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if ((c = wintoclient(ev->window)))
    {
        if (ev->send_event) setclientstate(c, WithdrawnState);
        else unmanage(c, 0);
    }
}

void
updatebars(void)
{
    Monitor *m;
    XSetWindowAttributes wa =
    {
        .override_redirect = True, /*patch */
        .background_pixmap = ParentRelative,
        .event_mask = ButtonPressMask|ExposureMask|PointerMotionMask
    };

    XClassHint ch = {WM_NAME, WM_NAME};
    for (m = mons; m; m = m->next)
    {
        if (!m->tagwin)
        {
            m->tagwin = XCreateWindow(dpy, root, m->wx, m->by + bh, m->mw / (CFG_TAG_PREVIEW_SCALE + !CFG_TAG_PREVIEW_SCALE),
                                      m->mh / (CFG_TAG_PREVIEW_SCALE + !CFG_TAG_PREVIEW_SCALE), 0, DefaultDepth(dpy, screen), CopyFromParent,
                                      DefaultVisual(dpy, screen), CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
            XDefineCursor(dpy, m->tagwin, cursor[CurNormal]->cursor);
            XUnmapWindow(dpy, m->tagwin);
        }
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
updateclientlist()
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
updatedesktop()
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
                c->bw = c->oldbw = CFG_BORDER_PX;
            else
                c->bw = c->oldbw = 0;

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
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if (c->name[0] == '\0') /* hack to mark broken clients */
        strcpy(c->name, BROKEN);
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
    data = !!data;
    if (state == netatom[NetWMModal])
    {
        if(toggle)
        {   setfloating(c, c->wasfloating);
            c->isurgent = !c->isurgent;
        }
        else
        {   setfloating(c, data);
            c->isurgent = data;
        }
    }
    else if (state == netatom[NetWMAlwaysOnTop])
    {
        if(toggle)
        {   setfloating(c, !c->wasfloating);
            c->alwaysontop = !c->alwaysontop;
        }
        else
        {   setfloating(c, data);
            c->alwaysontop = data;
        }
    }
    else if (state == netatom[NetWMStayOnTop])
    {
        if(toggle)
        {   setfloating(c, !c->isfloating);
            c->stayontop = !c->stayontop;
        }
        else
        {   setfloating(c, data);
            c->stayontop = data;
        }
    }
    else if (state == netatom[NetWMDemandAttention])
    {
        if(toggle)
        {   c->mon->sel == c ? unfocus(c, 1) : focus(c);
        }
        else
        {   data ? focus(c) : unfocus(c, 1);
        }
    }
    else if (state == netatom[NetWMFullscreen])
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
    }
    else if (state == netatom[NetWMMaximizedHorz])
    {
        if(toggle)
        {   MaximizeWindowHorizontal(NULL);
        }
        else if(data)
        {   maximizevert(c);
        }
        else
        {   resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 1);
        }
    }
    else if (state == netatom[NetWMMaximizedVert])
    {
        if(toggle)
        {   MaximizeWindowVertical(NULL);
        }
        else if(data)
        {   maximizehorz(c);
        }
        else
        {   resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 1);
        }
    }
    /*  else if (state == netatom[NetWMAbove])          {REPLACED BY NetWMAlwaysOnTop}      */
    else if (state == netatom[NetWMSticky])
    {
    }
    else if (state == netatom[NetWMBelow])
    {
    }
    else if (state == netatom[NetWMFocused])
    {
    }
    arrange(c->mon);
}

void
updatewindowtype(Client *c)
{
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);
    updatewindowstate(c, state, 1); /* _NET_WM_STATE_ADD */
    if (wtype == netatom[NetWMWindowTypeDesktop])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeDock])
    {   c->isfloating = c->stayontop = 1;
    }
    else if (wtype == netatom[NetWMWindowTypeToolbar])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeMenu])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeUtility])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeSplash])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeDialog])
    {   c->alwaysontop = c->isfloating = 1;
    }
    else if (wtype == netatom[NetWMWindowTypeDropdownMenu])
    {
    }
    else if (wtype == netatom[NetWMWindowTypePopupMenu])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeTooltip])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeNotification])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeCombo])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeDnd])
    {
    }
    else if (wtype == netatom[NetWMWindowTypeNormal])
    {
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
visiblitynotify(XEvent *e)
{
    /* issues with implementation as barwin causes all windwos to be visible for some reason? */
    return;
    /* to use set VisibilityChangeMask in bit mask manage() XSelectInput() */
    XVisibilityEvent *ev = &e->xvisibility;
    switch(ev->state)
    {
    case VisibilityPartiallyObscured:
    case VisibilityUnobscured:
        XMapWindow(dpy, ev->window);
        break;
    case VisibilityFullyObscured:
        XUnmapWindow(dpy, ev->window);
        break;
    }
}

void
winsetstate(Window win, long state)
{
    long data[] = { state, None };

    XChangeProperty(dpy, win, netatom[WMState], netatom[WMState], 32,
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

    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            if (c->win == w) return c;
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
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
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
    return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
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
