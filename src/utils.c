//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "utils.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define LOG_FILE "log.txt"

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

// count lines in a blob
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

// get pointer to each line and a char count
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

// print lines of a blob to stdout
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

bool toggle_bp (Line_Meta* lms, long int line, long int lc) {
	if (line < 0 || line >= lc) {
		return false;
	}
	lms[line].bp = !lms[line].bp;
	return true;
}
