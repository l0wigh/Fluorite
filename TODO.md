# TODO !
    - [X] Redraw every monitor when theme is changed !
    - [ ] Issues with border colors when sending windows to another visible workspace
    - [ ] Find a way to fix the fullscreen float window bug
        - open a floating window during fullscreen and then deactivate fullscreen -> BREAK !

# PARTIALLY DONE
- [-] Style the slaves stacking design
    - [ ] Add a right Master option
    - [ ] Add an option to do top-right to bottom-left, insteam of top-left to bottom-right (?) 
- [-] Fix the issue where windows are not resized to the frame size
    - [ ] Look for the XConfigure and the reparenting
    - [ ] Look for hints/atoms/attributes
    - [x] Try hacky fixes with XMove and XResize windows before reparenting. DWM seems to do a XMoveResize even if not needed (IF ALL ELSE FAILS) 
        - Seems to be the only solution right now, but it causes some kind of jumping on master switching
        - Firefox still stuggle sometimes (mainly at the opening)

# Not Really Important
- [ ] Fix the popup window not showing as popup (something with atoms or hints ? i think ragnar does it nicely)
- [ ] Fix windows that can be moved when inside tiling (thunar for example)
- [ ] Find why some apps make fullscreen bugs when closing (feh for example)
- [ ] Document every functions
- [-] Better floating default position
    - Seems to not work on some apps

# DONE !
- [x] Fixing the multi closing issue
    - Fixed with XSync(fluorite.display, True) and X(Un)GrabServer
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
- [x] Fixed issues with dmenu, rofi, firefox
    - Was just a bad variable management inside the unmap function
    - Added a closed variable and moving `fluorite.frames_count` inside the if statement fixed it
- [x] Add Workspaces
- [x] Add Fullscreen mode
- [x] Fix the closing windows outside the current workspaces
    - Should be fixed, stay vigilent for potential bugs after the whole rewrite of unmapping and changes made to organise stack
- [x] Check the binding for the "à" in XK_, it seems to not be working
- [x] Better closing function, avoid issues with Firefox for example
- [x] Keyboard focus on first slaves if mouse enter
    - Still apply Fluorite bindings based on the master window
- [x] Opening two thunar and closing one, makes the window manager dies
    - Fixed itself for some reason
- [x] Killing a firefox instance, kills all Firefox instances
    - Fixed by itself ?
- [x] Clean the SAME_BORDER code, it's now useless
- [x] Remove FluoriteBar code since it's not usefull any more (just use polybar !)
- [x] Add a force new master focus to the EnterNotify (REALLY NEEDED)
    - Might need some more testing
- [x] Fix writing of wifi module when not connected
- [x] Create a make install
- [x] Closing binding now have effect on the focused window
    - Might cause some issues
- [x] Moving window to another workspace now have effect on the focused window
    - Might cause some issues
- [x] Resize Window go to negative value
- [x] New binding to restore default master offset
- [x] Adding a default master offset value in config file
- [x] Find a way to only have one border when stacked
- [x] Handle fullscreen toggle when it's requested on floating
- [x] Sometimes floating will not go back to layout, use another checking method
- [x] Add a way to kill popup windows with keybinds when focused
- [x] Tray seems broken, maybe coming from floating window stuff
- [x] Fix floating movement locking
    - Block the good value based on what part will leave the current monitor
- [x] Fix Fullscreen Mode
    - Switching workspaces works badly
- [x] Fix focus when switching between workspaces that are in different monitors
    - Both keeps border and focus might be wrong
- [x] Slaves rotations keep focus on the previous slave
- [x] Border are still there on Fullscreen windows ! Oh no ! Anyways...
    - Set border to 0
- [x] Add config to select predefined workspaces for monitor
- [x] Fix the custom workspace names
- [x] Check monitor every mouse movements
    - Needs more testing to see if it's stable
- [x] Closing **last** slave window bring focus back on master window but, bindings doesn't works
    - Probably something to do with the unmap function that need to change **fluorite.workspaces[fluorite.current_workspace].current_focus**
    - Should be fixed, by hard refocus master window on unmapping
- [x] Fix floating windows
    - [x] Switching workspaces should move in X or Y the window based on the new monitor
    - [x] Sometimes it doesn't open in the right spot even with safe guard. But hiding and showing them again works.
        - Fixed by forcing it to a size and a position (not that great, it removes the flexibility)
- [x] Add monitor focus change binding
- [x] Sending window to a fullscreen workspace cause big issue
    - Close fullscreen on new workspace if needed
- [x] Create a function to set windows property automatically.
    - Required by swallowing mode on unmapping
    - Could reduce bloat
