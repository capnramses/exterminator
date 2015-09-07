//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "parse.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* TO SUPPORT
* info sources or just dir list?
~ info functions
*/

// get type of gdb m/i line
mi_type get_mi_type (char first) {
	switch (first) {
		// gdb system
		// =thread-group-added,id="i1"
		// =breakpoint-created,....
		case '=': {
			return GDB_SYS;
		}
		// comment
		// ~"GNU gdb (Ubuntu 7.7.1-0ubuntu5~14.04.2) 7.7.1\n"
		case '~': {
			return GDB_COMMENT;
		}
		// prompt
		// (gdb)
		case '(': {
			return GDB_PROMPT;
		}
		// echoed feedback
		// &"set listsize unlimited\n"
		case '&': {
			return GDB_ECHO;
		}
		// report op result
		// ^done
		// ^error,msg="Undefined command: \"blg\".  Try \"help\"."
		case '^': {
			return GDB_RESULT;
		}
		default: {
		}
	} // endswitch
	return GDB_UNKNOWN;
}

// lose the formatting and give me the line back with type info
void extract_mi_line (const char* input, char output[]) {
	size_t ip_len = strlen (input);
	if (ip_len < 1) {
		return;
	}
	// get length of final line
	size_t op_len = 0;
	// skip key char and opening and closing quotes
	for (size_t i = 2; i < ip_len - 1; i++) {
		if (input[i] == '\\') {
			switch (input[i + 1]) {
				// just print one slash
				case '\\': {
					output[op_len] = '\\';
					op_len++;
					i++;
					break;
				}
				// length 1
				case 'n': {
					output[op_len] = '\n';
					op_len++;
					i++;
					break;
				}
				// length 2 tabs-to-spaces
				case 't': {
					output[op_len] = ' ';
					op_len++;
					output[op_len] = ' ';
					op_len++;
					i++;
					break;
				}
				// " are length 1
				case '\"': {
					output[op_len] = '\"';
					op_len++;
					i++;
					break;
				}
				default: {
					log_err ("WARNING: escape char \\%c unhandled\n", input[i + 1]);
					i++;
				}
			} // endswitch
		} else {
			output[op_len] = input[i];
			op_len++;
		} // endif
	} // endfor
	
	if (op_len + 1 >= MAX_OP_STR) {
		log_err ("ERROR: gdb output string is longer than max expected "
			"(%i/%i)\n", (int)op_len, MAX_OP_STR);
		exit (1);
	}
}

void parse_source_file_name (const char* input, char* output) {
	//~"Current source file is genascii.c\n"
	
	long int len = strlen (input);
	long int line_start = 0;
	for (long int i = 0; i < len; i++) {
		if (input[i] == '\n') {
			char tmp[128];
			int k = 0;
			for (long int j = line_start; j <= i; j++) {
				tmp[k] = input[j];
				k++;
			}
			tmp[k] = 0;
			if (strstr (tmp, "Current source file")) {
				sscanf (tmp, "~\"Current source file is %s", output);
				long int llen = strlen (output);
				output[llen - 1] = output[llen - 2] = output[llen - 3] = 0;
				return;
			}
			
			line_start = i + 1;
		}
	}
	log_err ("ERROR: did not find source name in op\n");
	exit (1);
}

void parse_breakpoint (const char* input, char* file_name, int* line) {
	// ~"Breakpoint 1 at 0x40077c: file genascii.c, line 15.\n"
	// =breakpoint-created,bkpt={number="1",type="breakpoint",disp="keep",enabled="y",addr="0x000000000040077c",func="main",file="genascii.c",fullname="/home/anton/projects/exterminator/genascii.c",line="15",thread-groups=["i1"],times="0",original-location="/home/anton/projects/exterminator/genascii.c:15"}
	long int len = strlen (input);
	long int line_start = 0;
	for (long int i = 0; i < len; i++) {
		if (input[i] == '\n') {
			char tmp[128];
			int k = 0;
			for (long int j = line_start; j <= i; j++) {
				tmp[k] = input[j];
				k++;
			}
			tmp[k] = 0;
			if (strstr (tmp, "~\"Breakpoint ")) {
				int bnum = 0;
				char addr_str[32];
				int r = sscanf (tmp, "~\"Breakpoint %i at %s file %s line %i.\\n\"",
					&bnum, addr_str, file_name, line);
				int llen = strlen (file_name);
				file_name[llen - 1] = 0;
				log_err (tmp);
				assert (r == 4);
				return;
			}
			line_start = i + 1;
		}
	}
	log_err ("ERROR: did not find source name in op\n");
	exit (1);
}

