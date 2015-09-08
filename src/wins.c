//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "wins.h"
#include "parse.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

SLL_Node* watch_list = NULL;

void start_ncurses_defaults () {
	// return to normal terminal whenever we exit -- sanity!
	atexit (nice_exit);

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
	init_pair (1, COLOR_WHITE, COLOR_BLUE); // src, line #s
	init_pair (2, COLOR_WHITE, COLOR_BLACK); // borders, bp bar
	init_pair (3, COLOR_BLACK, COLOR_RED); // red break points
	init_pair (4, COLOR_BLACK, COLOR_WHITE); // side panels
	init_pair (5, COLOR_BLUE, COLOR_CYAN); // current src line
	init_pair (6, COLOR_RED, COLOR_WHITE); // title bars
	init_pair (7, COLOR_YELLOW, COLOR_BLUE); // comments in src
	init_pair (8, COLOR_BLACK, COLOR_RED); // debug op
}

void nice_exit () {
	endwin ();
}

void draw_defaults () {
	// and stack list
	SLL_Node* stack_list = NULL;
	//add_to_stack ("hmm", &stack_list);
	add_to_stack ("#0  main (argc=1, argv=0x7fffffffde98) at src/main.c:55",
		&stack_list);
	
	// title bar
	write_title_bars ();
	write_left_side_panel ();
	write_watch_panel (watch_list);
	write_stack_panel (stack_list);

	move (CURS_Y, CURS_X);
	// this need to be here before windows or it wont work
	refresh ();
}

// start line is 0 but will be displayed as 1 in side bar
void write_blob_lines (int startl, int endl, char* blob,
	long int lc, Line_Meta* lms, const char* file_name, int highlighted_line) {
	char tmp[101]; // surely no line is > 1024. SURELY
	char blank = ' ';
	int n = 0;
	
	int x = 26, y = 2;
	
	attron (COLOR_PAIR (1));
	
	// make blue background
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x, "%100c", blank);
	}
	
	for (long int i = startl; i <= endl; i++) {
		if (i >= lc) {
			break;
		}
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		// chop trailing ends off
		long int end = MIN (cc, 100);
		long int j, t = 0;
		for (j = 0; j < end; j++) {
			// trim gdb's line num and tab from start
			tmp[t] = blob[offs + j];
			// prevent lines doing their own breaks in my formatting
			if (tmp[t] == '\n') {
				tmp[t] = '\0';
			}
			t++;
		}
		// terminate blank lines
		tmp[j] = '\0';
		char tmp2[101];
		memset (tmp2, 0, 101);
		extract_mi_line (tmp, tmp2);
		//printf ("[%s]", tmp);
		
		bool is_comment = false;
		char a, b;
		int r = sscanf (tmp2, " %c%c", &a, &b);
		if (r == 2 && a == '/' && b == '/') {
			is_comment = true;
		}
		
		if (highlighted_line == i) {
			attroff(COLOR_PAIR (1));
			attron(COLOR_PAIR (5));
			mvprintw (y + n, x, "%-100s", tmp2);
			attroff(COLOR_PAIR (5));
			attron(COLOR_PAIR (1));
		} else if (is_comment) {
			attroff(COLOR_PAIR (1));
			attron(COLOR_PAIR (7));
			mvprintw (y + n, x, "%-100s", tmp2);
			attroff(COLOR_PAIR (7));
			attron(COLOR_PAIR (1));
		} else {
			mvprintw (y + n, x, "%s", tmp2);
		}
		n++;
	}
	
	attroff (COLOR_PAIR(1));
	attron(COLOR_PAIR(6));
	mvprintw (y - 1, x, "%30c", ' ');
	mvprintw (y - 1, x, "%s line (%i/%li)", file_name, highlighted_line + 1,
		lc);
	attroff(COLOR_PAIR(6));
	
	move (CURS_Y, CURS_X);
}

// the line numbers vertical bar
void redraw_line_nos (int startl, int endl, long int lc) {
	int x = 20, y = 2, n = 0;
	
	attron (COLOR_PAIR(1));

	for (int i = startl; i <= endl; i++) {
		if (i > lc) {
			break;
		}
		mvprintw (y + n, x, "%5i", i);
		n++;
	}
	
	attroff (COLOR_PAIR(1));
	move (CURS_Y, CURS_X);
}

// the break-point vertical bar
void redraw_bp_bar (int startl, int endl, long int lc, Line_Meta* lms) {
	int x = 25, y = 2, n = 0;
	
	attron (COLOR_PAIR(2));
	
	for (int i = startl; i <= endl; i++) {
		if (i >= lc) {
			break;
		}
		if (lms[i].bp) {
			attron (COLOR_PAIR(3));
			mvprintw (y + n, x, " ");
			attroff (COLOR_PAIR(3));
			attron (COLOR_PAIR(2));
		} else {
			mvprintw (y + n, x, " ");
		}
		n++;
	}
	
	attroff (COLOR_PAIR(2));
	move (CURS_Y, CURS_X);
	move (CURS_Y, CURS_X);
}

void write_left_side_panel () {
	int x = 0, y = 2;
	char blank = ' ';
	
	attron (COLOR_PAIR(1));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x, "%19c", blank);
	}
	attroff (COLOR_PAIR(1));
	move (CURS_Y, CURS_X);
}

void write_watch_panel (SLL_Node* list_ptr) {
	int x = 126, y = 2;
	char blank = ' ';
	
	attron (COLOR_PAIR(4));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x + 1, "%32c", blank);
	}
	
	SLL_Node* p = list_ptr;
	int n = 0;
	while (p) {
		mvprintw (n + y, x + 1, "%s", (char*)p->data);
		p = p->next;
		n++;
	}
	
	attroff (COLOR_PAIR(4));
	move (CURS_Y, CURS_X);
}

void write_stack_panel (SLL_Node* list_ptr) {
	int x = 159, y = 2;
	char blank = ' ';
	
	attron (COLOR_PAIR(2));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x, "%c", blank);
	}
	attroff (COLOR_PAIR(2));
	
	attron (COLOR_PAIR(4));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x + 1, "%64", blank);
	}
	
	SLL_Node* p = list_ptr;
	int n = 0;
	while (p) {
		mvprintw (n + y, x + 1, "%-64s", (char*)p->data);
		p = p->next;
		n++;
	}
	for (int i = n; i < 49; i++) {
		mvprintw (i + y, x + 1, "%64c", blank);
	}
	
	attroff (COLOR_PAIR(4));
	move (CURS_Y, CURS_X);
}

void write_title_bars () {
	char b = ' ';
	attron (COLOR_PAIR(6));
	mvprintw (0, 2, "EXTERMINATOR");
	mvprintw (1, 127, "Behold!", b);
	mvprintw (1, 160, "and despair!", b);
	attroff (COLOR_PAIR(6));
	attron (COLOR_PAIR(4));
	mvprintw (0, 0, "//");
	mvprintw (0, 14, "\\\\ by Anton Gerdelan @capnramses                        "
		"(B)reakpoint (R)un (spacebar/N)ext (W)atch (G)DB "
		"%112c\n", ' ');
	attroff (COLOR_PAIR(4));
	move (CURS_Y, CURS_X);
}

void write_gdb_op (char* buffer) {
	// clear area
	attron (COLOR_PAIR(2));
	for (int i = 0; i < 25; i++) {
		mvprintw (i + CURS_Y + 1, 0, "%127c", ' ');
	}
	
	char lines[256][128];
	int num_lines = 0;
	if (split_gdb_mi_block (buffer, lines, &num_lines)) {
		// TODO print the rest with (more) feature or sthng?
		
		// find last 13 lines
		int j = 0;
		int st = MAX (0, num_lines - 15);
		//log_msg ("start line = %i\n", st);
		for (long int i = st; i < num_lines - 2; i++) {
			if (i == st && st > 0) {
				mvprintw (j + CURS_Y + 1, 0, "...tail of longer gdb output:");
			} else {
				mvprintw (j + CURS_Y + 1, 0, lines[i]);
			}
			j++;
		}
	}
	
	// x doesn't line up after each line break -- would have to split buffer
	attroff (COLOR_PAIR(2));
	move (CURS_Y, CURS_X);
}

