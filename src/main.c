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
//#include "apg_data_structures.h"
#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <dirent.h> // directory contents POSIX systems
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>

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

int main (int argc, char** argv) {
	if (!restart_log ()) {
		return 1;
	}

//------- fork for gdb/mi
	log_msg ("forking child process...\n");
	int pipefd[2];
	pid_t cpid;
	char buf;
	
	pipe (pipefd); // create pipe
	cpid = fork (); // duplicate this proc
	if (cpid == 0) { // i am child
		log_msg ("i am the child\n");
		
		// TODO -- start the child's function and separate log
		// -- mirror all gdb output in said log to test it
	
		return 0;
	} else { // i am parent
	
	}
//-------

	if (argc < 2) {
		printf ("usage is ./exterminator my_text_file.c\n");
		return 0;
	}

	setlocale (LC_ALL, "");

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
	//noecho ();
	// enable F1 etc.
	keypad (stdscr, TRUE);
	init_pair (1, COLOR_WHITE, COLOR_BLUE); // src, line #s
	init_pair (2, COLOR_WHITE, COLOR_BLACK); // borders, bp bar
	init_pair (3, COLOR_BLACK, COLOR_RED); // red break points
	init_pair (4, COLOR_BLACK, COLOR_WHITE); // side panels
	init_pair (5, COLOR_BLUE, COLOR_CYAN); // current src line
	init_pair (6, COLOR_RED, COLOR_WHITE); // title bars
	
	// set-up watch list
	SLL_Node* watch_list = NULL;
	add_to_watch ("input_y", "60", &watch_list);
	add_to_watch ("input_x", "2", &watch_list);
	// and stack list
	SLL_Node* stack_list = NULL;
	//add_to_stack ("hmm", &stack_list);
	add_to_stack ("#0  main (argc=1, argv=0x7fffffffde98) at src/main.c:55",
		&stack_list);
	
	// title bar
	write_title_bars ();
	write_left_side_panel ();
	// line numbers bar
	redraw_line_nos (1, 49, lc);
	// break points bar
	redraw_bp_bar (0, 48, lc, lms);
	// source text window
	write_blob_lines (0, 48, blob, lc, lms, argv[1], 0);
	write_watch_panel (watch_list);
	write_stack_panel (stack_list);

	int input_y = 60;
	int input_x = 2;
	move (input_y, input_x);
	// this need to be here before windows or it wont work
	refresh ();

	long int y = 0;
	long int start_ln = 0;
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
				// move line cursor
				if (y > 0) {
					y--;
					lchange = true;
				// scroll page
				} else {
					if (start_ln > 0) {
						start_ln--;
						lchange = true;
					}
				}
				break;
			}
			case KEY_DOWN: {
				// move line cursor
				// not past end of doc even if room on pg
				if (start_ln + y < lc - 1) {
					if (y < 48) {
						y++;
						lchange = true;
					// scroll page
					} else {
						if (start_ln < lc - 1) {
							start_ln++;
							lchange = true;
						}
					}
				}
				break;
			}
			case KEY_NPAGE: {
				start_ln = MAX (MIN (start_ln + 50, lc - 49), 0);
				lchange = true;
				break;
			}
			case KEY_PPAGE: {
				start_ln = MAX (start_ln - 50, 0);
				lchange = true;
				break;
			}
			case 32: {
				assert (toggle_bp (lms, y + start_ln, lc));
				lchange = true;
				break;
			}
			default: {}
		}
		
		if (lchange) {
			erase ();
			
			write_title_bars ();
			write_left_side_panel ();
			// line numbers bar
			redraw_line_nos (start_ln + 1, start_ln + 49, lc);
			// break points bar
			redraw_bp_bar (start_ln, start_ln + 48, lc, lms);
			// source text window
			write_blob_lines (start_ln, start_ln + 48, blob, lc, lms, argv[1],
				start_ln + y);
			write_watch_panel (watch_list);
			write_stack_panel (stack_list);
			move (input_y, input_x);
			refresh ();
		}
		
	}

	// return to normal terminal
	endwin ();
	
	// free mem
	// --------


	return 0;
}

