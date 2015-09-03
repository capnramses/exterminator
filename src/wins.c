//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "wins.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// start line is 0 but will be displayed as 1 in side bar
void write_blob_lines (int startl, int endl, char* blob,
	long int lc, Line_Meta* lms, const char* file_name, int highlighted_line) {
	char tmp[100]; // surely no line is > 1024. SURELY
	char blank = ' ';
	int n = 0;
	
	int x = 26, y = 2;
	
	attron (COLOR_PAIR(1));
	
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
		//log_msg ("writing line %i) cc %i offs %i\n", i, cc, offs);
		// chop trailing ends off
		long int end = MIN (cc, 100);
		long int j;
		for (j = 0; j < end; j++) {
			tmp[j] = blob[offs + j];
			// prevent lines doing their own breaks in my formatting
			if (tmp[j] == '\n') {
				tmp[j] = '\0';
			}
		}
		// terminate blank lines
		tmp[j] = '\0';
		//printf ("[%s]", tmp);
		
		if (highlighted_line == i) {
			attroff(COLOR_PAIR(1));
			attron(COLOR_PAIR(5));
			mvprintw (y + n, x, "%-100s", tmp);
			attroff(COLOR_PAIR(5));
			attron(COLOR_PAIR(1));
		} else {
			mvprintw (y + n, x, "%s", tmp);
		}
		n++;
	}
	
	//box (win, 0, 0);
	attroff (COLOR_PAIR(1));
	attron(COLOR_PAIR(6));
	char c = ' ';
	//mvprintw (y - 1, x, "%-100c", c);
	mvprintw (y - 1, x, "%s line (%i/%li)", file_name, highlighted_line + 1, lc);
	attroff(COLOR_PAIR(6));
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
}

void write_left_side_panel () {
	int x = 0, y = 2;
	char blank = ' ';
	
	attron (COLOR_PAIR(1));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x, "%19c", blank);
	}
	attroff (COLOR_PAIR(1));
}

void write_watch_panel (SLL_Node* list_ptr) {
	int x = 126, y = 2;
	char blank = ' ';
	
	attron (COLOR_PAIR(2));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x, "%c", blank);
	}
	attroff (COLOR_PAIR(2));
	
	attron (COLOR_PAIR(4));
	for (int i = 0; i < 49; i++) {
		mvprintw (i + y, x + 1, "%32c", blank);
	}
	
	SLL_Node* p = list_ptr;
	int n = 0;
	while (p) {
		mvprintw (n + y, x + 1, "%-32s", (char*)p->data);
		p = p->next;
		n++;
	}
	
	attroff (COLOR_PAIR(4));
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
}

void write_title_bars () {
	char b = ' ';
	attron (COLOR_PAIR(6));
	mvprintw (0, 0, "//EXTERMINATOR\\\\");
	mvprintw (1, 127, "Behold!", b);
	mvprintw (1, 160, "and despair!", b);
	attroff (COLOR_PAIR(6));
}

