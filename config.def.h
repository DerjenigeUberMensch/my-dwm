/* See LICENSE file for copyright and license details. */

/* appearance */
static char WM_NAME[]               = "dwm.exe";//wm name displayed when using X (type neofetch to see this)
static const unsigned int borderpx  = 0;        /* border pixel of windows */
static const unsigned int snap      = 15;        /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 0;        /* 0 means bottom bar */
static const int barpadding         = 0;        /* pixel (status bar)/2 padding */
static const char *fonts[]          = {"monospace:size=15" };
static const char dmenufont[]       = "monospace:size=15";
static char col_black[]       = "#000000";
static char col_white[]       = "#ffffff";
/*
static char col_red[]         = "#ff0000";
static char col_green[]       = "#00b300";
static char col_term_blue[]   = "#ecffff";
static char col_yellow[]      = "#ffff00";
*/
static char *colors[][3]      = {
/*					fg         bg          border   */
/*                  fg=textColour                   */
	[SchemeNorm]    = { col_white, col_black,  col_white},
	[SchemeSel]     = { col_white, col_black,   col_white},
};
/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
    */
    /* class      instance    title       tags mask     isfloating   monitor */
    { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95]      */
static const int nmaster     = 1;    /* number of clients in master area             */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window  */

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },    /* first entry is default */
    { "><>",      NULL },    /* no layout function means floating behavior */
    { "[M]",      monocle },
};

/* key definitions */
//ALT Mod1Mask   //TO save memory refer to here
//Window Mod4Mask
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }


/* alt-tab configuration */
static const unsigned int tabModKey 		= 0x40;	/* if this key is hold the alt-tab functionality stays acitve. This key must be the same as key that is used to active functin altTabStart `*/
static const unsigned int tabCycleKey 		= 0x17;	/* if this key is hit the alt-tab program moves one position forward in clients stack. This key must be the same as key that is used to active functin altTabStart */
static const unsigned int tabPosY 			= 1;	/* tab position on Y axis, 0 = bottom, 1 = center, 2 = top */
static const unsigned int tabPosX 			= 1;	/* tab position on X axis, 0 = left, 1 = center, 2 = right */
static const unsigned int maxWTab 			= 600;	/* tab menu width */
static const unsigned int maxHTab 			= 200;	/* tab menu height */



/* Multimedia Keys (laptops and function keyboards) */
//volume
static const char *up_vol[]   = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+1%",   NULL };
static const char *down_vol[] = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-1%",   NULL };
static const char *mute_vol[] = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };
static const char *pause_vol[]= { "playerctl", "play-pause"};
static const char *next_vol[] = { "playerctl", "next"};
static const char *prev_vol[] = { "playerctl", "previous"};
//brightness 
static const char *brighter[] = { "brightnessctl", "set", "1%+", NULL };
static const char *dimmer[]   = { "brightnessctl", "set", "1%-", NULL };

//refer here => https://ratfactor.com/dwm
/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */ 
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_black, "-nf", col_white, "-sb", col_black, "-sf", col_white, topbar ? NULL : "-b", "-f", NULL }; /* flags -b == bottom bar; -f == getkeyboard input first then handle request; */
static const char *termcmd[]        = { "st", NULL }; 
static const char *screenshotcmd[]  = {"scrot", "~/Pictures/Screenshots/%Y-%m-%d-%T-screenshot.jpg", NULL};

static const Key keys[] = {
    /* modifier                     key        function        argument */
    { MODKEY,                       XK_d,      spawn,          {.v = dmenucmd } },
    { MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
    { MODKEY,                       XK_b,      togglebar,      {0} },
    { MODKEY,                       XK_q,	   view,           {0} },
    { MODKEY|ShiftMask,             XK_q,      killclient,     {0} }, 
    { MODKEY,                       XK_w,      togglemaximize, {0} }, 
    { ControlMask|Mod1Mask,         XK_q,	   forcekillclient,{0} },
    { MODKEY|ShiftMask,             XK_p,      quit,           {0} },
    { MODKEY,                       XK_z,      setlayout,      {.v = &layouts[0]} },//TILED
    { MODKEY,                       XK_x,      setlayout,      {.v = &layouts[1]} },//MONOCLE 
    { MODKEY,                       XK_c,      setlayout,      {.v = &layouts[2]} },//FLOATING
    //ALT TAB
    { Mod1Mask,             		XK_Tab,    altTabStart,	   {0} },

    //Multi media
    { 0, XF86XK_AudioMute,                     spawn, {.v = mute_vol } },
    { 0, XF86XK_AudioLowerVolume,              spawn, {.v = down_vol } },
    { 0, XF86XK_AudioRaiseVolume,              spawn, {.v = up_vol } },
    { 0, XF86XK_MonBrightnessDown,             spawn, {.v = dimmer } },
    { 0, XF86XK_MonBrightnessUp,               spawn, {.v = brighter } },
    { 0, XF86XK_AudioPlay,                     spawn, {.v = pause_vol } },
    { 0, XF86XK_AudioPause,                    spawn, {.v = pause_vol } },
    { 0, XF86XK_AudioNext,                     spawn, {.v = next_vol } },
    { 0, XF86XK_AudioPrev,                     spawn, {.v = prev_vol } },
    //Screenshot
    { MODKEY,                       XK_Print,  spawn,          {.v = screenshotcmd } },
    //Fulscreen function
    { 0,             XK_F11,      togglefullscr,  {0} },
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
    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
    { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
    { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

