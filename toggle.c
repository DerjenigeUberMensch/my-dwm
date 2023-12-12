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

    if (!mons->next) return;
    if ((m = dirtomon(arg->i)) == selmon) return;
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
	}; /* prevent 0 division errors */
	focus(c);
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
    if (!selmon->sel) return;
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
    if(!selmon->sel) return;
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
    Time lasttime;

    float frametime;

    c = selmon->sel;
    if (!c) return;
    if (c->isfullscreen) return; /* no support moving fullscreen windows by mouse */
    restack(selmon);
    if (!c->isfloating || selmon->lt[selmon->sellt]->arrange) togglefloating(NULL);
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess) return;
    if (!getrootptr(&x, &y)) return;
    frametime = 1000 / (cfg.windowrate + !cfg.windowrate); /* prevent 0 division errors */
    lasttime = 0;
    ocx = c->x;
    ocy = c->y;
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
                if ((ev.xmotion.time - lasttime) <= frametime) continue;
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
            resize(c, nx, ny, c->w, c->h, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    if(dockablewindow(c))
    {
        /* workaround as setting c->isfloating c->ismax 0|1 doesnt work properly */
        maximize(selmon->wx, selmon->wy, selmon->ww - 2 * cfg.borderpx, selmon->wh - 2 * cfg.borderpx);
        c->oldx+=cfg.snap;
        c->oldy+=cfg.snap;
        c->ismax = 1; 
    }
    else c->ismax = 0;
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}


void restartdwm(const Arg *arg)
{
    savesession();
    restart();
}
void
quitdwm(const Arg *arg)
{
    savesession();
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
     * basew            Minimum client request size (wont go smaller)
     * baseh            See above
     * rszbase(w/h)     base window (w/h) when resizing assuming resize hints wasnt set / too small
     */
    int rcurx, rcury;
    int ocw, och;
    int nw, nh;
    int ocx, ocy;
    int nx, ny;
    int horizcorner, vertcorner;
    int basew, baseh;
    float frametime;

    int di;
    unsigned int dui;
    Window dummy;

    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;
    c = selmon->sel;

    /* client checks */
    if (!c || c->isfullscreen) return;/* no support resizing fullscreen windows by mouse */
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess) return;
    if (!XQueryPointer (dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui)) return;
    if(!getrootptr(&rcurx, &rcury)) return;
    if (!c->isfloating || selmon->lt[selmon->sellt]->arrange) togglefloating(NULL);
    restack(selmon);

    frametime = 1000 / (cfg.windowrate + !cfg.windowrate); /*prevent 0 division errors */
    ocw = c->w;
    och = c->h;
    ocx = c->x;
    ocy = c->y;
    horizcorner = nx < c->w >> 1;
    vertcorner  = ny < c->h >> 1;
    basew = MAX(c->basew, cfg.rszbasew);
    baseh = MAX(c->baseh, cfg.rszbaseh);
    do {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type)
        {
        case ConfigureRequest:
        case Expose:
        case MapRequest: handler[ev.type](&ev); break;
        case MotionNotify:
            if(cfg.windowrate != 0)
            {
                if ((ev.xmotion.time - lasttime) <= frametime)
                    continue;
                lasttime = ev.xmotion.time;
            }
            /* leave as is cpu will always predict branch */
            nw = horizcorner ? MAX(ocw - (ev.xmotion.x - rcurx), basew) : MAX(ocw + (ev.xmotion.x - rcurx), basew);
            nh = vertcorner  ? MAX(och - (ev.xmotion.y - rcury), baseh) : MAX(och + (ev.xmotion.y - rcury), baseh);
            nx = horizcorner ? ocx + ocw - nw: c->x;
            ny = vertcorner  ? ocy + och - nh: c->y;
            resize(c, nx, ny, nw, nh, 1);
            break;
        }
    } while (ev.type != ButtonRelease);
    /* add if w + x > monx || w + x < 0 resize */
    if(c->w > c->mon->ww)
        togglehorizontalmax(NULL);
    if(c->h > c->mon->wh)
        toggleverticalmax(NULL);
    if(dockablewindow(c))
    {
        /* workaround as setting c->isfloating c->ismax 0|1 doesnt work properly */
        maximize(selmon->wx, selmon->wy, selmon->ww - 2 * cfg.borderpx, selmon->wh - 2 * cfg.borderpx);
        c->oldx+=cfg.snap;
        c->oldy+=cfg.snap;
        c->ismax = 1;
    }
    else c->ismax = 0;

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
    Monitor *m;
    m = selmon;

    if(!m) return;
    setclientlayout(m, arg->i);
    if (m->sel) arrange(m);
    else drawbar(m);
}

/* arg > 1.0 will set mfact absolutely */
    void
setmfact(const Arg *arg)
{
    float f;

    if (!arg || !selmon->lt[selmon->sellt]->arrange) return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95) return;
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
togglemaximize(const Arg *arg) 
{
    maximize(selmon->wx, selmon->wy, selmon->ww - 2 * cfg.borderpx, selmon->wh - 2 * cfg.borderpx);
    selmon->sel->isfloating = 0;
}


void
toggleverticalmax(const Arg *arg) {
    maximize(selmon->sel->x, selmon->wy, selmon->sel->w, selmon->wh - 2 * cfg.borderpx);
    selmon->sel->ismax = 0; /* no such thing as vertmax being fully maxed */
}

void
togglehorizontalmax(const Arg *arg) {
    maximize(selmon->wx, selmon->sel->y, selmon->ww - 2 * cfg.borderpx, selmon->sel->h);
    selmon->sel->ismax = 0; /* no such thing as horzmax being fully maxed */
}
void
alttabstart(const Arg *arg)
{
    selmon->altsnext = NULL;
    if (selmon->tabwin)
        alttabend();

    if (selmon->isAlt) {
        alttabend();
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
            drawtab(m->nTabs, 1, m);
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
            alttab();
            if (grabbed == 0) {
                alttabend();
            } else {
                while (grabbed) {
                    XNextEvent(dpy, &event);
                    if (event.type == KeyPress || event.type == KeyRelease) {
                        if (event.type == KeyRelease && event.xkey.keycode == cfg.tabmodkey) { /* if super key is released break cycle */
                            break;
                        } else if (event.type == KeyPress) {
                            if (event.xkey.keycode == cfg.tabcyclekey) {/* if XK_s is pressed move to the next window */
                                alttab();
                            }
                        }
                    }
                }

                c = selmon->sel;
                alttabend(); /* end the alt-tab functionality */
                /* XUngrabKeyboard(display, time); just a reference */
                XUngrabKeyboard(dpy, CurrentTime); /* stop taking all input from keyboard */
                focus(c);
                restack(selmon);
            }
        } else {
            alttabend(); /* end the alt-tab functionality */
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
    if (!selmon->sel || !mons->next) return;
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
    if (!c) return;
    if (c->isfullscreen) return; /* no support for fullscreen windows */
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
    if(!m->sel) return; /* no client focused */
    m->isfullscreen = !m->isfullscreen;
    winmode = m->isfullscreen;
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

    if (!selmon->sel) return;
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
    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags]) return;
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

    if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating) return;
    if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next))) return;
    pop(c);
}
