# Fluorite 1.0

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
- Static configuration for bindings, options, and design, compiled with Fluorite.
- Organizer mode, let you swap windows freely.
- Works with multiple monitors. XMonad style.

## Why RC (Release Candidate) ?

This version is pretty stable but needs more testing. It's on the master branch because the current code should be really close to be the final release.

## Documentation ?

Go to [Fluorite Website](https://fluorite.surge.sh) for more informations. It's a bit outdated so don't follow it blindly.

You can also find some quick tips inside [CONFIG.md](./CONFIG.md).

# Installation (basic informations)

## Deps

On Archlinux you can type this command to install everything you need.

``` sh
sudo pacman -S xorg xdotool libxft libxcomposite libxcursor libxrandr
```

## Build and install

After doing modifications to the config, just type (WITHOUT sudo). It will remake and copy the Fluorite executable to `/usr/bin/`.

``` sh
make install
```

## .xinitrc example

``` sh
setxkbmap -layout fr
polybar &
picom &
exec Fluorite
```

Keep in mind that the setxkbmap with the proper keyboard layout is *REQUIRED* so you can have all your bindings working.

## Known issues

- If java apps are clunky, you can add `export _JAVA_AWT_WM_NONREPARENTING=1` to you .xinitrc just before Fluorite execution.
