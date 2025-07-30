# TODO
- [ ] Add FSendWindowsTo(Next/Prev)Workspace
- [ ] Add Tags like DWM
- [ ] Add Monitors
- [ ] Add EWMH
    - https://specifications.freedesktop.org/wm-spec/1.3/

# BUG

# DONE
- [X] Add Stacking layout
- [X] Create Warping Cursor function use it everywhere
- [X] TileAll doesn't work and do only one window
- [X] Add Fullscreen
- [X] Firefox creating ghost windows
- [X] Firefox refuse to be grabbed ????
- [X] Check if new window pid is already opened, if so
    - It open the workspace containing the window
    - Maybe change it for something like this :
        - Send important notice on workspace
        - Move the window
- [X] Fix Fluorite layout
- [X] Swapping focus use fc variable, but most of the time, focus is set in another way.
- [X] Pavu control open in a weird state just like it create invisible windows and lock Fluorite
    - Probably fixed since pid + toplevel check
