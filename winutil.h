#ifndef WINTUTIL_H_
#define WINTUTIL_H_

#include <X11/Xlib.h>
#include <stdint.h>

/* Gets the window name from specified window into a char *
 * Recommend size 256
 * Returns XStatus
 * {Success, BadLength, BadName}
 */
int XGetWindowName(Display *display, Window win, char *name, size_t sizeofname);
/* Gets specified text prop from window and specified Atom into char *text with known size */
int XGetTextProp(Display *display, Window w, Atom atom, char *text, unsigned int size);
/* Sends a ping to specified window */
void XPingWindow(Display *display, Window win);
/* Gets the Pid of a specified Window
 * Returns -1 on failure
 */
pid_t XGetPid(Display *display, Window win);
/* Initiliazes All display Atoms */
void XInitAtoms(Display *display);

typedef struct ClientHash ClientHash;

struct ClientHash
{   
    uint32_t id; /* u32 saves some bytes on x64 platforms */
    void *data;
};

/* typedef unsigned long Atom; */
enum NETWMPROTOCOLS
{
    /* Root window properties */
    NetSupported, NetClientList, 
    NetNumberOfDesktops, NetDesktopGeometry,
    NetDesktopViewport, NetCurrentDesktop,
    NetDesktopNames, NetActiveWindow,
    NetWorkarea, NetSupportingWMCheck,
    NetVirtualRoots, NetDesktopLayout,
    NetShowingDesktop,
    /* other root messages */
    NetCloseWindow, NetMoveResizeWindow, 
    NetMoveResize,/* _NET_WM_MOVERESIZE */
    NetRestackWindow,
    NetRequestFrameExtents,
    /* application win properties */
    NetWMName, NetWMVisibleName,
    NetWMIconName, NetWMVisibleIconName,
    NetWMDesktop, NetWMWindowType, 
    NetWMState, NetWMAllowedActions,
    NetWMStrut, NetWMStrutPartial,
    NetWMIconGeometry, NetWMIcon,
    NetWMPid, NetWMHandledIcons, 
    NetWMUserTime, NetWMUserTimeWindow,
    NetWMFrameExtents, NetWMOpaqueRegion,
    NetWMStateFullscreen, 
    NetWMStateAlwaysOnTop, NetWMStateStayOnTop,
    NetWMStateMaximizedVert, NetWMStateMaximizedHorz, NetWMStateMinimize, 
    NetWMStateAbove, NetWMStateBelow, NetWMStateDemandAttention,  NetWMStateSticky,
    NetWMStateShaded, NetWMStateSkipTaskbar, NetWMStateSkipPager,
    NetWMStateModal, NetWMStateHidden, NetWMStateFocused,
    /* action requests */
    NetWMActionMove,
    NetWMActionResize, NetWMActionMaximizeVert, NetWMActionMaximizeHorz,
    NetWMActionFullscreen, NetWMActionChangeDesktop, NetWMActionClose,
    NetWMActionAbove, NetWMActionMinimize, NetWMActionBelow,
    /* actions msg */
    NetWMAbove, NetWMBelow, NetWMDemandAttention, NetWMFocused, 
    NetWMFullscreen,
    
    /* window types */
    NetWMWindowTypeDesktop, NetWMWindowTypeDock,
    NetWMWindowTypeToolbar, NetWMWindowTypeMenu, 
    NetWMWindowTypeUtility, NetWMWindowTypeSplash, 
    NetWMWindowTypeDropdownMenu, NetWMWindowTypePopupMenu,
    NetWMWindowTypeTooltip, NetWMWindowTypeNotification,
    NetWMWindowTypeCombo, NetWMWindowTypeDnd,
    NetWMWindowTypeDialog, NetWMWindowTypeNormal,

    /* wm protocols */
    NetWMPing, NetWMSyncRequest,
    NetWMFullscreenMonitors,
    /* other */
    NetWMFullPlacement ,NetWMBypassCompositor,
    NetWMWindowsOpacity,

    /* last */
    NetLast,
};
/* default atoms */
enum WMPROTOCOLS { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast, };

extern Atom netatom[NetLast];
extern Atom wmatom[WMLast];

#endif
