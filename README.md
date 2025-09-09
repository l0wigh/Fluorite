# Fluorite [EVO 2] (Beta 8)

This is a very early Evolution of Fluorite 1.1

It's close to a Relase Candidate but still need some battle testing to see if it fits the fluorite mentality

There is still "workflow" bugs that might bother me after a while. I'm thinking of unpredictable behaviors that break zone state.


## Things that changed in [EVO 2]

- No more reparenting (just like DWM or SXWM)
- Fixed long time bug with Firefox (unwanted move/resize of some windows)
- More layout possibilities (DWM, and Centered are implemented along side the Cascade one from Fluorite [EVO 1])
- Seamlessly go from Tiled to Floating window with a simple drag and drop
- No more hacky stuff to handle Fullscreen windows
- Dynamic Scratchpads
- Better EWMH handling, leading to a better integration with other X11 tools like Polybar
- Cursor Warp options
- Window swallowing
- Custom IPC for Polybar
- ...

## Things that didn't changed in [EVO 2]

- It's still light, functionnal, predictable and beautiful
- Also the whole codebase has changed and is more robust and less clunky
- Might also be even more performant with more features

## Polybar IPC

- Layout
```
[module/fluorite_layout]
type = custom/ipc
hook-0 = "echo Cascade"
hook-1 = "echo DWM"
hook-2 = "echo Centered"
hook-3 = "echo Stacked"
```

- Scratchpads list
```
[module/fluorite_scratchpads]
type = custom/ipc
hook-0 = xprop -root FLUORITE_SCRATCHPADS 2>/dev/null | awk -F '"' '/=/{print $2}' || echo ""
```

## Old README

Fluorite is a dynamic tiling window manager that aims to be light, functionnal, predictable, and beautiful.

![Fluorite Presentation](./screenshots/rose-pine.png)
*Rosé-pine Theme*

![Fluorite Presentation](./screenshots/Multimonitor.png)
*Multimonitor Demo*

## Features

- Dynamic master layout tiling.
- Keyboard centric.
- Stacked layout (Monocle layout in DWM).
- Light floating windows management.
- Organizer mode, let you swap windows freely.
- Works with multiple monitors. XMonad style.
- Static configuration for bindings, options, and design, compiled with Fluorite.
- Dynamic reconfiguration (overwriting the compiled one) using Xresources.

## Documentation ?

Go to [Fluorite Website](https://fluorite.surge.sh) for more informations. It should be mostly updated.

You can also find some quick tips inside [CONFIG.md](./CONFIG.md).

# Installation (basic informations)

## Deps

On Archlinux you can type this command to install everything you need.

``` sh
sudo pacman -S xorg xdotool libxft libxcomposite libxcursor libxrandr
```

## Build and install

After doing modifications to the config, just type this (WITHOUT sudo). It will remake and copy the Fluorite executable to `/usr/bin/`.

``` sh
make install
```

## .xinitrc example

``` sh
[[ -f ~/.Xresources ]] && xrdb -merge -I$HOME ~/.Xresources # For autoloading your Xresources file
setxkbmap -layout fr
polybar &
picom &
exec Fluorite
```

Keep in mind that the setxkbmap with the proper keyboard layout is *REQUIRED* so you can have all your bindings working.

## Known issues

- If java apps are clunky, you can add `export _JAVA_AWT_WM_NONREPARENTING=1` to your .xinitrc just before Fluorite execution.
