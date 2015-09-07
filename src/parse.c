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

