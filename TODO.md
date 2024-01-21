# TODO !
- [ ] Fix the popup window not showing as popup (something with atoms or hints ? i think ragnar does it nicely)
    - [ ] Fix dmenu issues. This might be cause by the fact that it works as a popup and that I don't have to manage it
- [ ] Seems like firefox making the master layout die (and cause real issues)
    - [ ] Again, probably an issues with how lighly windows are managed
        - Maybe try to first test if the mapping event isn't done by an already opened window
- [ ] Clean the code
- [ ] Add transparent windows (XCreateWindows and not XCreateSimpleWindows)
- [ ] Fix the gaps and position, the master window is slightly too big and not centered by default
- [ ] Polybar is crashing, FUCK !
    - Clearly because I'm not opening "floating" windows properly (maybe DWM have a solution for this)

# PARTIALLY DONE
- [-] Style the slaves stacking design
    - [ ] Add a right Master option
    - [ ] Add an option to do top-right to bottom-left, insteam of top-left to bottom-right (?) 
- [-] Fix the issue where windows are not resized to the frame size
    - [ ] Look for the XConfigure and the reparenting
    - [ ] Look for hints/atoms/attributes
    - [x] Try hacky fixes with XMove and XResize windows before reparenting. DWM seems to do a XMoveResize even if not needed (IF ALL ELSE FAILS) 
        - Seems to be the only solution right now, but it causes some kind of jumping on master switching

# DONE !
- [x] Fixing the multi closing issue
- [x] Add and manage the config file
- [x] Add rotations to slaves stack
- [x] Add master window resize
- [x] Bind global keys
    - Needs to define the keyboard inside xinitrc with `setxkbmap -layout fr`
- [x] Fixing the not full focus issue. Probably something to do with atoms, hints, or window config
    - Might be a partial fix: XSetInputFocus on `master_winframe->window`
