//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

/*
GOALS (in priority order)
0. WORKS ON LINUX
1. RUNS FAST
2. LOADS FAST

MISSIONS
1. super-fast text file loading and display with ncurses - MISSION ACCOMPLISHED
2. invoke gdb from same programme and start a session - MISSION ACCOMPLISHED
   * perhaps use argv[1] to specify executable - DONE
3. allow setting and unsetting of gdb breakpoints visually
   * keyboard -- PARTIAL
   * mouse
4. file browsing/unload/load
   * keyboard
   * mouse
5  visual stepping/focuse of code on spacebar or sthng easy - MISSION ACCOMPLD.
*/

/* TODO
(2)-unset breakpoints
(3)- file selection
(4) (c)ontinue
(5) (i)nterrupt
(later)
-- assembly side-by-side
-- nexti stepi
-- memory window
-- ?

GUI Overall
-----------
* display files in lhs
* left/right arrows or tab do focus shift

Debugging
---------
* unset breakpoint with spacebar
* change toggle() to set() and unset()
*/

/* IDEAS
* http://invisible-island.net/ncurses/man/
* http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
* ncursesw "wide" lib for wchars
* changing the background colour of a window can make stuff flash for attention
* test with 1GB source file -- will need malloc instead of blob static mem
*/

#include "utils.h"
#include "wins.h"
#include "ipc.h"
#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <dirent.h> // directory contents POSIX systems
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main (int argc, char** argv) {
	if (!restart_log ()) {
		return 1;
	}

	if (argc < 2) {
		printf ("usage is ./exterminator target\n");
		return 0;
	}
	
	// whine about terminal size
	struct winsize w;
	ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
	if (w.ws_row < 60 || w.ws_col < 236) {
		log_err ("your terminal is too small for //EXTERMINATOR\\\\\n"
			"if you're in a window, resize to > 236 cols and > 60 rows\n");
		return 0;
	}
	
	setlocale (LC_ALL, "");
	start_ncurses_defaults ();
	draw_defaults ();
	
	if (!start_ipc (argc, argv)) {
		return 1;
	}

	// return to normal terminal
	endwin ();

	return 0;
}

