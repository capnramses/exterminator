//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#pragma once

#include <stdbool.h>
#include <stdarg.h>

// meta-data about where each line is in the blob (binary large object)
typedef struct Line_Meta Line_Meta;
struct Line_Meta {
	long int cc;
	long int offs;
	bool bp;
};

// log is appended to - this clears it and prints the date
// returns false on error
bool restart_log ();

// print to log in printf() style
// returns false on error
bool log_msg (const char* message, ...);

// same as gl_log except also prints to stderr
// returns false on error
bool log_err (const char* message, ...);

// reads an entire ascii file and returns pointer
// to heap memory where it resides
// or NULL if there was a disaster
char* read_entire_file (const char* file_name, long int* sz);

// count lines in a blob
long int count_lines_in_blob (char* blob, long int sz);

// get pointer to each line and a char count
Line_Meta* get_line_meta_in_blob (char* blob, long int sz, long int lc);

// print lines of a blob to stdout
void print_lines (Line_Meta* lms, char* blob, long int lc);

bool toggle_bp (Line_Meta* lms, long int line, long int lc);

