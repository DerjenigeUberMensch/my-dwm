#ifndef TOGGLE_H_
#define TOGGLE_H_
#include "dwm.h"

extern void UserStats(const Arg *arg);
extern void FocusMonitor(const Arg *arg);
extern void FocusNextWindow(const Arg *arg);
extern void ChangeMasterWindow(const Arg *arg);
extern void KillWindow(const Arg *arg);
extern void TerminateWindow(const Arg *arg);
extern void DragWindow(const Arg *arg);
extern void Quit(const Arg *arg);
extern void Restart(const Arg *arg);
extern void ResizeWindow(const Arg *arg);
extern void SetWindowLayout(const Arg *arg);
extern void SetMonitorFact(const Arg *arg);
extern void SpawnWindow(const Arg *arg);
extern void MaximizeWindow(const Arg *arg);
extern void MaximizeWindowVertical(const Arg *arg);
extern void MaximizeWindowHorizontal(const Arg *arg);
extern void PreviewTag(const Arg *arg);
extern void TagWindow(const Arg *arg);
extern void TagMonitor(const Arg *arg);
extern void ToggleStatusBar(const Arg *arg);
extern void ToggleFloating(const Arg *arg);
extern void ToggleFullscreen(const Arg *arg);
extern void ToggleTag(const Arg *arg);
extern void ToggleView(const Arg *arg);
extern void View(const Arg *arg);
extern void Zoom(const Arg *arg);
extern void AltTab(const Arg *arg);
/* debug */
extern void tester(const Arg *arg);

#endif
