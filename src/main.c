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
2. invoke libgdb from same programme and start a session - MISSION ACCOMPLISHED
   * perhaps use argv[1] to specify executable - DONE
3. allow setting and unsetting of gdb breakpoints visually
   * keyboard -- PARTIAL
   * mouse
4. file browsing/unload/load
   * keyboard
   * mouse
5  visual stepping/focuse of code on spacebar or sthng easy - PARTIAL
*/

/* TODO
(1)- call bt every step and display
(2)- file change when nexti or stepi
(3)- file selection
-- (c)ontinue
-- (i)nterrupt
-- (s)tep
-- nexti stepi
-- assembly side-by-side
-- memory window

GDB Mode
--------
* format strings before printing
* scroll with spacebar?
* move to rhs so can be longer?

GUI Overall
-----------
* left/right arrows or tab do focus shift
* display files in lhs
* arbitrary KEY "COMMAND STRING" bindings file?

Debugging
---------
* unset breakpoint with spacebar
* change toggle() to set() and unset()
* stack trace on rhs
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

#define BIN "/usr/bin/gdb"

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
	if (w.ws_row < 50 || w.ws_col < 100) {
		log_err ("your terminal is too small for //EXTERMINATOR\\\\\n"
			"if you're in a window, try resizing to fullscreen or > 100 cols\n");
		return 0;
	}
	
	setlocale (LC_ALL, "");
	start_ncurses_defaults ();
	draw_defaults ();
	
	char** child_argv = NULL;
	child_argv = (char**)malloc (sizeof (char*) * (argc + 2));
	size_t len = strlen (BIN);
	child_argv[0] = (char*)malloc (len + 1);
	strcpy (child_argv[0], BIN);
	for (int i = 1; i < argc; i++) {
		len = strlen (argv[i]);
		child_argv[i] = (char*)malloc (len + 1);
		strcpy (child_argv[i], argv[i]);
	}
	len = strlen ("--interp=mi");
	child_argv[argc] = (char*)malloc (len + 1);
	strcpy (child_argv[argc], "--interp=mi");
	// must have a null ptr at end of list or explodes
	child_argv[argc + 1] = NULL;
	
	if (!start_ipc (child_argv)) {
		return 1;
	}
	
	for (int i = 0; i < argc + 1; i++) {
		assert (child_argv[i]);
		//free (child_argv[i]);
		child_argv[i] = NULL;
	}
	assert (child_argv);
	//free (child_argv);
	child_argv = NULL;

	// return to normal terminal
	endwin ();

	return 0;
}

