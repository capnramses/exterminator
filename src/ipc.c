//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "ipc.h"
#include "utils.h"
#include "wins.h"
#include "parse.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> //pid_t i guess
#include <string.h>
#include <assert.h>

bool start_ipc (char** argv) {
	log_msg ("starting ipc\n");

	// pipes are unidirectional so we need two, each with 2 ends
	// pipe # 0 is parent's read, 1 is parent's write
	// 0 end is always read, 1 is always write
	int pipes[2][2];
	if (-1 == pipe (pipes[0])) {
		log_err ("ERROR: Failed to create pipe\n");
		return false;
	}
	if (-1 == pipe (pipes[1])) {
		log_err ("ERROR: Failed to create pipe\n");
		return false;
	}
	
	// splits off a new process here
	// pid_t tells us if we are the child or the parent
	pid_t pid = fork ();
	// 0 is the child, pid of child is returned to parent, -1 on failure
	if (pid == -1) {
		log_err ("ERROR: Failed to fork gdb\n");
		return false;
	}
	
	// child
	if (0 == pid) {
		log_msg ("starting child with:[");
		int n = 0;
		char* p = argv[n];
		while (p) {
			if (n > 0) {
				log_msg (" ");
			}
			log_msg ("%s", p);
			n++;
			p = argv[n];
		}
		log_msg ("]\n");
		child_ipc (pipes, argv);
	// parent
	} else {
		parent_ipc (pipes);
	}
	
	return true;
}

void child_ipc (int pipes[][2], char** argv) {
	// connect calculator's in/out streams to our pipes
	// redirect child's read end to stdin
	// child's read is the 'read' end of the parent's write pipe
	dup2 (pipes[1][0], STDIN_FILENO);
	// child's write is the 'write' end of the parent's read pipe
	dup2 (pipes[0][1], STDOUT_FILENO);
	
	// close fds not required by child -- actively hide them from exec'd prog
	close (pipes[0][0]);
	close (pipes[0][1]);
	close (pipes[1][0]);
	close (pipes[1][1]);

	// launch the sucker
	// argv here should not be our prog's whole argv because that will launch us!
	execv (argv[0], argv);
}

void parent_ipc (int pipes[][2]) {
	char file_text_buff[262144];
	char ip_buff[MAX_OP_STR], op_buff[MAX_OP_STR];
	int input_l = 0, op_len = 0;
	file_text_buff[0] = ip_buff[0] = op_buff[0] = '\0';
		
	// close unneeded fds
	close (pipes[1][0]); // child's reading end
	close (pipes[0][1]); // child's writing end

	// get the boring intro and don't display it
	read_child (pipes[0][0], op_buff);

	Line_Meta* lms = NULL;
	long int lc = 0;
	char curr_source_file_name[128];
	memset (curr_source_file_name, 0, 128);
	{ // send "list" for default file
		write_child (pipes[1][1], "set listsize unlimited\n");
		read_child (pipes[0][0], op_buff);
		write_child (pipes[1][1], "list\n");
		op_len = read_child (pipes[0][0], file_text_buff);
		
		lc = count_lines_in_blob (file_text_buff, op_len + 1);
		log_msg ("lines in file is %li\n", lc);
		lms = get_line_meta_in_blob (file_text_buff, op_len + 1, lc);
		assert (lms);
		
		write_child (pipes[1][1], "info source\n");
		read_child (pipes[0][0], op_buff);
		parse_source_file_name (op_buff, curr_source_file_name);
		
		write_blob_lines (0, 48, file_text_buff, lc, lms, curr_source_file_name,
			0);
		redraw_line_nos (1, 49, lc);
		redraw_bp_bar (0, 48, lc, lms);
		attron (COLOR_PAIR(2));
		mvprintw (CURS_Y, 0, "%100c", ' ');
		attroff (COLOR_PAIR(2));
		move (CURS_Y, CURS_X);
		refresh ();
	}

	long int y = 0, start_ln = 0;
	bool gdb_entry = false;
	bool running = false;
	bool just_started_running = false;
	bool stepped = false;
	while (1) {
		bool collecting = true;
		int c = 0;
		while (collecting) {
			if (gdb_entry) {
				attron (COLOR_PAIR(8));
			}
		
			c = getch ();
			
			bool collected = false;
			bool lchange = false;
			stepped = false;
			just_started_running = false;
			
			// quit on esc
			if (!gdb_entry && (c == 27)) {
				log_msg ("quit normally by user\n");
				return;
			}
			
			// switch to typing gdb commands after typing 'g'
			if (!collected && !gdb_entry && ('g' == c)) {
				gdb_entry = true;
				collected = false;
				input_l = 0;
				mvprintw (CURS_Y - 1, 0, "(gdb) mode - press ESC to leave.\n");
				attron (COLOR_PAIR(8));
				echo ();
				continue;
			}
			
			if (gdb_entry) {
				// break gdb entry on esc
				if (c == 27) {
					gdb_entry = false;
					collected = false;
					input_l = 0; // reset buffer
					noecho ();
					attroff (COLOR_PAIR(8));
					mvprintw (CURS_Y - 1, 0, "leaving gdb command entry\n");
				// ignore invalid keys
				} else if (c > 31 && c < 128) {
					ip_buff[input_l] = c;
					input_l++;
					collected = true;
				// backspace
				} else if (c == KEY_BACKSPACE) {
					log_msg ("backspace\n");
					if (input_l > 0) {
						ip_buff[input_l - 1] = 0;
						input_l--;
						log_msg ("backspace ss\n");
					}
				// submit buffer on enter key
				} else if (c == '\n') {
					ip_buff[input_l] = '\n';
					input_l++;
					ip_buff[input_l] = '\0';
					input_l = 0;
					collecting = false;
					collected = true;
					// clear the line
					attroff (COLOR_PAIR(8));
					attron (COLOR_PAIR(2));
					mvprintw (CURS_Y, 0, "%100c", ' ');
					attroff (COLOR_PAIR(2));
					break;
					// TODO perhaps if buffer emtpy don't send or gdb won't reply
				} // if
				continue;
			}
			
			// normal editor commands
			switch (c) {
				case KEY_UP: {
					// move line cursor
					if (y > 0) {
						y--;
						lchange = true;
					// scroll page
					} else {
						if (start_ln > 0) {
							start_ln--;
							lchange = true;
						}
					}
					write_blob_lines (0, 48, file_text_buff, lc, lms,
						curr_source_file_name, y);
					break;
				}
				case KEY_DOWN: {
					// move line cursor
					// not past end of doc even if room on pg
					if (start_ln + y < lc - 1) {
						if (y < 48) {
							y++;
							lchange = true;
						// scroll page
						} else {
							if (start_ln < lc - 1) {
								start_ln++;
								lchange = true;
							}
						}
					}
					write_blob_lines (0, 48, file_text_buff, lc, lms,
						curr_source_file_name, y);
					break;
				}
				case KEY_NPAGE: {
					start_ln = MAX (MIN (start_ln + 50, lc - 49), 0);
					lchange = true;
					write_blob_lines (0, 48, file_text_buff, lc, lms,
						curr_source_file_name, y);
					break;
				}
				case KEY_PPAGE: {
					start_ln = MAX (start_ln - 50, 0);
					lchange = true;
					write_blob_lines (0, 48, file_text_buff, lc, lms,
						curr_source_file_name, y);
					break;
				}
				case 'b': {
					sprintf (ip_buff, "break %li\n", y + 1);
					lchange = true;
					collecting = false;
					break;
				}
				case 'r': {
					sprintf (ip_buff, "run %li\n", y + 1);
					lchange = true;
					collecting = false;
					if (!running) {
						just_started_running = true;
					}
					running = true;
					break;
				}
				case 32: { // spacebar
					sprintf (ip_buff, "next\n");
					lchange = true;
					collecting = false;
					stepped = true;
					break;
				}
				case 'n': {
					sprintf (ip_buff, "next\n");
					lchange = true;
					collecting = false;
					stepped = true;
					break;
				}
				case 's': {
					sprintf (ip_buff, "step\n");
					lchange = true;
					collecting = false;
					stepped = true;
					break;
				}
				default: {
					
				} // default case
			} // switch
			
			if (lchange) {
				move (CURS_Y, CURS_X);
				refresh ();
			}
		} // while collecting input
		
		write_child (pipes[1][1], ip_buff);
		
		op_len = read_child (pipes[0][0], op_buff);
		// TODO format, line-split, scroll
		write_gdb_op (op_buff);
		
		// extra read to (gdb) here
		if (just_started_running || stepped) {
			op_len = read_child (pipes[0][0], op_buff);
			write_gdb_op (op_buff);
			//log_msg ("did the 2nd read...\n");
		}
		
		if (running) {
			if (stepped || just_started_running) {
				int line = 0;
				parse_running_line (op_buff, &line);
				log_msg ("nexted line = %i\n", line);
				
				// update display with new focus
				// TODO change these nums if gone off page
				write_blob_lines (0, 48, file_text_buff, lc, lms,
					curr_source_file_name, line - 1);
				redraw_line_nos (1, 49, lc);
				redraw_bp_bar (0, 48, lc, lms);
				//attron (COLOR_PAIR(2));
				//mvprintw (CURS_Y, 0, "%100c", ' ');
				//attroff (COLOR_PAIR(2));
				
				// check for changes to any on our own watch list
				// "print varname"
				
				
				move (CURS_Y, CURS_X);
				refresh ();
			}
			
			// update the variables on the watch list
			SLL_Node* n = watch_list;
			while (n) {
				char varname[256], valstr[256];
				sscanf (n->data, "%s ", varname);
				sprintf (ip_buff, "print %s\n", varname);
				write_child (pipes[1][1], ip_buff);
				op_len = read_child (pipes[0][0], op_buff);
				memset (valstr, 0, 256);
				if (parse_watched (op_buff, valstr)) {
					sprintf (n->data, "%s %s", varname, valstr);
				}
				n = n->next;
			}
			// NOTE: could check if actually changed first
			write_watch_panel (watch_list);
			move (CURS_Y, CURS_X);
			refresh ();
			
			
		} else if ('b' == c) {
			char fn[256];
			memset (fn, 0, 256);
			int line = 0;
			parse_breakpoint (op_buff, fn, &line);
			// TODO -- toggle to set/unset
			toggle_bp (lms, line - 1, lc);
			redraw_bp_bar (0, 48, lc, lms);
		}
		
		
		// ... TODO
		
	} // while
	
} // func

void write_child (int pipe, const char* input) {
	long int len = strlen (input);
	log_msg ("ip:[%s]\n", input);
	write (pipe, input, len);
}

long int read_child (int pipe, char* output) {
	char tmp[MAX_OP_STR];
	long int len = 0;
	memset (output, 0, MAX_OP_STR);
	memset (tmp, 0, MAX_OP_STR);
	long int count = read (pipe, tmp, MAX_OP_STR - 1);
	bool term_fnd = false;
	while (!term_fnd) {
		strcat (output, tmp);
		len += count;
		if (strstr (output, "(gdb) \n")) {
			term_fnd = true;
		}
		if (!term_fnd) {
			// needed this or read kept all the old non-null chars after \0 !!
			memset (tmp, 0, MAX_OP_STR);
			count = read (pipe, tmp, MAX_OP_STR - 1);
		}
	}
	tmp[len] = '\0';
	if (len > 0) {
		output[len] = 0;
		log_msg ("op:[%s]\n", output);
	}
	return len;
}

