# dwm

> _"dwm is an extremely fast, small, and dynamic window manager for X."_

**_Not for long._**

## Patches applied:

1.  **Actual fullscreen**
2.  **Alt-Tab2**
3.  **maximize**
4.  **refreshrate**
5.  **noborderflicker**
6.  **activetagindicatorbar**
7.  **activemonitor**
8.  **bartabgroups**
9.  **stacker**
10. **winview**
11. **grid**
11. **winicon**
12. **restartrig**
13. **restoreafterrestart**
14.  resize anywhere
15.  Alt-Tab minimal

## Preview:

![alttab](/cool_images/alt_tab.png "AltTab.")

## Requirements
1. pactl
2. playerctl
3. brightnessctl
4. dmenu
5. st
6. scrot   
7. x11 (xlib)
8. xorg
9. xorg-xinit
10. 8MiB <~> 15MiB + (xorg RAM) depending on your system this number may vary though legacy systems may use less RAM
You can download these via your package manager

**Pacman && yay**
```
sudo pacman -S --needed libpulse playerctl brightnessctl dmenu scrot libx11 xorg xorg-xinit && yay -S st
```

## Usage 
To **use** Sd-WM you must first **compile it**.
Afterwards you must put `exec dwm` in your `~/.xinitrc` file. **See Below.**


## Compiling
1. Clone this repository: 
https://github.com/DerjenigeUberMensch/Sd-WM.git
2. ```cd Sd-WM``` into the repository
3. ```git checkout origin/Experimental```
3. Configure it See **Configuration** (Optional)
4. `sudo make clean install` to compiled
5. Done.

## Configuration
To **configure** Sd-WM head on over to `config.def.h` and change the variables there to fit your needs.
If you feel like **patching** Sd-WM **yourself** you may do so and find **documentation** **[here](https://dwm.suckless.org/customisation/)**. 
Once you **finish** `rm config.h` if it exists and **recompile.**

## Troubleshoot
This is an **_experimental_** build and may contain bugs, 
Screen tearing can be mitigated by using a compositor or by enabling it in your driver setting
