#ifndef EVENTS_H_
#define EVENTS_H_

#include <X11/Xlib.h>

extern void keypress(XEvent *event);
extern void keyrelease(XEvent *event);
extern void buttonpress(XEvent *event);
extern void buttonrelease(XEvent *event);
extern void motionnotify(XEvent *event);
extern void enternotify(XEvent *event);
extern void leavenotify(XEvent *event);
extern void focusin(XEvent *event);
extern void focusout(XEvent *event);
extern void keymapnotify(XEvent *event);
extern void keymapstate(XEvent *event);
extern void expose(XEvent *event);
extern void graphicsexpose(XEvent *event);
extern void noexpose(XEvent *event);
extern void structurenotify(XEvent *event);
extern void substructurenotify(XEvent *event);
extern void substructureredirect(XEvent *event);
extern void circulaterequest(XEvent *event);
extern void configurerequest(XEvent *event);
extern void maprequest(XEvent *event);
extern void resizerequest(XEvent *event);
extern void circulatenotify(XEvent *event);
extern void configurenotify(XEvent *event);
extern void createnotify(XEvent *event);
extern void destroynotify(XEvent *event);
extern void gravitynotify(XEvent *event);
extern void mapnotify(XEvent *event);
extern void mappingnotify(XEvent *event);
extern void unmapnotify(XEvent *event);
extern void visibilitynotify(XEvent *event);
extern void reparentnotify(XEvent *event);
extern void resizeredirect(XEvent *event);
extern void propertychange(XEvent *event);
extern void focuschange(XEvent *event);
extern void colormapnotify(XEvent *event);
extern void clientmessage(XEvent *event);
extern void propertynotify(XEvent *event);
extern void selectionclear(XEvent *event);
extern void selectionnotify(XEvent *event);
extern void selectionrequest(XEvent *event);

extern void (*handler[LASTEvent]) (XEvent *);

#endif
