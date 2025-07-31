# Fluorite EVO 2 (Beta 2)

This is a very early Evolution of Fluorite 1.1

There is still many many things to add (ex: Monitors). Use 1.1 !

## Things that changed in EVO 2

- No more reparenting (just like DWM or SXWM)
- Fixed long time bug with Firefox (unwanted move/resize of some windows)
- More layout possibilities (DWM one is implemented along side the default ones)
- Seamlessly go from Tiled to Floating window with a simple drag and drop
- No more hacky stuff to handle Fullscreen windows
- ...

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
