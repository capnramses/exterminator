#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <stdbool.h>
#include <dirent.h> // directory contents POSIX systems
#include <stdlib.h>
#include <string.h>

#define TMP_FILE "src/main.c"

/*
http://invisible-island.net/ncurses/man/
http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
windows
pads (for windows with big scrolling contents)
panels (overlapping windows)
ncursesw "wide" lib for wchars
menus library
*/

typedef struct Code_Line Code_Line;
struct Code_Line {
	size_t offset;
	int cols;
};

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
	
	
	
	
	
	int code_lc = 0;
	int code_max_col = 0;
	size_t code_sz = 0;
	char* code_buff = NULL;
	{ // read entire file into buffer
		FILE* f = fopen (TMP_FILE, "r");
		fseek (f, 0, SEEK_END);
		code_sz = ftell (f);
		rewind (f);
		code_buff = (char*)malloc (code_sz + 1);
		assert (code_buff);
		size_t result = fread (code_buff, 1, code_sz, f);
		assert (result == code_sz);
		code_buff[code_sz] = '\0';
		fclose (f);
	}
	
	{ // count lines in file
		for (size_t i = 0; i < code_sz; i++) {
			if (code_buff[i] == '\n' || code_buff[i] == '\0') {
				code_lc++;
			}
		}
	}
	Code_Line* lines_meta = NULL;
	lines_meta = (Code_Line*)malloc (code_lc * sizeof (Code_Line));
	{ // lines format and cols count
		int col = 0;
		size_t next_offs = 0;
		int l = 0;
		for (size_t i = 0; i < code_sz; i++) {
			col++;
			if (code_buff[i] == '\n' || code_buff[i] == '\0') {
				lines_meta[l].cols = col;
				lines_meta[l].offset = next_offs;
				next_offs = i;
				l++;
				col = 0;
			}
			if (code_buff[i] == '\t') {
				col++;
			}
			if (col > code_max_col) {
				code_max_col = col;
			}
		}//for
	}//block
	//printf ("cols %i rows %i sz %lu\n", code_max_col, code_lc, code_sz);
	
/*	for (int i = 0; i < code_lc; i++) {
		char* tmp = (char*)malloc (lines_meta[i].cols);
		int cols = lines_meta[i].cols;
		strncpy (tmp, &code_buff[lines_meta[i].offset], cols);
		tmp[cols - 1] = '\0';
		printf ("%s", tmp);
		free (tmp);
	}*/
	//printf ("[%s]\n", code_buff);
	
	
	
	
	
	
	
	
	
	
	
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
	
	//printf ("cols: %i rows: %i bytes: %lu\n", code_max_col, code_lc, code_sz);
	WINDOW* code_win = create_win (120, 50, 24, 2, false, 1);
	int end = code_lc;
	if (end > 44) {
		end = 44;
	}
		endwin ();
	for (int i = 0; i < end; i++) {
		char* tmp = (char*)malloc (lines_meta[i].cols + 1);
		int cols = lines_meta[i].cols;
		strncpy (tmp, &code_buff[lines_meta[i].offset], cols);
		tmp[cols] = '\0';
		//printf ("%s", tmp);
		mvwprintw (code_win, i + 1, 1, "%s", tmp);
		free (tmp);
	}
	//box (code_win, 0, 0);
	mvwprintw (code_win, 0, 1, TMP_FILE);
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
		
		/*bool lchange = false;
		switch (c) {
			case KEY_LEFT: {
				x--;
				if (x < 0) {
					x = 0;
				}
				lchange = true;
				break;
			}
			case KEY_RIGHT: {
				if (code_max_col > 120) {
					x++;
					if (x >= code_max_col - 120) {
						x = code_max_col - 120;
					}
					lchange = true;
				}
				break;
			}
			case KEY_UP: {
				y--;
				if (y < 0) {
					y = 0;
				}
				lchange = true;
				break;
			}
			case KEY_DOWN: {
				if (code_lc > 48) {
					y++;
					if (y > code_lc - 48) {
						y = code_lc - 48;
					}
					lchange = true;
				}
				break;
			}
			case KEY_NPAGE: {
				if (code_lc > 98) {
					y += 50;
					if (y > code_lc - 48) {
						y = code_lc - 48;
					}
					lchange = true;
				}
				break;
			}
			case KEY_PPAGE: {
				y -= 50;
				if (y < 0) {
					y = 0;
				}
				lchange = true;
				break;
			}
			default: {}
		}
		
		if (lchange) {
			assert (ERR != prefresh (code_pad, y, x, 3, 25, 47 + 3, 120 + 25));
			werase (linno_win);
			for (int i = y; i < y + 48; i++) {
				wprintw (linno_win, "%3i", i + 1);
			}
			wrefresh (linno_win);
		}*/
		
	}

	// return to normal terminal
	endwin ();
	
	// free mem
	// --------
	if (code_buff) {
		free (code_buff);
		code_buff = NULL;
	}
	if (lines_meta) {
		free (lines_meta);
		lines_meta = NULL;
	}

	return 0;
}

