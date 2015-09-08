//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#pragma once

#include "utils.h"
#include <ncurses.h>

// default cursor/prompt position
#define CURS_X 0
#define CURS_Y 52

void start_ncurses_defaults ();

void nice_exit ();

void draw_defaults ();

// start line is 0 but will be displayed as 1 in side bar
void write_blob_lines (int startl, int endl, char* blob,
	long int lc, Line_Meta* lms, const char* file_name, int highlighted_line);

void redraw_line_nos (int startl, int endl, long int lc);

void redraw_bp_bar (int startl, int endl, long int lc, Line_Meta* lms);

void write_left_side_panel ();

void write_watch_panel (SLL_Node* list_ptr);

void write_stack_panel (SLL_Node* list_ptr);

void write_title_bars ();

void write_gdb_op (char* buffer);

extern SLL_Node* watch_list;
