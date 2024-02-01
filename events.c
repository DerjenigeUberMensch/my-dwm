
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "dwm.h"
#include "config.h"
#include "keybinds.h"

#include "events.h"
    
void 
(*handler[LASTEvent]) (XEvent *) = 
{
    /* Keyboard */
    [KeyPress] = keypress,
    [KeyRelease] = keyrelease,
    /* Pointer */
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [MotionNotify] = motionnotify,
    /* Win Crossing */
    [EnterNotify] = enternotify,
    [LeaveNotify] = leavenotify,
    /* Input focus */
    [FocusIn] = focusin,
    [FocusOut] = focusout,
    /* keymap state notification */
    [KeymapNotify] = keymapnotify,
    /* Exposure */
    [Expose] = expose,
    [GraphicsExpose] = graphicsexpose,
    [NoExpose] = noexpose,
    /* structure control */
    [ConfigureRequest] = configurerequest,
    [CirculateRequest] = circulaterequest,
    [MapRequest] = maprequest,
    [ResizeRequest] = resizerequest,

    /* window state notify */
    [CirculateNotify] = circulatenotify,
    [ConfigureNotify] = configurenotify,
    [CreateNotify] = createnotify,
    [DestroyNotify] = destroynotify,
    [GravityNotify] = gravitynotify,
    [MapNotify] = mapnotify,
    [MappingNotify] = mappingnotify,
    [UnmapNotify] = unmapnotify,
    [VisibilityNotify] = visibilitynotify,
    /* colour map state notify */
    [ColormapNotify] = colormapnotify,
    /* client communication */
    [ClientMessage] = clientmessage,
    [PropertyNotify] = propertynotify,
    [SelectionClear] = selectionclear,
    [SelectionNotify] = selectionnotify,
    [SelectionRequest] = selectionrequest,
};

static void 
updateclicktype(XButtonEvent *e, unsigned int *click, Arg *arg)
{
    int i, x;
    Client *c;
    *click = ClkRootWin;
    if(e->window == selmon->barwin)
    {
        i = x = 0;
        do x += TEXTW(tags[i]);
        while (e->x >= x && ++i < LENGTH(tags));
        if (i < LENGTH(tags))
        {
            *click = ClkTagBar;
            arg->ui = 1 << i;
            /* hide preview if we click the bar */
        }
        else if (e->x < x + (int)TEXTW(selmon->ltsymbol))
            *click = ClkLtSymbol;
        else if (e->x > selmon->ww - (int)TEXTW(stext) * CFG_SHOW_WM_NAME)
            *click = ClkStatusText;
        else
        {   *click = ClkWinTitle;
        }
    }
    else if ((c = wintoclient(e->window)))
    {
        detach(c);
        attach(c);
        focus(c);
        if(c->isfloating || c->alwaysontop) XRaiseWindow(dpy, c->win);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        *click = ClkClientWin;
    }
}

void
buttonpress(XEvent *e)
{
    unsigned int i, click;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;
    const int cleanev = CLEANMASK(ev->state);

    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon)
    {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    updateclicktype(ev, &click, &arg);
    switch(click)
    {
        case ClkTagBar: break;
        case ClkLtSymbol: break;
        case ClkStatusText: break;
        case ClkWinTitle: break;
        case ClkClientWin: 
            c = wintoclient(ev->window);
            if(m->sel != c)
            {
                detach(c);
                attach(c);
                focus(c);
                if(c->isfloating || c->alwaysontop) XRaiseWindow(dpy, c->win);
                XAllowEvents(dpy, ReplayPointer, CurrentTime);
            }
            break;
    }

    for (i = 0; i < LENGTH(buttons); i++)
    {
        if (click == buttons[i].click &&
                buttons[i].type == ButtonPress && 
                buttons[i].func &&
                buttons[i].button == ev->button &&
                CLEANMASK(buttons[i].mask) == cleanev)
        {   buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
            break;
        }
    }
}

void
buttonrelease(XEvent *e)
{
    unsigned int i, click;
    Arg arg = {0};
    Monitor *m;
    XButtonReleasedEvent *ev = &e->xbutton;
    const int cleanev = CLEANMASK(ev->state);

    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon)
    {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    updateclicktype(ev, &click, &arg);
    switch(click)
    {
        case ClkTagBar: break;
        case ClkLtSymbol: break;
        case ClkStatusText: break;
        case ClkWinTitle: break;
        case ClkClientWin: break;
    }

    for (i = 0; i < LENGTH(buttons); i++)
    {
        if (click == buttons[i].click &&
                buttons[i].func &&
                buttons[i].button == ev->button &&
                CLEANMASK(buttons[i].mask) == cleanev)
        {   buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
            break;
        }
    }
}

void
circulaterequest(XEvent *e)
{
}

void
circulatenotify(XEvent *e)
{
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
}

void
colormapnotify(XEvent *e)
{
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


/* picture in picture stuff uses this
 * check the if statment if (ISVISIBLE(c)
 */
void
configurerequest(XEvent *e)
{
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    if ((c = wintoclient(ev->window)))
    {
        m = c->mon;
        if (ev->value_mask & CWBorderWidth)
        {   c->bw = ev->border_width;
        }
        else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange)
        {
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
            {   
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
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

void
createnotify(XEvent *e)
{

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

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
}

void
focusout(XEvent *e)
{
}

void
graphicsexpose(XEvent *e)
{
}

void
gravitynotify(XEvent *e)
{
}

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
                    && keys[i].func) 
            {   keys[i].func(&(keys[i].arg));
                return;
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
                    && keys[i].func) 
            {   keys[i].func(&(keys[i].arg));
                return;
            }
            break;
        }
    }
}

void
mapnotify(XEvent *e)
{

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
motionnotify(XEvent *e)
{
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if (ev->window != root)
        return;
    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) 
    {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    mon = m;
}

void
noexpose(XEvent *e)
{

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
resizerequest(XEvent *e)
{
    /* popup windows sometimes need this */
    Client *c;
    XResizeRequestEvent *ev = &e->xresizerequest;
    c = wintoclient(ev->window);
    if(c) 
    {   resize(c, c->x, c->y, ev->width, ev->height, 1);
        if(!docked(c)) 
        {   setfloating(c, 1);
        }
    }
    else  XResizeWindow(dpy, ev->window, ev->width, ev->height);
}

void selectionclear(XEvent *e)
{
}
void
selectionnotify(XEvent *e)
{
}

void
selectionrequest(XEvent *e)
{
}

void
keymapnotify(XEvent *e)
{
}

void
leavenotify(XEvent *e)
{
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
visibilitynotify(XEvent *e)
{
    return;
    /* issues with implementation as barwin causes all windwos to be visible for some reason? */
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

