#exterminator

Deadly ex-perimental termin-al debugger (it's an ncurses front-end for GDB). Written in C99.

![screenshot](shots/crongscroll.png)

Because modern IDEs are inexcusably slow, and why can't I just have GDB with a better (Turbo C style) UI and less typing?

##goals

* runs on GNU/Linux
* interactive visual output
* loads immediately, no mess, no fuss
* runs super-fast - instant stepping, quick key commands
* no dependencies or middleware beyond GDB and ncurses

##platforms

* GNU/Linux is the target platform
* It accidentally works on OS X because BSD
* MS Windows is not a build target

##major to-dos

* un-set breakpoints
* goto line button
* step-ins with displayed file change, and stack update
* left-hand bar with source files to switch to
* side-by-side asm (later)
* memory inspection window (later)

##status

* most important features working but not robust
* starts a debugging session, loads text of main file, allows visual breakpoints, run, next (step), gdb console, watch list, stack trace
* no file change/browse yet
* has some memory crash errors / malloc and string lengths need tidy

##building instructions

`make`

##basic usage

* maximise your terminal window and/or reduce font size to fit more on display
* launch with: `./exterminator target_prog [targetargs]` 
* source file containing entry point is immediately displayed.
* scroll through lines with cursor keys
* set a break point with `b`. you will see break points displayed in the margin.
* run debug session press `r`
* step to the next line press `spacebar` or `n`
* add a variable to the watch list with `w`
* type more detailed commands by opening GDB terminal `g`.
* press `ESC` to leave GDB terminal mode
* press `ESC` to quit programme
