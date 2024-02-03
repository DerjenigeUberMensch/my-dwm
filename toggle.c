
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "drw.h"
#include "util.h"
#include "winutil.h"
#include "dwm.h"
#include "config.h"
#include "toggle.h"
#include "events.h"


/*
 * Making your own stuff with Arg
 * Get your Function if it doesnt require an Arg simply pass NULL:
 * Example: ResizeWindow(NULL);
 * However if it does then pass Arg and its data type see dwm.h
 * Example:
 * Arg arg;
 * arg.i = 10;
 * SomeToggleFunction(&arg);
 */

/* Mostly a testing function */
void
UserStats(const Arg *arg)
{
    static int somenum = 1;
    setsticky(selmon->sel, somenum);
    somenum ^= 1;
}

/* Switch to a monitor based on the argument int arg i */
void
FocusMonitor(const Arg *arg)
{
    Monitor *m;

    if (!mons->next) return;
    if ((m = dirtomon(arg->i)) == selmon) return;
    unfocus(selmon->sel, 0);
    selmon = m;
    focus(NULL);
}

/* Changes the max number of master windows possible */
void
ChangeMasterWindow(const Arg *arg)
{
    selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
    arrange(selmon);
}

/* Kills the current window */
void
KillWindow(const Arg *arg)
{
    killclient(selmon->sel, Graceful);
}

/* Attempts to kill the current window directly instead of just sending a signal and waiting for the window to respond */
void
TerminateWindow(const Arg *arg)
{
    killclient(selmon->sel, Destroy);
}

/* keybind to move the current window where the mouse cursor is */
void
DragWindow(const Arg *arg) /* movemouse */
{
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime;

    const int frametime = 1000 / (CFG_WIN_RATE + !CFG_WIN_RATE); /* prevent 0 division errors */

    c = selmon->sel;
    if (!c) return;
    if (c->isfullscreen) return; /* no support moving fullscreen windows by mouse */
    restack(selmon);
    if (!c->isfloating || selmon->lt[selmon->sellt]->arrange) ToggleFloating(NULL);
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess) return;
    if (!getrootptr(&x, &y)) return;
    lasttime = 0;
    ocx = c->x;
    ocy = c->y;
    XRaiseWindow(dpy, c->win);
    do
    {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type)
        {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if(CFG_WIN_RATE != 0)
            {
                if ((ev.xmotion.time - lasttime) <= frametime) continue;
                lasttime = ev.xmotion.time;
            }
            nx = ocx + (ev.xmotion.x - x);
            ny = ocy + (ev.xmotion.y - y);
            /* snap to window area */
            if (abs(selmon->wx - nx) < CFG_SNAP)
                nx = selmon->wx;
            else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < CFG_SNAP)
                nx = selmon->wx + selmon->ww - WIDTH(c);
            if (abs(selmon->wy - ny) < CFG_SNAP)
                ny = selmon->wy;
            else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < CFG_SNAP)
                ny = selmon->wy + selmon->wh - HEIGHT(c);

            resize(c, nx, ny, c->w, c->h, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    /* prevent restacking moving the window back */
    if(!docked(c)) setfloating(c, 1);
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) 
    {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
    restack(selmon);
}

/* restarts dwm */
void
Restart(const Arg *arg)
{
    savesession();
    restart();
}

/* quits dwm */
void
Quit(const Arg *arg)
{
    savesession();
    quit();
}

/* resizes the current window based on mouse position */
void
ResizeWindow(const Arg *arg) /* resizemouse */
{
    /* rcurx, rcury     old x/y Pos for root cursor
     * ocw/och          old client width/height
     * nw/nh            new width/height
     * ocx/ocy          old client posX/posY
     * nx/ny            new posX/posY
     * horiz/vert       check if top left/bottom left of screen
     * di,dui,dummy     holder vars to pass check (useless aside from check)
     * basew            Minimum client request size (wont go smaller)
     * baseh            See above
     */
    int rcurx, rcury;
    int ocw, och;
    int nw, nh;
    int ocx, ocy;
    int nx, ny;
    int horiz, vert;
    int basew, baseh;
    int incw, inch;
    const float frametime = 1000 / (CFG_WIN_RATE + !CFG_WIN_RATE); /*prevent 0 division errors */

    unsigned int dui;
    Window dummy;

    Cursor cur;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;
    c = selmon->sel;

    /* client checks */
    if (!c || c->isfullscreen || c->isfixed) return;
    if (!XQueryPointer (dpy, c->win, &dummy, &dummy, &rcurx, &rcury, &nx, &ny, &dui)) return;
    horiz = nx < c->w >> 1 ? -1 : 1;
    vert  = ny < c->h >> 1 ? -1 : 1;
    if(horiz == -1)
    {
        if(vert == -1) cur = cursor[CurResizeTopLeft]->cursor;
        else cur = cursor[CurResizeBottomRight]->cursor;
    }
    else
    {
        if(vert == -1) cur = cursor[CurResizeTopRight]->cursor;
        else cur = cursor[CurResizeBottomLeft]->cursor;
    }
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cur, CurrentTime) != GrabSuccess) return;
    if (!c->isfloating || selmon->lt[selmon->sellt]->arrange) ToggleFloating(NULL);
    restack(selmon);
    ocw = c->w;
    och = c->h;
    ocx = c->x;
    ocy = c->y;
    incw = c->incw;
    inch = c->inch;
    basew = MAX(c->minw ? c->minw : c->basew, CFG_RESIZE_BASE_WIDTH);
    baseh = MAX(c->minh ? c->minh : c->baseh, CFG_RESIZE_BASE_HEIGHT);
    do
    {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type)
        {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if(CFG_WIN_RATE != 0)
            {
                if ((ev.xmotion.time - lasttime) <= frametime)
                    continue;
                lasttime = ev.xmotion.time;
            }
            /* calculate */
            nw = ocw + horiz * (ev.xmotion.x - rcurx);
            nh = och + vert * (ev.xmotion.y - rcury);
            /* clamp */
            nw = MAX(nw, basew);
            nh = MAX(nh, baseh);
            if(nw < incw) nw = ocw;
            if(nh < inch) nh = ocw;
            /* flip sign if -1 else default to 0 */
            nx = ocx + !~horiz * (ocw - nw);
            ny = ocy + !~vert  * (och - nh);
            resize(c, nx, ny, nw, nh, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    if(WIDTH(c) > c->mon->ww)
        maximizehorz(c);
    if(HEIGHT(c) + bh * c->mon->showbar >= c->mon->wh)
        maximizevert(c);
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) 
    {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
    restack(selmon);
}

/* sets the window layout based on a enum in dwm.h -> Grid, Floating, Monocle, Tiled */
void
SetWindowLayout(const Arg *arg)
{
    Monitor *m;
    Client *mnext;
    m = selmon;

    if(!m || m->isfullscreen) return;
    setmonlyt(m, arg->i);
    arrangemon(m);
    if(m->sel) 
    {       
        mnext = nextvisible(m->clients);
        if(m->sel != mnext) { detach(m->sel); attach(m->sel); }
        arrange(m);
    }
    else 
    {   /* update the way the bar looks to show "responsiveness" */
        drawbar(m);
    }
}

/* Sets the size of the master window 0.0 -> 1.0 for the Tiled Layout
 * where 1 is just monocle with extra steps */
void
SetMonitorFact(const Arg *arg)
{
    float f;

    if (!arg || !selmon->lt[selmon->sellt]->arrange) return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95) return;
    selmon->mfact = f;
    arrange(selmon);
}

/* Creates a window Usage:
 * char *variablename[] = {"executablename", NULL}
 * if you want to add arguments to the executable you do so after so like this:
 * char *variablename[] = {"executablename", "argument1", NULL}
 * there is no limit to the amount of args that you can use BUT you must end it with NULL
 *
 * { KeyPress,         SUPER,                      XK_n,       UserStats,       {variablename} },
 * After that if you in keybinds simply pass it in the Argument brackets:       ^^^^^^^^^^^^^ 
 *
 * if you are making you want to use it in code then make a Arg
 * Arg arg;
 * arg.v = variablename;
 * SpawnWindow(&arg);
 */

/* Spawns a window based on arguments provided */
void
SpawnWindow(const Arg *arg)
{
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char **)arg->v)[0], (char **)arg->v);
        die("FATAL ERROR: EXECVP '%s' FAILED:", ((char **)arg->v)[0]);
    }
}

/* Maximizes the currently selected window */
void
MaximizeWindow(const Arg *arg)
{
    Client *c = selmon->sel;
    if(c && !c->isfixed && !c->mon->isfullscreen)
    {
        if(docked(c))
        {   
            setfloating(c, 1);
            /* assume client has never been moved */
            if(c->x == c->oldx && c->y == c->oldy && c->oldw == c->w && c->oldh == c->h)
            {
                c->oldx += CFG_SNAP;
                c->oldy += CFG_SNAP;
            }
            resize(c, c->oldx, c->oldy, c->oldw, c->oldh, 1);
        }
        else
        {   
            setfloating(c, 0);
            maximize(c);
        }
        arrange(selmon);
    }
}

/* Maximizes a window vertically */ 
void
MaximizeWindowVertical(const Arg *arg) 
{
    Client *c = selmon->sel;
    if(c && !c->isfixed) 
    {
        if(dockedvert(c))
        {   
            if(!c->isfloating) setfloating(c, 1);
            resize(c, c->w, c->y, c->oldw, c->h, 1);
        }
        else
        {   
            setfloating(c, 0);
            maximizevert(c);
        }
        arrange(selmon);
    }
}

/* Maximizes a window horizontally */
void
MaximizeWindowHorizontal(const Arg *arg) 
{
    Client *c = selmon->sel;
    if(c && !c->isfixed)
    {
        if(dockedhorz(c))
        {   
            if(!c->isfloating) setfloating(c, 1);
            resize(c, c->x, c->oldy, c->w, c->oldh, 1);
        }
        else
        {   
            setfloating(c, 0);
            maximizehorz(c);
        } 
        arrange(selmon);
    }
}

/* Switches to the next window visible
 * then makes it so that both windows are next to each other 
 * producing a somewhat simple (in theory) alttab function
 */

/* Switches to the next visible window based on user input */
void
AltTab(const Arg *arg)
{
    int grabbed;
    Monitor *m;
    Client *tabnext;
    XEvent event;

    m = selmon;
    grabbed = 0;

    if(!m->sel) focus(NULL);
    if(!m->sel) return;
    if(!m->isfullscreen) drawalttab(1, m);
    tabnext = alttab(1);
    drawalttab(0, m);

    /* grab keyboard (input) grab mouse (prevent detaching of over clients) */
    if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
    {   grabbed = 1;
    }
    else 
    { 
        alttabend(tabnext); 
        return; 
    }

    if(XGrabPointer(dpy, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, 
                GrabModeAsync, GrabModeAsync, None, cursor[CurNormal]->cursor, CurrentTime) == GrabSuccess) {/**/}
    else
    {
        XUngrabKeyboard(dpy, CurrentTime); 
        alttabend(tabnext); 
        return; 
    }

    while (grabbed && running)
    {
        XNextEvent(dpy, &event);
        switch(event.type)
        {
        case KeyPress:
            if(event.xkey.keycode == CFG_ALT_TAB_CYCLE_KEY) 
            {
                tabnext = alttab(0);
                if(!m->isfullscreen) drawalttab(0, m);
            }
            break;
        case KeyRelease:
            grabbed = event.xkey.keycode != CFG_ALT_TAB_SWITCH_KEY;
            break;
        case PropertyNotify:
            /* update window if changes */
            if(event.xproperty.window == selmon->tabwin)
            {
                if(event.xproperty.atom == XA_WM_NAME || event.xproperty.atom == netatom[NetWMName])
                {
                    XUnmapWindow(dpy, selmon->tabwin);
                    XDestroyWindow(dpy, selmon->tabwin);
                    drawalttab(1, selmon);
                }
            }
            handler[event.type](&event);
        default: /* still handle events */
            if (handler[event.type]) handler[event.type](&event);
        }
    }

    alttabend(tabnext); /* end the alt-tab functionality */
    XUngrabKeyboard(dpy, CurrentTime);
    XUngrabPointer(dpy, CurrentTime);
}

/* Tags a window AKA makes it visible in another desktop thing or "tag" */
void
TagWindow(const Arg *arg)
{
    if (selmon->sel && arg->ui & TAGMASK)
    {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        arrange(selmon);
    }
}

/* Tags a monitor */
void
TagMonitor(const Arg *arg)
{
    if (!selmon->sel || !mons->next) return;
    sendmon(selmon->sel, dirtomon(arg->i));
}

/* Toggles if we show the Status bar or not */
void
ToggleStatusBar(const Arg *arg)
{
    setshowbar(selmon, !selmon->showbar);
    updatebarpos(selmon);
    XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
    arrange(selmon);
}

/* This toggles the floating layout */
void
ToggleFloating(const Arg *arg)
{
    Client *c;
    int togglefloat;

    c = selmon->sel;
    togglefloat = 0;
    if (!c) return;
    if (c->isfullscreen) return; /* no support for fullscreen windows */
    togglefloat = !selmon->sel->isfloating || selmon->sel->isfixed;
    if(togglefloat)
    {
        setfloating(c, togglefloat);
        resize(c, c->x, c->y, c->w, c->h, 0);
    }
    arrange(selmon);
}

/* Toggles fullscreen mode for all windows in current tag */
void
ToggleFullscreen(const Arg *arg)
{
    Client *c;
    Monitor *m;
    m = selmon;
    m->isfullscreen = !m->isfullscreen;
    for (c = m->clients; c; c = c->next) 
    {
        if(!ISVISIBLE(c) || c->alwaysontop || c->stayontop) continue;
        setfullscreen(c, m->isfullscreen);
    }
    if(m->isfullscreen)  setmonlyt(m, Monocle);
    else setmonlyt(m, m->olyt);
    ToggleStatusBar(NULL);
    arrange(m);
}

/* Probably toggles the tag selected IDK */
void
ToggleTag(const Arg *arg)
{
    unsigned int newtags;

    if (!selmon->sel) return;
    newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
    if (newtags)
    {
        selmon->sel->tags = newtags;
        focus(NULL);
        arrange(selmon);
    }
    updatedesktop();
}

/* This is used in keybinds default SUPER + (1-9)
 * And essential just changes the tag to the one specified in the number
 * AKA 1 << TAG
 * This is really only a "User function" see View for devs
 */
void
ToggleView(const Arg *arg)
{
    unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

    if (newtagset)
    {
        selmon->tagset[selmon->seltags] = newtagset;
        focus(NULL);
        arrange(selmon);
        updatedesktop();
    }
}

/* Switches to the tag number specified in base power of 2^x
 * Example:
 * tag1,tag2,tag3,tag4,tag5,tag6,tag7,tag8,tag9
 * {1,  2,   4,   8,   16,  32,  64,  128, 256}
 */
void
View(const Arg *arg)
{
    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags]) return;
    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK) selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
    focus(NULL);
    arrange(selmon);
    updatedesktop();
}

/* Idk what this does */
void
Zoom(const Arg *arg)
{
    Client *c = selmon->sel;

    if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating) return;
    if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next))) return;
    pop(c);
}
