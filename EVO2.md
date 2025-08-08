# TODO
- [ ] Add the possibility to seamlessly move a floating window to another monitors and that it change it's workspace too
- [ ] ? Add a way to recognize a missplaced floating and fix it's workspace to the current monitor it's in
- [ ] Add Tags like DWM
- [ ] Add EWMH
    - https://specifications.freedesktop.org/wm-spec/1.3/
- [ ] Add Organizer back !

# BUG
- [ ] Centered layout needs to have more horizontal gaps space between windows in columns
- [ ] Find a way to verify certains values
    - fl_hdn
- [ ] Ghidra floating window, not nicely placed and focus + warp...

# CLEAN
- [ ] Reset borders function
    - Find function that do loops through stacks and XSetWindowBorder

# IDEAS
- Find a way to determine if a new window is linked to another one already present.
    - if yes, I might be able to make them float on top of the "toplevel" window

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
- [X] Add function FRemoveActiveWindow and search for net_active_window
- [X] Still having issues with fullscreen from time to time
- [X] Going to a fullscreen monitor does not reapply active window name
- [X] Swallowing a window when in fullscreen quit the fullscreen visually, but not in the variables
- [X] Switching ws sometimes does not reapply active window name and doesn't warp cursor
- [X] Check for memory leaks
    - Seems fine now. I needed to free useless nw in Map function
- [X] Centered layout needs to have vertical gap on the right side of the master window
- [X] Swallowed windows seems to open in fullscreen for some reason
- [X] Refocus the previous tiled focus window when hiding floating (or FResetFocus)
- [X] Crash when swapping master with a floating window
- [X] Hiding floating windows, only hide the first one
- [X] Floating windows still doesn't appaers when a new one is opened
- [X] Memory leak coming from Xresources I guess.
    - Seems corrected
