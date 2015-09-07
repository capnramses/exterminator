//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#pragma once

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

void parse_source_file_name (const char* input, char* output);

void parse_breakpoint (const char* input, char* file_name, int* line);
