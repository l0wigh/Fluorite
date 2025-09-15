# Fluorite [EVO 2] (Beta 12)

Fluorite is a dynamic tiling window manager that aims to be light, functionnal, predictable, and beautiful.

![Fluorite Presentation](./screenshots/rose-pine.png)
*Rosé-pine Theme*

![Fluorite Presentation](./screenshots/Multimonitor.png)
*Multimonitor Demo*

## Features

- Keyboard centric
- Multi-Monitors with hot-reload capabilities (XMonad style)
- Multiple dynamic master layout tiling (Cascade, DWM, Centered, Stacked)
- Static configuration for bindings and options. Less computer resources needed for stuff you almost never change
- Dynamic (re)configuration for your theme using Xresources. "Wow is that Hyprland ?"
- Dynamic scratchpads that you bind to any of your keyboard keys. No need for multiple bindings in your configuration
- Window swallowing. Get that useless terminal out of your way
- EWMH handling. Give you more features without more configuration required
- Polybar IPC. Active Layout and Scratchpads should not be some kind of secrets

## Dependencies installation

- Arch: 
```sh
sudo pacman -S base-devel libx11 libxcursor libxrandr xdotool
```

- Void: 
```sh
sudo xbps-install -Sy base-devel libX11-devel libXcursor-devel libXrandr-devel xdotool-devel
```

- Gentoo (Untested): 
```sh
sudo emerge --ask x11-libs/libX11 x11-libs/libXcursor x11-libs/libXrandr x11-misc/xdotool sys-libs/glibc x11-libs/libxcb x11-libs/libXrender x11-libs/libXfixes x11-libs/libXext x11-libs/libXtst x11-libs/libXinerama x11-libs/libxkbcommon x11-libs/libXau x11-libs/libXdmcp
```

If you are on another distro and want to help other users to know what to install, feel free to create a pull request or an issue !

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

## Polybar IPC

Fluorite can handle very basic Polybar IPC Modules. Add them in your polybar and set the Fluorite option `POLYBAR_IPC` to `True`.

- Layout
```
[module/fluorite_layout]
type = custom/ipc
hook-0 = "echo Cascade"
hook-1 = "echo DWM"
hook-2 = "echo Centered"
hook-3 = "echo Stacked"
initial = 1 ; Change it to your default layout (1 = Cascade, 2 = DWM, ...)
```

- Scratchpads list
```
[module/fluorite_scratchpads]
type = custom/ipc
hook-0 = xprop -root FLUORITE_SCRATCHPADS 2>/dev/null | awk -F '"' '/=/{print $2}' || echo ""
initial = 1 ; Will check on startup if there is already some Scratchpads
```

## Documentation ?

Go to [Fluorite Website](https://fluorite.surge.sh) for more informations.

You can also find some quick tips inside [CONFIG.md](./CONFIG.md).

## Known issues

- If java apps are clunky, you can add `export _JAVA_AWT_WM_NONREPARENTING=1` to your .xinitrc just before Fluorite execution.
