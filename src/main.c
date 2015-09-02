#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <stdbool.h>
#include <dirent.h> // directory contents POSIX systems
#include <stdlib.h>
#include <string.h>

/*
http://invisible-island.net/ncurses/man/
http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
windows
pads (for windows with big scrolling contents)
panels (overlapping windows)
ncursesw "wide" lib for wchars
menus library
*/

WINDOW* create_win (int w, int h, int xi, int yi, bool box, int pair) {
	// note backwards x,y convention
	WINDOW* win = newwin (h, w, yi, xi);
	if (box) {
		box (win, 0, 0); // default surrounding chars
	}
	assert (wbkgd (win, COLOR_PAIR (pair)) > -1);
	wrefresh (win);
	return win;
}

void wipe_win (WINDOW* win) {
	// clear window borders
	wborder (win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	// contents
	wrefresh (win);
	// delete
	delwin (win);
	win = NULL;
}

int main () {

	setlocale (LC_ALL, "");
	// start ncurses terminal
	initscr ();

	/*
	COLOR_BLACK   0
 	COLOR_RED     1
 	COLOR_GREEN   2
 	COLOR_YELLOW  3
 	COLOR_BLUE    4
 	COLOR_MAGENTA 5
 	COLOR_CYAN    6
	COLOR_WHITE   7
	*/
	// rgb 0-1000
	
	
	start_color ();
	// get unbuffered input and disable ctrl-z, ctrl-c
	raw ();
	// stop echoing user input
//	noecho ();
	// enable F1 etc.
	keypad (stdscr, TRUE);
	init_pair (1, COLOR_WHITE, COLOR_BLUE);
	init_pair (2, COLOR_WHITE, COLOR_BLACK);
	init_pair (3, COLOR_RED, COLOR_WHITE);
	init_pair (4, COLOR_GREEN, COLOR_BLACK);

	attron (COLOR_PAIR (1));
	printw ("EXTERMINATOR\n");
	refresh ();
	attroff (COLOR_PAIR (1));
	
	// windows
	// -------
	WINDOW* title_win = create_win (184, 1, 0, 0, false, 4);
	mvwprintw (title_win, 0, 84, "// EXTERMINATOR \\\\");
	wrefresh (title_win);
	WINDOW* menu_win = create_win (184, 1, 0, 1, false, 3);
	wprintw (menu_win, "FILE   EDIT   VIEW   DEBUG   THINGY");
	wrefresh (menu_win);
	WINDOW* browse_win = create_win (20, 50, 0, 2, true, 1);
	{
		DIR* dirp;
		struct dirent* direntp;
		int n = 0;
		dirp = opendir (".");
		if (dirp) {
			direntp = readdir (dirp);
			while (direntp) {
				char tmp[18];
				strncpy (tmp, direntp->d_name, 18);
				mvwprintw (browse_win, n + 1, 1, "%s", tmp);
				direntp = readdir (dirp);
				n++;
			}
			closedir (dirp);
		} else {
			printf ("NOPE!\n");
		}
		wrefresh (browse_win);
	}
	WINDOW* bp_win = create_win (1, 48, 20, 3, false, 3);
	WINDOW* linno_win = create_win (3, 48, 21, 3, false, 2);
	for (int i = 0; i < 48; i++) {
		wprintw (linno_win, "%3i", i + 1);
	}
	wrefresh (linno_win);
	WINDOW* code_win = create_win (120, 50, 24, 2, false, 1);
	mvwprintw (code_win, 0, 1, "main.c");
	
	{ // TODO load into dynamic list thingy
		// TODO should this be a pad?
		FILE* f = fopen ("src/main.c", "r");
		assert (f);
		char line[118];
		int n = 0;
		while (fgets (line, 118, f)) {
			line[117] = '\0';
			mvwprintw (code_win, 1 + n, 1, "%s", line);
			n++;
		}
		fclose (f);
	}
	box (code_win, 0, 0);
	wrefresh (code_win);
	WINDOW* watch_win = create_win (40, 30, 144, 2, true, 1);
	WINDOW* st_win = create_win (40, 30, 144, 32, true, 1);
	
	while (1) {
		// can use this to wait only 1/10s for user input
		// halfdelay()

		// print to back buffer
		// flip buffers
		//refresh ();

		int c = getch ();
		// quit on ESC
		if (27 == c) {
			break;
		}
	}

	// return to normal terminal
	endwin ();

	return 0;
}
