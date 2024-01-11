#ifndef TOGGLE_H_
#define TOGGLE_H_
void FocusMonitor(const Arg *arg);
void FocusNextWindow(const Arg *arg);
void ChangeMasterWindow(const Arg *arg);
void KillWindow(const Arg *arg);
void TerminateWindow(const Arg *arg);
void DragWindow(const Arg *arg);
void Quit(const Arg *arg);
void Restart(const Arg *arg);
void ResizeWindow(const Arg *arg);
void SetWindowLayout(const Arg *arg);
void SetMonitorFact(const Arg *arg);
void SpawnWindow(const Arg *arg);
void MaximizeWindow(const Arg *arg);
void MaximizeWindowVertical(const Arg *arg);
void MaximizeWindowHorizontal(const Arg *arg);
void PreviewTag(const Arg *arg);
void TagWindow(const Arg *arg);
void TagMonitor(const Arg *arg);
void ToggleStatusBar(const Arg *arg);
void ToggleFloating(const Arg *arg);
void ToggleFullscreen(const Arg *arg);
void ToggleTag(const Arg *arg);
void ToggleView(const Arg *arg);
void View(const Arg *arg);
void Zoom(const Arg *arg);
void AltTab(const Arg *arg);
/* debug */
void tester(const Arg *arg);

#endif
