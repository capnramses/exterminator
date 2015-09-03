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
	char tmp[1024]; // surely no line is > 1024. SURELY
	int n = 0;
	
	int x = 6, y = 2;
	
	attron (COLOR_PAIR(1));
	
	long int range = (endl - startl) + 1;
	for (long int i = 49 - range; i < 49; i++) {
		mvprintw (i + y, x, "%-100s", "\0");
	}
	
	for (long int i = startl; i <= endl; i++) {
		if (i >= lc) {
			break;
		}
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		//log_msg ("writing line %i) cc %i offs %i\n", i, cc, offs);
		if (cc >= 1023) {
			fprintf (stderr, "ERR cc = %li on line %li\n", cc, i);
			exit (1);
		}
		long int j;
		for (j = 0; j < cc; j++) {
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
			mvprintw (y + n, x, "%-100s", tmp);
		}
		n++;
	}
	
	//box (win, 0, 0);
	attroff (COLOR_PAIR(1));
	attron(COLOR_PAIR(6));
	mvprintw (y - 1, x, "%s line (%i/%li)", file_name, highlighted_line + 1, lc);
	attroff(COLOR_PAIR(6));
}

void redraw_line_nos (int startl, int endl, long int lc) {
	int x = 0, y = 2, n = 0;
	
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

void redraw_bp_bar (int startl, int endl, long int lc, Line_Meta* lms) {
	int x = 5, y = 2, n = 0;
	
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

