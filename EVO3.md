# EVO3 Informations

Since the EVO2 was (close to ?) features complete, I wanted to focus on EWMH/ICCCM. Doing this will helps with strange behaviours from many apps.

- Discord notifications not working when the app was definitely not on screen.
- Ghidra (and probably other java apps) not showing popups on the right position.
- Dock apps not working properly in Multi-monitors setups
- Fluorite not handling Multi-monitors, struts, ... Properly causing issues with hot(un)plugging and specific setups.

Also some apps like pagers or docks, might need more informations from Fluorite to give full potential.

It was also time for me to format the code using normal standards. Right now it's using .clang-format file to handle this. The formating is subject to change.

I will finally try to clean the code to make it easier to work with. I might create multiple files, and stuff like that. It's not done for now since I'm not sure how I want it to be.

New features might sneak in, but it's not the point of the EVO 3 anyways.

## TODO

- [ ] Discord not giving controls when reappearing on screen, a click is required
- [ ] Missing EWMH
- [ ] Missing Client message handling
- [ ] Create multiple files to have a cleaner project
- [ ] Find and remove deadcode
- [ ] (?) Rename functions that doesn't make sense

## DONE

- [x] Ghidra window placements
- [x] Multimonitors
    - [x] Quickshell place it's windows properly
    - [x] No strut issues after plugging/unplugging
    - [x] No strange process are required anymore to make the hot plug works
- [x] Discord will give everything notifications if it's not on an active workspace
- [x] Fullscreen is now set in EWMH
- [x] Strange behavior from Firefox when setting the window fullscreen with binding
    - It leave the fullscreen of Youtube videos for exemple
- [x] Binary ninja issues came from the fact that the window was both floating and fixed, which make it fixed. The issue is that theses windows are by default behind tiled windows. So I fixed it by saying that if it's a transient window it's not a fixed which might create issues
- [x] Fixed a long time issues with floating windows appearing behind other windows. They will be now raised normally

## Issues

Binary ninja opening a removed file create a window that is showed behind it, I think I can analyse it's xprop to fix this issues on multiple programs.

_NET_WM_ALLOWED_ACTIONS(ATOM) = _NET_WM_ACTION_CLOSE
_NET_WM_USER_TIME(CARDINAL) = 54864784
_NET_WM_STATE(ATOM) = _NET_WM_STATE_MODAL
WM_TRANSIENT_FOR(WINDOW): window id # 0x2200011
_NET_WM_ICON(CARDINAL) = Icon (16 x 16):
░░▒▒▓▓▓▓▒▒░░  
░░▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓░░  
░░██▒▒░░░░▒▒▒▒░░░░▒▒██░░  
░░██▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒██░░  
▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓  
░░▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓░░
▒▒▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▓▓▒▒
▓▓▓▓▒▒██▒▒░░░░░░░░░░░░▒▒██▒▒▓▓▓▓
▓▓▓▓▒▒██ ▒▒░░ ██▒▒▓▓▓▓
▒▒▓▓▒▒██▒▒ ░░██▓▓░░ ▒▒██▒▒▓▓▒▒
░░▓▓▒▒▓▓██▓▓██▓▓▓▓██▓▓██▓▓▒▒▓▓░░
▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓  
░░██▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓██░░  
░░██▓▓▒▒▒▒▒▒▒▒▒▒▒▒▓▓██░░  
░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░  
░░▒▒▓▓▓▓▒▒░░

    Icon (24 x 24):
                      ░░▒▒▒▒▒▒▒▒░░
                ░░▒▒▓▓████████████▓▓▒▒░░
              ▒▒▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▒▒
            ▓▓▓▓▓▓▒▒░░░░░░░░░░░░░░░░▒▒▓▓▓▓▓▓
          ▓▓▓▓▓▓░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▓▓▓▓▓▓
        ▒▒██▓▓░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▓▓██▒▒
      ░░▓▓▓▓░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▓▓▓▓░░
      ▒▒██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██▒▒
      ▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓
    ░░██▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▓▓██░░
    ▒▒██▒▒▒▒▓▓██▓▓████████████████████▓▓██▓▓▒▒▒▒██▒▒
    ▒▒██▒▒▒▒▓▓██░░                    ░░██▓▓▒▒▒▒██▒▒
    ▒▒██▒▒▒▒▓▓██░░        ▒▒░░        ░░██▓▓▒▒▒▒██▒▒
    ▒▒██▓▓▒▒▓▓██░░      ░░████░░      ░░██▓▓▒▒▓▓██▒▒
    ░░██▓▓▒▒▓▓██▓▓░░░░░░▓▓████▓▓░░░░░░▓▓██▓▓▒▒▓▓██░░
      ▓▓▓▓▒▒▒▒▓▓██▓▓██▓▓██▓▓▓▓██▓▓██▓▓██▓▓▒▒▒▒▓▓▓▓
      ▒▒██▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓██▒▒
      ░░▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓░░
        ▒▒██▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓██▒▒
          ▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓
            ▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓
              ▒▒▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▒▒
                ░░▒▒▓▓████████████▓▓▒▒░░
                      ░░▒▒▒▒▒▒▒▒░░

    Icon (32 x 32):
                              ░░░░▒▒▒▒░░░░
                        ▒▒▓▓▓▓▓▓████████▓▓▓▓▓▓▒▒
                    ▒▒▓▓██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▓▓▒▒
                ░░▓▓▓▓▓▓▓▓▓▓▒▒▒▒░░░░░░░░▒▒▒▒▓▓▓▓▓▓▓▓▓▓░░
              ░░▓▓▓▓██▓▓▒▒░░░░░░░░░░░░░░░░░░░░▒▒▓▓██▓▓▓▓░░
            ░░▓▓▓▓▓▓▒▒░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▓▓▓▓██░░
          ░░▓▓▓▓▓▓▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒▓▓▓▓▓▓░░
          ▓▓▓▓▓▓▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒▓▓▓▓▓▓
        ▒▒████▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒████▒▒
        ▓▓▓▓▓▓░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▓▓▓▓▓▓
      ▒▒██▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒████▒▒
      ▓▓▓▓▓▓░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▓▓▓▓▓▓
      ▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓
    ░░██▓▓▒▒▒▒▓▓██▓▓████████████████████████████████▓▓██▓▓▒▒▒▒▓▓██░░
    ░░██▓▓▒▒▒▒▓▓██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▓▓▒▒▒▒▓▓██▒▒
    ▒▒██▓▓▒▒▒▒▓▓▓▓██░░                            ░░██▓▓▓▓▒▒▒▒▓▓██▒▒
    ▒▒██▓▓▒▒▒▒▓▓▓▓██░░            ░░░░            ░░██▓▓▓▓▒▒▒▒▓▓██▒▒
    ░░██▓▓▒▒▒▒▓▓▓▓██░░          ▒▒████▒▒          ░░██▓▓▓▓▒▒▒▒▓▓██▒▒
    ░░██▓▓▓▓▒▒▓▓▓▓██▒▒          ▓▓▓▓▓▓▓▓          ▒▒██▓▓▓▓▒▒▓▓▓▓██░░
      ▓▓▓▓▓▓▒▒▒▒▓▓██▓▓▒▒░░░░░░▓▓██▓▓▓▓██▓▓░░░░░░▒▒▓▓██▓▓▒▒▒▒▓▓▓▓▓▓
      ▓▓▓▓▓▓▒▒▒▒▓▓██▓▓████████▓▓██▓▓▓▓██▓▓████████▓▓██▓▓▒▒▒▒▓▓▓▓▓▓
      ▒▒██▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▓▓██▒▒
        ▓▓▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▓▓
        ▒▒██▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓██▒▒
          ▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓
          ░░▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓░░
            ░░██▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓██░░
              ░░▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓░░
                ░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░
                    ▒▒▓▓██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▓▓▒▒
                        ▒▒▓▓▓▓████████████▓▓▓▓▒▒
                              ░░▒▒▒▒▒▒▒▒░░

    Icon (48 x 48):
    (not shown)
    Icon (64 x 64):
    (not shown)
    Icon (128 x 128):
    (not shown)

_NET_WM_ICON_NAME(UTF8_STRING) =
XdndAware(ATOM) = BITMAP
WM_NAME(STRING) = "Error"
_NET_WM_NAME(UTF8_STRING) = "Error"
_MOTIF_WM_HINTS(_MOTIF_WM_HINTS) = 0x3, 0x26, 0x1e, 0x0, 0x0
_NET_WM_WINDOW_TYPE(ATOM) = _NET_WM_WINDOW_TYPE_DIALOG, _NET_WM_WINDOW_TYPE_NORMAL
_XEMBED_INFO(_XEMBED_INFO) = 0x0, 0x1
WM_CLIENT_LEADER(WINDOW): window id # 0x2200009
WM_HINTS(WM_HINTS):
Client accepts input or input focus: True
window id # of group leader: 0x2200009
WM_CLIENT_MACHINE(STRING) = "gentoo-thinkpad"
_NET_WM_PID(CARDINAL) = 3459957
_NET_WM_SYNC_REQUEST_COUNTER(CARDINAL) = 35651614
_GTK_APPLICATION_ID(UTF8_STRING) = "com.vector35.binaryninja"
_KDE_NET_WM_DESKTOP_FILE(UTF8_STRING) = "com.vector35.binaryninja"
WM_CLASS(STRING) = "binary-ninja", "Binary Ninja"
WM_PROTOCOLS(ATOM): protocols WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING, _NET_WM_SYNC_REQUEST
WM_NORMAL_HINTS(WM_SIZE_HINTS):
user specified location: 276, 547
user specified size: 500 by 103
program specified minimum size: 500 by 103
program specified maximum size: 500 by 103
window gravity: Static

---

Transmission sometimes open a floating window that hides behind it for some reason. Start a torrent then ask to delete it while it's running.

WM_STATE(WM_STATE):
window state: Normal
icon window: 0x0
_NET_WM_DESKTOP(CARDINAL) = 0
_NET_WM_ALLOWED_ACTIONS(ATOM) = _NET_WM_ACTION_CLOSE
_NET_WM_USER_TIME(CARDINAL) = 1672243
_NET_WM_STATE(ATOM) = _NET_WM_STATE_MODAL
WM_TRANSIENT_FOR(WINDOW): window id # 0x3a00007
_NET_WM_ICON_NAME(UTF8_STRING) =
_NET_WM_ICON(CARDINAL) = Icon (128 x 128):
(not shown)

XdndAware(ATOM) = BITMAP
WM_NAME(STRING) = " "
_NET_WM_NAME(UTF8_STRING) = " "
_MOTIF_WM_HINTS(_MOTIF_WM_HINTS) = 0x3, 0x26, 0x1e, 0x0, 0x0
_NET_WM_WINDOW_TYPE(ATOM) = _NET_WM_WINDOW_TYPE_DIALOG, _NET_WM_WINDOW_TYPE_NORMAL
_XEMBED_INFO(_XEMBED_INFO) = 0x0, 0x1
WM_CLIENT_LEADER(WINDOW): window id # 0x3a00009
WM_HINTS(WM_HINTS):
Client accepts input or input focus: True
window id # of group leader: 0x3a00009
WM_CLIENT_MACHINE(STRING) = "gentoo-thinkpad"
_NET_WM_PID(CARDINAL) = 77933
_NET_WM_SYNC_REQUEST_COUNTER(CARDINAL) = 60817453
_GTK_APPLICATION_ID(UTF8_STRING) = "transmission-qt"
_KDE_NET_WM_DESKTOP_FILE(UTF8_STRING) = "transmission-qt"
WM_CLASS(STRING) = "transmission-qt", "transmission"
WM_PROTOCOLS(ATOM): protocols WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING, _NET_WM_SYNC_REQUEST
WM_NORMAL_HINTS(WM_SIZE_HINTS):
user specified location: 619, 522
user specified size: 472 by 102
program specified minimum size: 472 by 102
program specified maximum size: 472 by 102
window gravity: Static
