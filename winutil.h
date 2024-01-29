#ifndef WINTUTIL_H_
#define WINTUTIL_H_

char *XGetWindowName(Display *display, Window win);
int XGetTextProp(Display *display, Window w, Atom atom, char *text, unsigned int size);
void XPingWindow(Display *display, Window win);
pid_t XGetPid(Display *display, Window win);
void XInitAtoms(Display *display);


/* typedef unsigned long Atom; */
enum 
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
    NetWMStateFullscreen, NetWMCheck,
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
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast, };

extern Atom netatom[NetLast];
extern Atom wmatom[WMLast];

#endif
