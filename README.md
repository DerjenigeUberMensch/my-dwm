# dwm

> _"dwm is an extremely fast, small, and dynamic window manager for X."_

**_Not for long._**

## Patches applied:

1. **Actual fullscreen**
2. **Alt-Tab**
3. Alt-Tab minimal
4. **maximize**
5. **refreshrate**
6. **resizehere**
7. **resizecorners**
8. Easy config

## Preview:

![alttab](/cool_images/alt_tab.png "AltTab.")

## Usage 
To **use** Sd-WM you must first **compile it**.
Afterwards you must put `exec dwm` in your `~/.xinitrc` file. **See Below.**


## Compiling
1. Clone this repository: 
https://github.com/DerjenigeUberMensch/Sd-WM.git
2. `cd Sd-WM` into the repository
3. Configure it See **Configuration** (Optional)
4. `sudo make clean install` to compiled
5. Ignore any warnings (compiled with O0)
5. Enjoy!

*-O1+ compilation not fully supported.*

## Configuration
To **configure** Sd-WM head on over to `config.def.h` and change the variables there to fit your needs.
If you feel like **patching** Sd-WM **yourself** you may do so and find **documentation** **[here](https://dwm.suckless.org/customisation/)**. 
Once you **finish** `rm config.h` if it exists and **recompile.**

## Troubleshoot
You may need libraries such as x11 or "xlib" `sudo pacman -S libx11`

