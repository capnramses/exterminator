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
2. invoke libgdb from same programme and start a session
   * perhaps use argv[1] to specify executable
3. allow setting and unsetting of gdb breakpoints visually
   * keyboard
   * mouse
4. file browsing/unload/load
   * keyboard
   * mouse
*/

/* TODO
* rewrite/remove createwin wipewin
*/

#include "utils.h"
#include "wins.h"
#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
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
chgat - changes some but not all characters after cursor (for fuck's sake)

changing the background colour of a window can make stuff flash for attention

*/

/*WINDOW* create_win (int w, int h, int xi, int yi, bool box, int pair) {
	// note backwards x,y convention
	WINDOW* win = newwin (h, w, yi, xi);
	if (box) {
		box (win, 0, 0); // default surrounding chars
	}
	//assert (wbkgd (win, COLOR_PAIR (pair)) > -1);
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
}*/

int main (int argc, char** argv) {
	if (argc < 2) {
		printf ("usage is ./exterminator my_text_file.c\n");
		return 0;
	}

	setlocale (LC_ALL, "");

	if (!restart_log ()) {
		return 1;
	}
	
//--------
	log_msg ("reading file %s\n", argv[1]);
	long int sz = 0;
	char* blob = read_entire_file (argv[1], &sz);
	assert (blob);
	
	long int lc = count_lines_in_blob (blob, sz);
	log_msg ("lines in blob is %li\n", lc);
	
	Line_Meta* lms = get_line_meta_in_blob (blob, sz, lc);
	assert (lms);
	
	//print_lines (lms, blob, lc);
//--------
	
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
	
	set_tabsize (2);
	start_color ();
	// get unbuffered input and disable ctrl-z, ctrl-c
	raw ();
	// stop echoing user input
	noecho ();
	// enable F1 etc.
	keypad (stdscr, TRUE);
	init_pair (1, COLOR_WHITE, COLOR_BLUE);
	init_pair (2, COLOR_WHITE, COLOR_BLACK);
	init_pair (3, COLOR_BLACK, COLOR_RED);
	init_pair (4, COLOR_GREEN, COLOR_BLACK);
	init_pair (5, COLOR_BLUE, COLOR_CYAN);
	init_pair (6, COLOR_BLACK, COLOR_WHITE);
	
	// this need to be here before windows or it wont work
	refresh ();
	
	// windows
	// -------
	/*WINDOW* title_win = create_win (184, 1, 0, 0, false, 4);
	mvwprintw (title_win, 0, 84, "// EXTERMINATOR \\\\");
	wrefresh (title_win);
	WINDOW* menu_win = create_win (184, 1, 0, 1, false, 3);
	wprintw (menu_win, "FILE   EDIT   VIEW   DEBUG   THINGY");
	wrefresh (menu_win);
	WINDOW* browse_win = create_win (20, 50, 0, 2, true, 1);
	{ // TODO make me a redraw function
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
		}
		wrefresh (browse_win);
	}
	WINDOW* bp_win = create_win (1, 48, 20, 3, false, 3);
	WINDOW* linno_win = create_win (4, 48, 21, 3, false, 2);
	
	
	WINDOW* code_win = create_win (120, 50, 25, 2, false, 1);*/
	
	// title bar
	attron(COLOR_PAIR(6));
	mvprintw (0, 40, "//EXTERMINATOR\\\\");
	attroff(COLOR_PAIR(6));
	// line numbers bar
	redraw_line_nos (1, 49, lc);
	// break points bar
	redraw_bp_bar (0, 48, lc, lms);
	// source text window
	write_blob_lines (0, 48, blob, lc, lms, argv[1], 0);

/*
	WINDOW* watch_win = create_win (40, 30, 145, 2, true, 1);
	WINDOW* st_win = create_win (40, 30, 145, 32, true, 1);
	WINDOW* op_win = create_win (144, 10, 0, 52, false, 1);
	wprintw (op_win, "debug output goes here1\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here2\n");
	wprintw (op_win, "debug output goes here11\n");
	wrefresh (op_win);
	*/
	long int x = 0, y = 0;
	//wmove (code_win, y, x);
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
		
		bool lchange = false;
		// keys list http://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html
		switch (c) {
			case KEY_UP: {
				if (y > 0) {
					y--;
					lchange = true;
				}
				break;
			}
			case KEY_DOWN: {
				if (y < lc - 1) {
					y++;
					lchange = true;
				}
				break;
			}
			case KEY_NPAGE: {
				if (y + 50 < lc) {
					y += 50;
					lchange = true;
				}
				break;
			}
			case KEY_PPAGE: {
				if (y - 50 > 0) {
					y -= 50;
					lchange = true;
				}
				break;
			}
			case 32: {
				assert (toggle_bp (lms, y, lc));
				lchange = true;
				break;
			}
			default: {}
		}
		
		if (lchange) {
			erase ();
			
			// NOTE: redudant rendering b/c erased whole thing
			attron(COLOR_PAIR(6));
			mvprintw (0, 40, "//EXTERMINATOR\\\\");
			attroff(COLOR_PAIR(6));
			
			// line numbers bar
			redraw_line_nos (1, 49, lc);
			// break points bar
			redraw_bp_bar (0, 48, lc, lms);
			// source text window
		
			write_blob_lines (0, 48, blob, lc, lms, argv[1], y);
			//redraw_line_nos (linno_win, y + 1, 48 + y, lc);
			// cursor and line highlight
			// there are window-specific versions of these but i want to cover a
			// bunch of stuff with one highlight?
			//move (y + 3, x + 20);
			//int chgat(int n, attr_t attr, short color, const void *opts)
			// attribs are A_BLINK, A_BOLD, A_DIM, A_REVERSE, A_STANDOUT and A_UNDERLINE. ??
			//chgat (125, COLOR_PAIR (4), COLOR_PAIR (4), NULL);
			//log_msg ("y%i x%i\n", y, x);
			refresh ();
		}
		
	}

	// return to normal terminal
	endwin ();
	
	// free mem
	// --------


	return 0;
}

