//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#pragma once

#include <stdbool.h>

#define MAX_OP_STR 8192

// types of gdb m/i output
enum mi_type {
	GDB_UNKNOWN,
	GDB_SYS,
	GDB_COMMENT,
	GDB_PROMPT,
	GDB_ECHO,
	GDB_RESULT
};
typedef enum mi_type mi_type;

mi_type get_mi_type (char first);

// lose the formatting and give me the line back with type info
void extract_mi_line (const char* input, char output[]);

bool parse_source_file_name (const char* input, char* output);

bool parse_breakpoint (const char* input, char* file_name, int* line);

// line that debugger is focussed on
bool parse_running_line (const char* input, int* line, char* file_name);

bool parse_watched (const char* input, char* val_str);

long int count_lines_in_string (const char* input);

bool split_gdb_mi_block (const char* input, char lines[][128], int* num_lines);

bool split_st_mi_block (const char* input, char lines[][100], int* num_lines);

