/*
 * Skip to 'HERE' if you dont know what your doing 
 */




#ifndef KEYBINDS_DEF_H_
#define KEYBINDS_DEF_H_

#include <X11/XF86keysym.h> 
#include "toggle.h"
#include "config.h"

/* key definitions */
#define ALT     Mod1Mask
#define NUMLOCK Mod2Mask
#define SUPER   Mod4Mask /* windows key */
#define CTRL    ControlMask
#define SHIFT   ShiftMask
#define TAB     XK_Tab
#define CAPSLOCK LockMask
/* Mouse definitions */
#define LMB  Button1    /* left  mouse button */
#define MMB  Button2    /* midde mouse button */
#define RMB  Button3    /* Right mouse button */
/*
#define Button4 Button4
#define Button5 Button5
#define Button6 Button6
*/
/* #define ISO_LEVEL5_SHIFT Mod3Mask
 * #define ISO_LEVEL3_SHIFT Mod5Mask */

#define TAGKEYS(KEY,TAG) \
{ KeyPress,                     SUPER,                       KEY,      View,            {.ui = 1 << TAG} }, \
{ KeyPress,                     SUPER|CTRL,                  KEY,      ToggleView,      {.ui = 1 << TAG} }, \
{ KeyPress,                     SUPER|SHIFT,                 KEY,      TagWindow,       {.ui = 1 << TAG} }, \


/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* Multimedia Keys (laptops and function keyboards) */
static const char *up_vol[]   = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+1%",   NULL };
static const char *down_vol[] = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-1%",   NULL };
static const char *mute_vol[] = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };
static const char *pause_vol[]= { "playerctl", "play-pause"};
static const char *next_vol[] = { "playerctl", "next"};
static const char *prev_vol[] = { "playerctl", "previous"};
static const char *brighter[] = { "brightnessctl", "set", "1%+", NULL };
static const char *dimmer[]   = { "brightnessctl", "set", "1%-", NULL };
/* commands */
static char dmenumon[2] = "0"; 
static const char *dmenucmd[] = 
{ 
    "dmenu_run", "-m", dmenumon, "-fn", dmenufont, 
    "-nb", CFG_DMENU_COL_NORM_BACKGROUND, "-nf", CFG_DMENU_COL_NORM_FOREGROUND, 
    "-sb", CFG_DMENU_COL_SEL_BACKGROUND, "-sf", CFG_DMENU_COL_SEL_FOREGROUND, 
    CFG_DMENU_TOP_BAR ? NULL : "-b", CFG_DMENU_FAST_INPUT ? "-f" : NULL ,CFG_DMENU_CASE_SENSITIVE ? "-i" : NULL, NULL
}; /* flags -b == bottom bar; -f == getkeyboard input first then handle request; */
/* HERE 
 * Keybinds are as clear as possible so just go down and read them if you dont get it go to the function definitions
 * and search up the function in toggle.c
 */
static const char *termcmd[]        = { "st", NULL };
static const char *filemanager[]    = {"thunar", NULL };
static const Key keys[] = 
{
    /*Action            modifier                    key         function            argument */
    /* Windowkey+n UserStats is a debug function                */
    { KeyPress,         SUPER,                      XK_n,       UserStats,          {0} },
    /* Windowkey+d      This opens dmenu                        */
    { KeyPress,         SUPER,                      XK_d,       SpawnWindow,        {.v = dmenucmd } },
    /* Windowkey+Enter  This opens a terminal                   */
    { KeyPress,         SUPER,                      XK_Return,  SpawnWindow,        {.v = termcmd } },
    /* Windowkey+e      This Spawns the filemanager specified   */
    { KeyPress,         SUPER,                      XK_e,       SpawnWindow,        {.v = filemanager } },
    /* Windowkey+b      This toggles on and off the status bar */
    { KeyPress,         SUPER,                      XK_b,       ToggleStatusBar,    {0} },
    /* Windowkey+b      This toggles between prev and next tag */
    { KeyPress,         SUPER,                      XK_q,	    View,               {0} },
    /* Windowkey+b      This kills the current window           */
    { KeyPress,         SUPER|SHIFT,                XK_q,       KillWindow,         {0} },
    /* Windowkey+b      This kills the current window           */
    { KeyPress,         CTRL|ALT,                   XK_q,	    TerminateWindow,    {0} },
    /* Windowkey+w      This maxizes the current window         */
    { KeyPress,         SUPER,                      XK_w,       MaximizeWindow,     {0} },
    /* Windowkey+Shift+pThis Exist the current dwm session      */
    { KeyRelease,       SUPER|SHIFT,                XK_p,       Quit,               {0} },
    /* Windowkey+Ctrl+p This Restarts the current dwm session   */
    { KeyPress,         SUPER|CTRL,                 XK_p,       Restart,            {0} },  /* UNSAFE sscanf() */
    /* Windowkey+z      This sets the window layout to Tiled    */
    { KeyPress,         SUPER,                      XK_z,       SetWindowLayout,    {Tiled} },
    /* Windowkey+x      This sets the window layout to Floating */
    { KeyPress,         SUPER,                      XK_x,       SetWindowLayout,    {Floating} },
    /* Windowkey+c      This sets the window layout to Monocle  */
    { KeyPress,         SUPER,                      XK_c,       SetWindowLayout,    {Monocle} },
    /* Windowkey+g      This sets the window layout to Grid     */
    { KeyPress,         SUPER,                      XK_g,       SetWindowLayout,    {Grid} },
    /* F11              This toggles Fullscreen on a window     */
    { KeyPress,         0,                          XK_F11,     ToggleFullscreen,   {0} },
    /* Alt+Tab          This switches the first and next window */
    { KeyPress,         ALT,                        TAB,        AltTab,	            {0} },
    /* multimedia keys */

    /* AudioMute Button This mutes the volume                   */
    { KeyPress,         0, XF86XK_AudioMute,                    SpawnWindow,        {.v = mute_vol } },
    /* AudioLowerVolume this decrease the current volume        */
    { KeyPress,         0, XF86XK_AudioLowerVolume,             SpawnWindow,        {.v = down_vol } },
    /* AudioRaiseVolume this increases the current volume       */
    { KeyPress,         0, XF86XK_AudioRaiseVolume,             SpawnWindow,        {.v = up_vol } },
    /* MonBrightnessDown this decreases the current brightness  */
    { KeyPress,         0, XF86XK_MonBrightnessDown,            SpawnWindow,        {.v = dimmer } },
    /* MonBrightnessUp this increases the current brightness    */
    { KeyPress,         0, XF86XK_MonBrightnessUp,              SpawnWindow,        {.v = brighter } },
    /* AudioPlay This pauses/unpauses the current audio played  */
    { KeyPress,         0, XF86XK_AudioPlay,                    SpawnWindow,        {.v = pause_vol } },
    /* AudioPause This pauses/unpauses the current audio played */
    { KeyPress,         0, XF86XK_AudioPause,                   SpawnWindow,        {.v = pause_vol } },
    /* AudioNext This goes to the next track                    */
    { KeyPress,         0, XF86XK_AudioNext,                    SpawnWindow,        {.v = next_vol } },
    /* AudioNext This goes to the previous track                */
    { KeyPress,         0, XF86XK_AudioPrev,                    SpawnWindow,        {.v = prev_vol } },

    TAGKEYS(                        XK_1,                      0)
    TAGKEYS(                        XK_2,                      1)
    TAGKEYS(                        XK_3,                      2)
    TAGKEYS(                        XK_4,                      3)
    TAGKEYS(                        XK_5,                      4)
    TAGKEYS(                        XK_6,                      5)
    TAGKEYS(                        XK_7,                      6)
    TAGKEYS(                        XK_8,                      7)
    TAGKEYS(                        XK_9,                      8)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = 
{
    /*type                  click                event mask      button             function            argument */
    /* This sets the window layout                  */
    { ButtonPress,        ClkLtSymbol,          0,              LMB,              SetWindowLayout,    {.i = Tiled} },
    /* This sets the window layout                  */
    { ButtonPress,        ClkLtSymbol,          0,              RMB,              SetWindowLayout,    {.i = Monocle} },
    /* This moves a window based on the mouse pos   */
    { ButtonPress,        ClkClientWin,         SUPER,          LMB,              DragWindow,         {0} },
    /* This resizes a window based on the mouse pos */
    { ButtonPress,        ClkClientWin,         SUPER,          RMB,              ResizeWindow,       {0} },
    /* This toggles between the first and last tag  */
    { ButtonPress,        ClkTagBar,            0,              LMB,              View,               {0} },
    /* This moves a window to the selected tag      */
    { ButtonPress,        ClkTagBar,            SUPER,          LMB,              TagWindow,          {0} },
    /* This doesnt work                             */
    { ButtonPress,        ClkTagBar,            SUPER,          RMB,              ToggleTag,          {0} },
};


#endif
