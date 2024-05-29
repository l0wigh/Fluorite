# Part 1: Static configuration

The static configuration can be found inside `config/config_fluorite.h` and it contains all the options that can't be dynamically reloaded. Which means you need to configure this first.

Inside this file you'll find :
    - Metakey option
    - Follow windows option
        (True means that sending a window to a new workspace will make you go to that workspace)
    - Max windows option
        (Limit the number of windows per workspace. Less windows means less ram usage)
    - Auto floating option
        (False means that floating windows will be put inside the tiling layout)
    - Default monitor workspace variable
    - Workspace names variable
        (Change the workspace names for polybar for example)
    - Default floating array
        (Windows that needs to be opened in floating mode)
    - Default fixed array
        (Windows that will not be managed by Fluorite)
    - Fluorite intern functions
    - User defined functions
    - Bind array
        (Type of Metakey (0 means no need for metakey), key, function)

# Part 2: Dynamic configuration

The dynamic configuration needs to be created by user. Fluorite will use default values if nothing is set by the user. If a configuration is found, Fluorite will use it at startup

To create the configuration, go to `$HOME/.config/` (create the .config folder if it doesn't exist yet) and create a folder called `Fluorite`.

Inside this folder you can create and edit your fluorite.conf file. If you doesn't feel inspired or want to start from a default configuration, copy ones from `dynamic_config` folder inside Fluorite repo.

Options should be written in CAPS. One option per line. Respect the OPTIONS=value format, otherwise **kaboom** !

Keep in mind that no security are there to avoid errors in the configuration, be carefull with the values you want to set.

You can set only the options that you want to configure and let Fluorite use default values for the other. Just remove the lines you don't need to configure.

You can reload the config while Fluorite is running by using a keybind that will call `fluorite_reload_config`. (By default META + SHIFT + R)

Available options :
    - BORDER_WIDTH (integer value, stay at or above 0)
    - BORDER_FOCUSED (hex value, 0xRRGGBB or RRGGBB)
    - BORDER_UNFOCUSED (hex value, 0xRRGGBB or RRGGBB)
    - BORDER_INACTIVE (hex value, 0xRRGGBB or RRGGBB)
    - GAPS (integer value, stay at or above 0)
    - STACK_OFFSET (integer value, stay at or above 0)
    - TOPBAR_GAPS (integer value, stay at or above 0)
    - BOTTOMBAR_GAPS (integer value, stay at or above 0)
    - DEFAULT_MASTER_OFFSET (integer value, stay above half your monitor width)

Example :
```
BORDER_WIDTH=1
BORDER_FOCUSED=0x35e5dc
BORDER_UNFOCUSED=0xf576e4
BORDER_INACTIVE=0x9c082d
GAPS=5
STACK_OFFSET=5
TOPBAR_GAPS=25
BOTTOMBAR_GAPS=0
DEFAULT_MASTER_OFFSET=0
```


# Additionnal X11 programs

Here you can find programs that I use with Fluorite

- Launcher      : Rofi
- Compositor    : [Picom](https://github.com/XoDefender/picom) (XoDefender fork)
- Taskbar       : Polybar
- The fishes    : Asciiquarium
- The matrix    : cmatrix
- Taskmanager   : btop

Default config files for these programs are located inside `dotfiles_examples` folder
