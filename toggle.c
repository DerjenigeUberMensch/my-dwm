/* user functions */

#include "toggle.h"
void
tester(const Arg *arg)
{
}


void
focusmon(const Arg *arg)
{
    Monitor *m;

    if (!mons->next)
        return;
    if ((m = dirtomon(arg->i)) == selmon)
        return;
    unfocus(selmon->sel, 0);
    selmon = m;
    focus(NULL);
}
void
focusnext(const Arg *arg) {
	Monitor *m;
	Client *c;
	m = selmon;
	c = m->sel;

	if (arg->i) {
		if (c->next)
			c = c->next;
		else
			c = m->clients;
	} else {
		Client *last = c;
		if (last == m->clients)
			last = NULL;
		for (c = m->clients; c->next != last; c = c->next);
	}
	focus(c);
	return;
}

void
incnmaster(const Arg *arg)
{
    selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
    arrange(selmon);
}

void
killclient(const Arg *arg)
{
    if (!selmon->sel)
        return;
    if (!sendevent(selmon->sel, wmatom[WMDelete])) {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, selmon->sel->win);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
}
void
forcekillclient(const Arg *arg) //Destroys window disregarding any errors that may occur in the process
{
    if(!selmon->sel)
        return;
    if(!sendevent(selmon->sel, wmatom[WMDelete]))
    {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XDestroyWindow(dpy, selmon->sel->win); //A destructive ation that can cause an BadWindow error to occur
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);

    }
}
void
movemouse(const Arg *arg)
{
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
        return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
        return;
    if (!getrootptr(&x, &y))
        return;
    do {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type) {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if(cfg.windowrate != 0)
            {
                if ((ev.xmotion.time - lasttime) <= (1000 / cfg.windowrate))
                    continue;
                lasttime = ev.xmotion.time;
            }
            nx = ocx + (ev.xmotion.x - x);
            ny = ocy + (ev.xmotion.y - y);
            if (abs(selmon->wx - nx) < cfg.snap)
                nx = selmon->wx;
            else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < cfg.snap)
                nx = selmon->wx + selmon->ww - WIDTH(c);
            if (abs(selmon->wy - ny) < cfg.snap)
                ny = selmon->wy;
            else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < cfg.snap)
                ny = selmon->wy + selmon->wh - HEIGHT(c);
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
                    && (abs(nx - c->x) > cfg.snap || abs(ny - c->y) > cfg.snap))
                togglefloating(NULL);
            if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
                resize(c, nx, ny, c->w, c->h, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    if(dockablewindow(c))
        c->isfloating = 0;
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}
void restartdwm(const Arg *arg)
{
    restart();
}
void
quitdwm(const Arg *arg)
{
    quit();
}

void
resizemouse(const Arg *arg)
{
    /* rcurx, rcury     old x/y Pos for root cursor
     * ocw/och          old client width/height
     * nw/nh            new width/height
     * ocx/ocy          old client posX/posY
     * nx/ny            new posX/posY
     * horiz/vert       check if top left/bottom left of screen
     * di,dui,dummy     holder vars to pass check (useless aside from check)
     */
    int rcurx, rcury;
    int ocw, och;
    int nw, nh;
    int ocx, ocy;
    int nx, ny;
    int horizcorner, vertcorner;
    int di;
    unsigned int dui;
    Window dummy;

    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    /* client checks */
    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
        return;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
        return;
    if (!XQueryPointer (dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
        return;
    if(!getrootptr(&rcurx, &rcury))
        return;
    restack(selmon);

    ocw = c->w;
    och = c->h;
    ocx = c->x;
    ocy = c->y;

    horizcorner = nx < c->w >> 1;
    vertcorner  = ny < c->h >> 1;
    do {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type)
        {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if(cfg.windowrate != 0)
            {
                if ((ev.xmotion.time - lasttime) <= (1000 / cfg.windowrate))
                    continue;
            }
            lasttime = ev.xmotion.time;

            nw = horizcorner ? MAX(ocw - (ev.xmotion.x - rcurx), 1) : MAX(ocw + (ev.xmotion.x - rcurx), 1);
            nh = vertcorner  ? MAX(och - (ev.xmotion.y - rcury), 1) : MAX(och + (ev.xmotion.y - rcury), 1);

            nx = horizcorner ? ocx + ocw - nw: c->x;
            ny = vertcorner  ? ocy + och - nh: c->y;

            if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
                    && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
            {
                if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
                        && (abs(nw - c->w) > cfg.snap || abs(nh - c->h) > cfg.snap))
                    togglefloating(NULL);
            }
            if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
                resize(c, nx, ny, nw, nh, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    if(dockablewindow(c))
        c->isfloating = 0;
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}
void
setlayout(const Arg *arg)
{
    if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
        selmon->sellt ^= 1;
    if (arg && arg->v)
        selmon->lt[selmon->sellt] = (Layout *)arg->v;
    strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
    if (selmon->sel)
        arrange(selmon);
    else
        drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
    float f;

    if (!arg || !selmon->lt[selmon->sellt]->arrange)
        return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95)
        return;
    selmon->mfact = f;
    arrange(selmon);
}

void
spawn(const Arg *arg)
{
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char **)arg->v)[0], (char **)arg->v);
        die("FATAL ERROR: EXECVP '%s' FAILED:", ((char **)arg->v)[0]);
    }
}
void
togglemaximize(const Arg *arg) {
    maximize(selmon->wx, selmon->wy, selmon->ww - 2 * cfg.borderpx, selmon->wh - 2 * cfg.borderpx);
    selmon->sel->isfloating = 0;
}

void
toggleverticalmax(const Arg *arg) {
    maximize(selmon->sel->x, selmon->wy, selmon->sel->w, selmon->wh - 2 * cfg.borderpx);
}

void
togglehorizontalmax(const Arg *arg) {
    maximize(selmon->wx, selmon->sel->y, selmon->ww - 2 * cfg.borderpx, selmon->sel->h);
}
void
alttabstart(const Arg *arg)
{
    selmon->altsnext = NULL;
    if (selmon->tabwin)
        altTabEnd();

    if (selmon->isAlt) {
        altTabEnd();
    } else {
        selmon->isAlt = 1;
        selmon->altTabN = 0;

        Client *c;
        Monitor *m = selmon;

        m->nTabs = 0;
        for(c = m->clients; c; c = c->next) { /* count clients */
            if(!ISVISIBLE(c)) continue;
            /* if (HIDDEN(c)) continue; uncomment if you're using awesomebar patch */

            ++m->nTabs;
        }

        if (m->nTabs > 0) {
            m->altsnext = (Client **) malloc(m->nTabs * sizeof(Client *));

            int listIndex = 0;
            for(c = m->stack; c; c = c->snext) { /* add clients to the list */
                if(!ISVISIBLE(c)) continue;
                /* if (HIDDEN(c)) continue; uncomment if you're using awesomebar patch */

                m->altsnext[listIndex++] = c;
            }

            drawTab(m->nTabs, 1, m);

            struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };

            /* grab keyboard (take all input from keyboard) */
            int grabbed = 1;
            for (int i = 0; i < 1000; i++) {
                if (XGrabKeyboard(dpy, DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
                    break;
                nanosleep(&ts, NULL);
                if (i == 1000 - 1)
                    grabbed = 0;
            }

            XEvent event;
            altTab();
            if (grabbed == 0) {
                altTabEnd();
            } else {
                while (grabbed) {
                    XNextEvent(dpy, &event);
                    if (event.type == KeyPress || event.type == KeyRelease) {
                        if (event.type == KeyRelease && event.xkey.keycode == cfg.tabmodkey) { /* if super key is released break cycle */
                            break;
                        } else if (event.type == KeyPress) {
                            if (event.xkey.keycode == cfg.tabcyclekey) {/* if XK_s is pressed move to the next window */
                                altTab();
                            }
                        }
                    }
                }

                c = selmon->sel;
                altTabEnd(); /* end the alt-tab functionality */
                /* XUngrabKeyboard(display, time); just a reference */
                XUngrabKeyboard(dpy, CurrentTime); /* stop taking all input from keyboard */
                focus(c);
                restack(selmon);
            }
        } else {
            altTabEnd(); /* end the alt-tab functionality */
        }
    }
}

void
tag(const Arg *arg)
{
    if (selmon->sel && arg->ui & TAGMASK) {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        arrange(selmon);
    }
}

void
tagmon(const Arg *arg)
{
    if (!selmon->sel || !mons->next)
        return;
    sendmon(selmon->sel, dirtomon(arg->i));
}

void
togglebar(const Arg *arg)
{
    selmon->showbar = !selmon->showbar;
    updatebarpos(selmon);
    XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
    arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
    Client *c; 
    int togglefloat;

    c = selmon->sel;
    togglefloat = 0;
    if (!c)
        return;
    if (c->isfullscreen) /* no support for fullscreen windows */
        return;
    togglefloat = !selmon->sel->isfloating || selmon->sel->isfixed;
    if(togglefloat)
    {
        c->isfloating = togglefloat;
        resize(c, c->x, c->y, c->w, c->h, 0);
    }
    arrange(selmon);
}

void
togglefullscr(const Arg *arg)
{
    Client *c;
    Monitor *m;
    int winmode; /* if fullscreen or not */

    m = selmon;
    if(!m->sel) /* no client focused */
        return;

    winmode = !m->sel->isfullscreen;
    for (c = m->clients; c; c = c->next)
    {
        if(!ISVISIBLE(c)) continue;
        setfullscreen(c, winmode);
    }
    focus(m->sel);
    restack(m);
}

void
toggletag(const Arg *arg)
{
    unsigned int newtags;

    if (!selmon->sel)
        return;
    newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
    if (newtags) {
        selmon->sel->tags = newtags;
        focus(NULL);
        arrange(selmon);
    }
}

void
toggleview(const Arg *arg)
{
    unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

    if (newtagset) {
        selmon->tagset[selmon->seltags] = newtagset;
        focus(NULL);
        arrange(selmon);
    }
}

void
view(const Arg *arg)
{
    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
        return;
    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK)
        selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
    focus(NULL);
    arrange(selmon);
}

void
zoom(const Arg *arg)
{
    Client *c = selmon->sel;

    if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
        return;
    if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
        return;
    pop(c);
}
/* Selects for the view of the focused window. The list of tags */
/* to be displayed is matched to the focused window tag list. */
void
winview(const Arg* arg){
	Window win, win_r, win_p, *win_c;
	unsigned nc;
	int unused;
	Client* c;
	Arg a;

	if (!XGetInputFocus(dpy, &win, &unused)) return;
	while(XQueryTree(dpy, win, &win_r, &win_p, &win_c, &nc)
	      && win_p != win_r) win = win_p;

	if (!(c = wintoclient(win))) return;

	a.ui = c->tags;
	view(&a);
}

