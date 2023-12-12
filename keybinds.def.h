

/* key definitions */
#define ALT     Mod1Mask
#define NUMLOCK Mod2Mask
#define SUPER   Mod4Mask /* windows key */
#define CTRL    ControlMask
#define SHIFT   ShiftMask
#define TAB     XK_Tab
/* #define ISO_LEVEL5_SHIFT Mod3Mask
 * #define ISO_LEVEL3_SHIFT Mod5Mask */

/* TAGGING IS NOT IMPLEMENT IN THIS BUILD OF DWM and is left for user discretion */
#define TAGKEYS(KEY,TAG) \
{ KeyPress,                     SUPER,                       KEY,      view,           {.ui = 1 << TAG} }, \
{ KeyPress,                     SUPER|CTRL,                  KEY,      toggleview,     {.ui = 1 << TAG} }, \
{ KeyPress,                     SUPER|SHIFT,                 KEY,      tag,            {.ui = 1 << TAG} }, \


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
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_black, "-nf", col_white, "-sb", col_black, "-sf", col_white, topbar ? NULL : "-b" , fastinputbar ? "-f" : NULL , NULL }; /* flags -b == bottom bar; -f == getkeyboard input first then handle request; */
static const char *termcmd[]        = { "st", NULL };
static const char *screenshotcmd[]  = {"scrot","-d3", "$HOME/Pictures/Screenshots/%Y-%m-%d-%T-screenshot.png", NULL}; /*doesnt work*/

static const Key keys[] = {
    /*Action                        modifier                     key        function        argument */
    { KeyPress,                     SUPER,          XK_n,       tester,         {0} },
    { KeyPress,                     SUPER,          XK_d,       spawn,          {.v = dmenucmd } },
    { KeyPress,                     SUPER,          XK_Return,  spawn,          {.v = termcmd } },
    { KeyRelease,                   SUPER,          XK_Print,   spawn,          {.v = screenshotcmd } },
    { KeyPress,                     SUPER,          XK_b,       togglebar,      {0} },
    { KeyPress,                     SUPER,          XK_q,	    view,           {0} },
    { KeyPress,                     SUPER|SHIFT,    XK_q,       killclient,     {0} },
    { KeyPress,                     CTRL|ALT,       XK_q,	    forcekillclient,{0} },
    { KeyPress,                     SUPER,          XK_w,       togglemaximize, {0} },
    { KeyRelease,                   SUPER|SHIFT,    XK_p,       quitdwm,        {0} },
    { KeyPress,                     SUPER|CTRL,     XK_p,       restartdwm,     {0} }, 
    { KeyPress,                     SUPER,          XK_z,       setlayout,      {TILED} },
    { KeyPress,                     SUPER,          XK_x,       setlayout,      {FLOATING} },
    { KeyPress,                     SUPER,          XK_c,       setlayout,      {MONOCLE} },
    { KeyPress,                     SUPER,          XK_g,       setlayout,      {GRID} },
    { KeyPress,                     0,              XK_F11,     togglefullscr,  {0} },

    { KeyPress,                     ALT,            TAB,        alttabstart,	{0} },
    { KeyPress,                     0, XF86XK_AudioMute,                        spawn, {.v = mute_vol } },
    { KeyPress,                     0, XF86XK_AudioLowerVolume,                 spawn, {.v = down_vol } },
    { KeyPress,                     0, XF86XK_AudioRaiseVolume,                 spawn, {.v = up_vol } },
    { KeyPress,                     0, XF86XK_MonBrightnessDown,                spawn, {.v = dimmer } },
    { KeyPress,                     0, XF86XK_MonBrightnessUp,                  spawn, {.v = brighter } },
    { KeyPress,                     0, XF86XK_AudioPlay,                        spawn, {.v = pause_vol } },
    { KeyPress,                     0, XF86XK_AudioPause,                       spawn, {.v = pause_vol } },
    { KeyPress,                     0, XF86XK_AudioNext,                        spawn, {.v = next_vol } },
    { KeyPress,                     0, XF86XK_AudioPrev,                        spawn, {.v = prev_vol } },

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
static const Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
    { ClkWinTitle,          0,              Button2,        zoom,           {0} },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
    { ClkClientWin,         SUPER,          Button1,        movemouse,      {0} },
    { ClkClientWin,         SUPER,          Button2,        togglefloating, {0} },
    { ClkClientWin,         SUPER,          Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            SUPER,          Button1,        tag,            {0} },
    { ClkTagBar,            SUPER,          Button3,        toggletag,      {0} },
};

