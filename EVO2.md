# TEST
- Nothing

# TODO
- [ ] Add the possibility to seamlessly move a floating window to another monitors and that it change it's workspace too
- [ ] ? Add a way to recognize a missplaced floating and fix it's workspace to the current monitor it's in
- [ ] Add Tags like DWM
- [ ] Add EWMH
    - https://specifications.freedesktop.org/wm-spec/1.3/

# BUG
- [ ] Find a way to verify certains values
    - fl_hdn
- [ ] Ghidra floating window, not nicely placed and focus + warp...

# CLEAN
- [ ] Check for memory leaks
    - Might not leak but in btop at startup 4M and after a while 6M
    - This might be caused because I'm managing more windows and be normal then
- [ ] Reset borders function
    - Find function that do loops through stacks and XSetWindowBorder

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
- [X] Add FSendWindowsTo(Next/Prev)Workspace
- [X] Add Monitors
- [X] Cursor Warp doesn't occur when coming from an empty monitor
- [X] Floating windows doesn't move to the right spot when swapping monitors
- [X] No refocus of fullscreen monitors when moving mouse
- [X] Sending windows to a fullscreen workspace might lead to issues
