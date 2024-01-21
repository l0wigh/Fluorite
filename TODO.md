# TODO !
- [ ] Fix the popup window not showing as popup (something with atoms or hints ? i think ragnar does it nicely)
    - dmenu, firefox, rofi, polybar issues are all linked to this. FIX IT !!!
- [ ] Clean the code
- [ ] Fix the gaps and position, the master window is slightly too big and not centered by default

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
- [x] Add transparent windows (XCreateWindows and not XCreateSimpleWindows)
    - Seems to be working, Might be wrong in some cases
    - Seems like picom can't affect it and should be done inside program config
    - Maybe try to find a way to do it inside config file
