/* MIT/X Consortium License 
 * See LICENSE for non specific functions (everything else)
 *
 * XGetWindowName()
 *      2006-2014 Anselm R Garbe <anselm@garbe.us>
 *      2011 Connor Lane Smith <cls@lubutu.com>
 *
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xft/Xft.h>

#include "winutil.h"

int
XGetTextProp(Display *display, Window w, Atom atom, char *text, unsigned int size)
{
    char **list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0)
        return 0;
    text[0] = '\0';
    if (!XGetTextProperty(display, w, &name, atom) || !name.nitems)
        return 0;
    if (name.encoding == XA_STRING)
        strncpy(text, (char *)name.value, size - 1);
    else if (XmbTextPropertyToTextList(display, &name, &list, &n) >= Success && n > 0 && *list)
    {
        strncpy(text, *list, size - 1);
        XFreeStringList(list);
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

char *
XGetWindowName(Display *display, Window win)
{
    #define CHAR_BUFF (256)
    static char buf[CHAR_BUFF];
    char **list;
    int n;
    XTextProperty prop;
     
    if (!XGetTextProperty(display, win, &prop, XInternAtom(display, "_NET_WM_NAME", False)) || !prop.nitems) 
    {
        if (!XGetWMName(display, win, &prop) || !prop.nitems) return "";
    }
    if (!XmbTextPropertyToTextList(display, &prop, &list, &n) && n > 0) 
    {
        strncpy(buf, list[0], sizeof(buf));
        XFreeStringList(list);
    }
    else 
    {   strncpy(buf, (char *)prop.value, sizeof(buf));
    }

    XFree(prop.value);
    buf[sizeof(buf) - 1] = '\0';

    return buf;
}

pid_t
XGetPid(Display *display, Window win)
{
    Atom actualType;
    int format;
    pid_t pid = -1;
    unsigned long nitems, bytesAfter;
    unsigned char *propData = NULL;

    Atom atom = XInternAtom(display, "_NET_WM_PID", False);
    if (XGetWindowProperty(display, win, atom, 0, 1, False, XA_CARDINAL,
                           &actualType, &format, &nitems, &bytesAfter, &propData) != Success) return -1;
    if (propData) pid = *((pid_t*)propData);
    XFree(propData);
    return pid;
}

void
XInitAtoms(Display *display)
{


   /* wm */
    netatom[WMProtocols] = XInternAtom(display, "WM_PROTOCOLS", False);
    netatom[WMDelete] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    netatom[WMState] = XInternAtom(display, "WM_STATE", False);
    netatom[WMTakeFocus] = XInternAtom(display, "WM_TAKE_FOCUS", False);
    /* ewmh */
    netatom[NetActiveWindow] = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    netatom[NetWMName] = XInternAtom(display, "_NET_WM_NAME", False);
    netatom[NetWMIcon] = XInternAtom(display, "_NET_WM_ICON", False);
    netatom[NetCloseWindow] = XInternAtom(display, "_NET_CLOSE_WINDOW", False);
    netatom[NetWMCheck] = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMWindowType] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList] = XInternAtom(display, "_NET_CLIENT_LIST", False);
    netatom[NetDesktopViewport] = XInternAtom(display, "_NET_DESKTOP_VIEWPORT", False);
    netatom[NetNumberOfDesktops] = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    netatom[NetCurrentDesktop] = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    netatom[NetDesktopNames] = XInternAtom(display, "_NET_DESKTOP_NAMES", False);
    netatom[NetWMWindowsOpacity] = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);

    netatom[NetMoveResizeWindow] = XInternAtom(display, "_NET_MOVERESIZE_WINDOW", False);

    /* window types */
    netatom[NetWMWindowTypeDesktop] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    netatom[NetWMWindowTypeDock] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    netatom[NetWMWindowTypeToolbar] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    netatom[NetWMWindowTypeMenu] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", False);
    netatom[NetWMWindowTypeUtility] = XInternAtom(display, "_NET_WMWINDOW_TYPE_UTILITY", False);
    netatom[NetWMWindowTypeSplash] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetWMWindowTypeNormal] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    /* wm state */
    netatom[NetWMState] = XInternAtom(display, "_NET_WM_STATE", False);
    netatom[NetWMStayOnTop] = XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", False); /* either I have dementia or does this not exists? */
    netatom[NetWMFullscreen] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMAlwaysOnTop] = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    netatom[NetWMMaximizedVert] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    netatom[NetWMMaximizedHorz] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    netatom[NetWMAbove] = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    netatom[NetWMBelow] = XInternAtom(display, "_NET_WM_STATE_BELOW", False);
    netatom[NetWMDemandAttention] = XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    netatom[NetWMMinize] = XInternAtom(display, "_NET_WM_MINIMIZE", False);
    netatom[NetWMSticky] = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
    netatom[NetWMHidden] = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
    netatom[NetWMModal] = XInternAtom(display, "_NET_WM_STATE_MODAL", False);
    /* tracking */
    netatom[NetWMUserTime] = XInternAtom(display, "_NET_WM_USER_TIME", False);
    netatom[NetWMPing] = XInternAtom(display, "_NET_WM_PING", False);
    /* actions suppoorted */
    netatom[NetWMActionMove] = XInternAtom(display, "_NET_WM_ACTION_MOVE", False);
    netatom[NetWMActionResize] = XInternAtom(display, "_NET_WM_ACTION_RESIZE", False);
    netatom[NetWMActionMinimize] = XInternAtom(display, "_NET_WM_ACTION_MINIMIZE", False);
    netatom[NetWMActionMaximizeHorz] = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
    netatom[NetWMActionMaximizeVert] = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
    netatom[NetWMActionFullscreen] = XInternAtom(display, "_NET_WM_ACTION_FULLSCREEN", False);
    netatom[NetWMActionChangeDesktop] = XInternAtom(display, "_NET_WM_ACTION_CHANGE_DESKTOP", False);
    netatom[NetWMActionClose] = XInternAtom(display, "_NET_WM_ACTION_CLOSE", False);
    netatom[NetWMActionAbove] = XInternAtom(display, "_NET_WM_ACTION_ABOVE", False);
    netatom[NetWMActionBelow] = XInternAtom(display, "_NET_WM_ACTION_BELOW", False);

    netatom[NetSupported] = XInternAtom(display, "_NET_SUPPORTED", False);
    /* wm state */
    netatom[NetWMFullscreen] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMMaximizedVert] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    netatom[NetWMMaximizedHorz] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    netatom[NetWMAbove] = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    netatom[NetWMBelow] = XInternAtom(display, "_NET_WM_STATE_BELOW", False);
    netatom[NetWMDemandAttention] = XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    netatom[NetWMMinize] = XInternAtom(display, "_NET_WM_MINIMIZE", False);
    netatom[NetWMSticky] = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
    netatom[NetWMHidden] = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
    netatom[NetWMModal] = XInternAtom(display, "_NET_WM_STATE_MODAL", False);

    /* Root window properties */
    netatom[NetSupported] = XInternAtom(display, "_NET_SUPPORTED", False);
    netatom[NetClientList] = XInternAtom(display, "_NET_CLIENT_LIST", False);
    netatom[NetNumberOfDesktops] = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    netatom[NetDesktopGeometry] = XInternAtom(display, "_NET_DESKTOP_GEOMETRY", False);
    netatom[NetDesktopViewport] = XInternAtom(display, "_NET_DESKTOP_VIEWPORT", False);
    netatom[NetCurrentDesktop] = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    netatom[NetDesktopNames] = XInternAtom(display, "_NET_DESKTOP_NAMES", False);
    netatom[NetActiveWindow] = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    netatom[NetWorkarea] = XInternAtom(display, "_NET_WORKAREA", False);
    netatom[NetSupportingWMCheck] = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetVirtualRoots] = XInternAtom(display, "_NET_VIRTUAL_ROOTS", False);
    netatom[NetDesktopLayout] = XInternAtom(display, "_NET_DESKTOP_LAYOUT", False);
    netatom[NetShowingDesktop] = XInternAtom(display, "_NET_SHOWING_DESKTOP", False);
    /* other root messages */
    netatom[NetCloseWindow] = XInternAtom(display, "_NET_CLOSE_WINDOW", False);
    netatom[NetMoveResizeWindow] = XInternAtom(display, "_NET_MOVERESIZE_WINDOW", False);
    netatom[NetMoveResize] = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
    netatom[NetRestackWindow] = XInternAtom(display, "_NET_RESTACK_WINDOW", False);
    netatom[NetRequestFrameExtents] = XInternAtom(display, "_NET_REQUEST_FRAME_EXTENTS", False);
    /* application win properties */
    netatom[NetWMName] = XInternAtom(display, "_NET_WM_NAME", False);
    netatom[NetWMVisibleName] = XInternAtom(display, "_NET_WM_VISIBLE_NAME", False);
    netatom[NetWMIconName] = XInternAtom(display, "_NET_WM_ICON_NAME", False);
    netatom[NetWMVisibleIconName] = XInternAtom(display, "_NET_WM_VISIBLE_ICON_NAME", False);
    netatom[NetWMDesktop] = XInternAtom(display, "_NET_WM_DESKTOP", False);
    netatom[NetWMState] = XInternAtom(display, "_NET_WM_STATE", False);
    netatom[NetWMAllowedActions] = XInternAtom(display, "_NET_WM_ALLOWED_ACTIONS", False);
    netatom[NetWMStrut] = XInternAtom(display, "_NET_WM_STRUT", False);
    netatom[NetWMStrutPartial] = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
    netatom[NetWMIconGeometry] = XInternAtom(display, "_NET_WM_ICON_GEOMETRY", False);
    netatom[NetWMIcon] = XInternAtom(display, "_NET_WM_ICON", False);
    netatom[NetWMPid] = XInternAtom(display, "_NET_WM_PID", False);
    netatom[NetWMHandledIcons] = XInternAtom(display, "_NET_WM_HANDLED_ICONS", False);
    netatom[NetWMUserTime] = XInternAtom(display, "_NET_WM_USER_TIME", False);
    netatom[NetWMUserTimeWindow] = XInternAtom(display, "_NET_WM_USER_TIME_WINDOW", False);
    netatom[NetWMFrameExtents] = XInternAtom(display, "_NET_FRAME_EXTENTS", False);
    netatom[NetWMOpaqueRegion] = XInternAtom(display, "_NET_WM_OPAQUE_REGION", False);
    netatom[NetWMBypassCompositor] = XInternAtom(display, "_NET_WM_BYPASS_COMPOSITOR", False);

    /* net actions */
    netatom[NetWMActionMove] = XInternAtom(display, "_NET_WM_ACTION_MOVE", False);
    netatom[NetWMActionResize] = XInternAtom(display, "_NET_WM_ACTION_RESIZE", False);
    netatom[NetWMActionMinimize] = XInternAtom(display, "_NET_WM_ACTION_MINIMIZE", False);
    netatom[NetWMActionMaximizeHorz] = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
    netatom[NetWMActionMaximizeVert] = XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
    netatom[NetWMActionFullscreen] = XInternAtom(display, "_NET_WM_ACTION_FULLSCREEN", False);
    netatom[NetWMActionChangeDesktop] = XInternAtom(display, "_NET_WM_ACTION_CHANGE_DESKTOP", False);
    netatom[NetWMActionClose] = XInternAtom(display, "_NET_WM_ACTION_CLOSE", False);
    netatom[NetWMActionAbove] = XInternAtom(display, "_NET_WM_ACTION_ABOVE", False);
    netatom[NetWMActionBelow] = XInternAtom(display, "_NET_WM_ACTION_BELOW", False);

    /* window types */
    netatom[NetWMWindowType] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDesktop] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    netatom[NetWMWindowTypeDock] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    netatom[NetWMWindowTypeToolbar] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    netatom[NetWMWindowTypeMenu] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", False);
    netatom[NetWMWindowTypeUtility] = XInternAtom(display, "_NET_WMWINDOW_TYPE_UTILITY", False);
    netatom[NetWMWindowTypeSplash] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetWMWindowTypeNormal] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    /* Window manager protocols */
    netatom[NetWMPing] = XInternAtom(display, "_NET_WM_PING", False);
    netatom[NetWMSyncRequest] = XInternAtom(display, "_NET_WM_SYNC_REQUEST", False);
    netatom[NetWMFullscreenMonitors] = XInternAtom(display, "_NET_WM_FULLSCREEN_MONITORS", False);

    /* stuff */
    netatom[NetWMCheck] = XInternAtom(display, "_NET_WM_CHECK", False);
    netatom[NetWMFullscreen] = XInternAtom(display, "_NET_WM_FULLSCREEN", False);
    netatom[NetWMAlwaysOnTop] = XInternAtom(display, "_NET_WM_ABOVE", False);

    /* other */
    netatom[NetWMFullPlacement] = XInternAtom(display, "_NET_WM_FULL_PLACEMENT", False);
    netatom[NetWMWindowsOpacity] = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
}
