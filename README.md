# Fluorite 0.4

Fluorite is a really simple window manager that aims to be light and functionnal.

## Information ?

Go to [Fluorite Website](https://fluorite.surge.sh) for more informations. I talk about configuration, installation, ect...

## Installation (basic informations)

### Deps

On Archlinux you can type to install everything you need

``` sh
sudo pacman -S xorg
```

### Build and install

After doing modifications to the config, just type (WITHOUT sudo). It will remake and copy the Fluorite executable to /usr/bin/

``` sh
make install
```

### .xinitrc example

``` sh
polybar &
picom &
setxkbmap fr &
exec Fluorite
```

Keep in mind that the setxkbmap with the proper keyboard layout is *REQUIRED* so you can have all your bindings working.

### Known issues

    - Closing multiple windows in the same time on the current workspace make Fluorite crash (ex: killall st)
