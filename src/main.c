/* TODO
 short files aren't displaying
 rewrite/remove createwin wipewin
*/

#include <ncurses.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <stdbool.h>
#include <dirent.h> // directory contents POSIX systems
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define TMP_FILE "ascii.txt" //"src/main.c"
#define LOG_FILE "log.txt"

/*
http://invisible-island.net/ncurses/man/
http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
windows
pads (for windows with big scrolling contents)
panels (overlapping windows)
ncursesw "wide" lib for wchars
menus library
*/

// log is appended to - this clears it and prints the date
// returns false on error
bool restart_log () {
	FILE* f = fopen (LOG_FILE, "w");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE log file %s for writing\n", LOG_FILE);
		return false;
	}
	time_t now = time (NULL);
	char* date = ctime (&now);
	fprintf (f, "LOG_FILE log. local time %s\n", date);
	fprintf (f, "build version: %s %s\n", __DATE__, __TIME__);
	fclose (f);
	return true;
}

// print to log in printf() style
// returns false on error
bool log_msg (const char* message, ...) {
	va_list argptr;
	FILE* f = fopen (LOG_FILE, "a");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE %s file for appending\n", LOG_FILE);
		return false;
	}
	va_start (argptr, message);
	vfprintf (f, message, argptr);
	va_end (argptr);
	fclose (f);
	return true;
}

// same as gl_log except also prints to stderr
// returns false on error
bool log_err (const char* message, ...) {
	va_list argptr;
	FILE* f = fopen (LOG_FILE, "a");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE %s file for appending\n", LOG_FILE);
		return false;
	}
	va_start (argptr, message);
	vfprintf (f, message, argptr);
	va_end (argptr);
	va_start (argptr, message);
	vfprintf (stderr, message, argptr);
	va_end (argptr);
	fclose (f);
	return true;
}

//
// meta-data about where each line is in the blob (binary large object)
typedef struct Line_Meta Line_Meta;
struct Line_Meta {
	long int cc;
	long int offs;
};

//
// reads an entire ascii file and returns pointer
// to heap memory where it resides
// or NULL if there was a disaster
char* read_entire_file (const char* file_name, long int* sz) {
	FILE* f = fopen (file_name, "r");
	assert (f);
	
	// get file size
	int r = fseek (f, 0, SEEK_END);
	assert (r == 0);
	*sz = ftell (f);
	rewind (f);
	printf ("size is %li\n", *sz);
	
	// mallocate
	char* p = (char*)malloc (*sz);
	assert (p);
	
	size_t rr = fread (p, *sz, 1, f);
	assert (rr == 1);
	
	fclose (f);
	return p;
}

long int count_lines_in_blob (char* blob, long int sz) {
	long int lc = 0;
	for (long int i = 0; i < sz; i++) {
		if (blob[i] == '\n') {
			lc++;
		}
	}
	// add first line too!
	if (sz > 0) {
		lc++;
	}
	
	return lc;
}

// get pointer to each line
// and a char count
Line_Meta* get_line_meta_in_blob (char* blob, long int sz, long int lc) {
	Line_Meta* lms = (Line_Meta*)malloc (lc * sizeof (Line_Meta));
	// zero this as last line might be empty and won't be set to anything in this
	// loop
	memset (lms, 0, lc * sizeof (Line_Meta));
	
	int l = 0;
	int c = 0;
	int o = 0;
	for (long int i = 0; i < sz; i++) {
		c++;
		if (blob[i] == '\n') {
			lms[l].cc = c;
			lms[l].offs = o;
			// next line starts on the next char
			o = i + 1;
			c = 0;
			l++;
		}
	}
	
	return lms;
}

void print_lines (Line_Meta* lms, char* blob, long int lc) {
	char tmp[1024]; // surely no line is > 1024. SURELY
	for (long int i = 0; i < lc; i++) {
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		assert (cc < 1023);
		long int j;
		for (j = 0; j < cc; j++) {
			tmp[j] = blob[offs + j];
		}
		tmp[j] = '\0';
		printf ("[%s]", tmp);
	}
}

// start line is 0 but will be displayed as 1 in side bar
void write_blob_lines (WINDOW* win, int startl, int endl, char* blob,
	long int lc, Line_Meta* lms) {
	char tmp[1024]; // surely no line is > 1024. SURELY
	int n = 0;
	
	assert (endl < lc);
	
	werase (win);
	
	for (long int i = startl; i <= endl; i++) {
		if (endl >= lc) {
			break;
		}
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		log_msg ("writing line %i) cc %i offs %i\n", i, cc, offs);
		if (cc >= 1023) {
			fprintf (stderr, "ERR cc = %li on line %li\n", cc, i);
			exit (1);
		}
		long int j;
		for (j = 0; j < cc; j++) {
			tmp[j] = blob[offs + j];
		}
		tmp[j] = '\0';
		//printf ("[%s]", tmp);
		mvwprintw (win, n + 1, 1, "%s", tmp);
		n++;
	}
}

void redraw_line_nos (WINDOW* win, int startl, int endl) {
	werase (win);
	for (int i = startl; i <= endl; i++) {
		wprintw (win, "%3i", i);
	}
	wrefresh (win);
}

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

	if (!restart_log ()) {
		return 1;
	}
	
//--------
	log_msg ("reading file %s\n", TMP_FILE);
	long int sz = 0;
	char* blob = read_entire_file (TMP_FILE, &sz);
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
//	noecho ();
	// enable F1 etc.
	keypad (stdscr, TRUE);
	init_pair (1, COLOR_WHITE, COLOR_BLUE);
	init_pair (2, COLOR_WHITE, COLOR_BLACK);
	init_pair (3, COLOR_RED, COLOR_WHITE);
	init_pair (4, COLOR_GREEN, COLOR_BLACK);
	
	// this need to be here before windows or it wont work
	refresh ();
	
	// windows
	// -------
	WINDOW* title_win = create_win (184, 1, 0, 0, false, 4);
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
	WINDOW* linno_win = create_win (3, 48, 21, 3, false, 2);
	redraw_line_nos (linno_win, 1, 48);
	
	WINDOW* code_win = create_win (120, 50, 24, 2, false, 1);
	write_blob_lines (code_win, 0, 47, blob, lc, lms);
	box (code_win, 0, 0);
	wrefresh (code_win);

	WINDOW* watch_win = create_win (40, 30, 144, 2, true, 1);
	WINDOW* st_win = create_win (40, 30, 144, 32, true, 1);
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
	
	long int x = 0, y = 0;
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
		switch (c) {
			case KEY_UP: {
				if (y > 0) {
					y--;
					lchange = true;
				}
				break;
			}
			case KEY_DOWN: {
				if (y < lc - 48) {
					y++;
					lchange = true;
				}
				break;
			}
			case KEY_NPAGE: {
				if (y + 50 < lc - 48) {
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
			default: {}
		}
		
		if (lchange) {
			write_blob_lines (code_win, y, 47 + y, blob, lc, lms);
			box (code_win, 0, 0);
			wrefresh (code_win);
			redraw_line_nos (linno_win, y + 1, 48 + y);
		}
		
	}

	// return to normal terminal
	endwin ();
	
	// free mem
	// --------


	return 0;
}

