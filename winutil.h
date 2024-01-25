#ifndef WINTUTIL_H_
#define WINTUTIL_H_

char *XGetWindowName(Display *display, Window win);
int XGetTextProp(Display *display, Window w, Atom atom, char *text, unsigned int size);
pid_t XGetPid(Display *display, Window win);
void XInitAtoms(Display *display);


/* typedef unsigned long Atom; */
/*
    NetSupported, NetWMName, NetWMIcon, NetWMState, NetCloseWindow, NetWMCheck,
    NetWMFullscreen, NetWMAlwaysOnTop, NetWMStayOnTop, NetActiveWindow,
    NetWMWindowType, NetWMWindowTypeDialog, NetClientList,

    NetWMActionMove, NetWMActionResize, NetWMActionMinimize, NetWMActionMaximizeHorz,
    NetWMActionMaximizeVert, NetWMActionFullscreen, NetWMActionChangeDesktop, NetWMActionClose, 
    NetWMActionAbove, NetWMActionBelow,

    NetMoveResizeWindow, NetWMMaximizedVert, NetWMMaximizedHorz, NetWMMinize,
    NetWMAbove, NetWMBelow, NetWMDemandAttention, NetWMFocused, NetWMSticky,
    NetWMModal, NetWMHidden, 
    

    NetWMWindowTypeDesktop, NetWMWindowTypeDock,
    NetWMWindowTypeToolbar, NetWMWindowTypeMenu, 
    NetWMWindowTypeUtility, NetWMWindowTypeSplash, 
    NetWMWindowTypeDropdownMenu, NetWMWindowTypePopupMenu,
    NetWMWindowTypeTooltip, NetWMWindowTypeNotification,
    NetWMWindowTypeCombo, NetWMWindowTypeDnd,

    NetWMWindowTypeNormal,

    NetWMUserTime, NetWMPing,

    NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops, NetCurrentDesktop,

    NetWMWindowsOpacity,
    NetLast,
*/
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
    NetWMFullscreen, NetWMCheck,
    NetWMAlwaysOnTop, NetWMStayOnTop,
    /* action requests */
    NetWMMaximizedVert, NetWMMaximizedHorz, NetWMMinize, NetWMActionMove,
    NetWMActionResize, NetWMActionMaximizeVert, NetWMActionMaximizeHorz,
    NetWMActionFullscreen, NetWMActionChangeDesktop, NetWMActionClose,
    NetWMActionAbove, NetWMActionMinimize, NetWMActionBelow,
    /* actions msg */
    NetWMAbove, NetWMBelow, NetWMDemandAttention, NetWMFocused, NetWMSticky,
    NetWMModal, NetWMHidden, 
    
    /* window types */
    NetWMWindowTypeDesktop, NetWMWindowTypeDock,
    NetWMWindowTypeToolbar, NetWMWindowTypeMenu, 
    NetWMWindowTypeUtility, NetWMWindowTypeSplash, 
    NetWMWindowTypeDropdownMenu, NetWMWindowTypePopupMenu,
    NetWMWindowTypeTooltip, NetWMWindowTypeNotification,
    NetWMWindowTypeCombo, NetWMWindowTypeDnd,
    NetWMWindowTypeDialog, NetWMWindowTypeNormal,

    /* wm protocols */
    WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast,
    NetWMPing, NetWMSyncRequest,
    NetWMFullscreenMonitors,
    /* other */
    NetWMFullPlacement ,NetWMBypassCompositor,
    NetWMWindowsOpacity,

    /* last */
    NetLast,
};

extern Atom netatom[NetLast];

#endif
