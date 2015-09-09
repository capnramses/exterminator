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

#define BIN "/usr/bin/gdb"

bool start_ipc (int argc, char** argv) {
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
		child_ipc (pipes, argc, argv);
	// parent
	} else {
		parent_ipc (pipes, argc, argv);
	}
	
	return true;
}

void child_ipc (int pipes[][2], int argc, char** argv) {
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

	char target[1024];
	memset (target, 0, 1024);
	if (argc > 1) {
		strcpy (target, argv[1]);
	}
	// launch the sucker
	// argv here should not be our prog's whole argv because that will launch us!
	char* args[] = {BIN, target, "--interp=mi", NULL};
	execv (args[0], args);
}

void parent_ipc (int pipes[][2], int argc, char** argv) {
	char file_text_buff[262144];
	char ip_buff[MAX_OP_STR], op_buff[MAX_OP_STR];
	char bt_lines[49][100];
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
		write_child (pipes[1][1], "list 1\n"); // 1 means start at top
		op_len = read_child (pipes[0][0], file_text_buff);
		
		lc = count_lines_in_blob (file_text_buff, op_len + 1);
		log_msg ("lines in file is %li\n", lc);
		lms = get_line_meta_in_blob (file_text_buff, op_len + 1, lc);
		assert (lms);
		
		write_child (pipes[1][1], "info source\n");
		read_child (pipes[0][0], op_buff);
		log_msg ("parsing src file [%s]\n", curr_source_file_name);
		assert (parse_source_file_name (op_buff, curr_source_file_name));
		
		write_blob_lines (0, 48, file_text_buff, lc, lms, curr_source_file_name,
			0);
		redraw_line_nos (1, 49, lc);
		redraw_bp_bar (0, 48, lc, lms);
		attron (COLOR_PAIR(2));
		mvprintw (CURS_Y, 0, "%100c", ' ');
		attroff (COLOR_PAIR(2));
		move (CURS_Y, CURS_X);
		//refresh ();
	}

	long int y = 0;
	bool gdb_entry = false;
	bool watch_entry = false;
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
			if (!gdb_entry && !watch_entry && (c == 27)) {
				log_msg ("quit normally by user\n");
				return;
			}
			
			// switch to typing gdb commands after typing 'g'
			if (!collected && !gdb_entry && !watch_entry && ('g' == c)) {
				gdb_entry = true;
				collected = false;
				input_l = 0;
				mvprintw (CURS_Y - 1, 0, "%100c", ' ');
				mvprintw (CURS_Y - 1, 0, "(gdb) mode - press ESC to leave.");
				attron (COLOR_PAIR(8));
				move (CURS_Y, CURS_X);
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
					mvprintw (CURS_Y - 1, 0, "%100c", ' ');
					mvprintw (CURS_Y - 1, 0, "leaving gdb command entry");
					mvprintw (CURS_Y, 0, "%100c", ' ');
					move (CURS_Y, CURS_X);
				// ignore invalid keys
				} else if (c > 31 && c < 128) {
					ip_buff[input_l] = c;
					input_l++;
					collected = true;
				// backspace
				} else if (c == KEY_BACKSPACE) {
					if (input_l > 0) {
						ip_buff[input_l - 1] = 0;
						input_l--;
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
			
			// switch to typing gdb commands after typing 'g'
			if (!watch_entry && ('w' == c)) {
				watch_entry = true;
				input_l = 0;
				input_l = 0;
				mvprintw (CURS_Y - 1, 0, "%100c", ' ');
				mvprintw (CURS_Y - 1, 0, "Watch: enter variable name or ESC to abort:");
				echo ();
				mvprintw (CURS_Y, 0, "%100c", ' ');
				move (CURS_Y, CURS_X);
				continue;
			}
			
			if (watch_entry) {
				// break watch entry on esc
				if (c == 27) {
					watch_entry = false;
					input_l = 0; // reset buffer
					noecho ();
					mvprintw (CURS_Y - 1, 0, "%100c", ' ');
					mvprintw (CURS_Y - 1, 0, "leaving watch command entry");
					mvprintw (CURS_Y, 0, "%100c", ' ');
					move (CURS_Y, CURS_X);
				} else if (c > 31 && c < 128) {
					ip_buff[input_l] = c;
					input_l++;
				} else if (c == KEY_BACKSPACE) {
					if (input_l > 0) {
						ip_buff[input_l - 1] = 0;
						input_l--;
					}
				// submit buffer on enter key
				} else if (c == '\n') {
					mvprintw (CURS_Y - 1, 0, "%100c", ' ');
					move (CURS_Y, CURS_X);
					ip_buff[input_l] = '\0';
					input_l++;
					log_msg ("var is [%s] len %i\n", ip_buff, input_l);
					//ip_buff[input_l] = '\0';
					// clear the line
					mvprintw (CURS_Y, 0, "%100c", ' ');
					// perhaps if buffer emtpy don't send or gdb won't reply
					if (input_l < 1) {
						mvprintw (CURS_Y - 1, 0, "leaving watch command entry");
						mvprintw (CURS_Y, 0, "%100c", ' ');
						move (CURS_Y, CURS_X);
					// add to watch list
					} else {
						char tmp[1024], valstr[256];
						sprintf (tmp, "print %s\n", ip_buff);
						write_child (pipes[1][1], tmp);
						op_len = read_child (pipes[0][0], op_buff);
						memset (valstr, 0, 256);
						if (parse_watched (op_buff, valstr)) {
							add_to_watch (ip_buff, valstr, &watch_list);
						} else {
							add_to_watch (ip_buff, "???", &watch_list);
						}
						//op_len = read_child (pipes[0][0], op_buff);
						write_watch_panel (watch_list);
					}
					watch_entry = false;
					noecho ();
					input_l = 0;
				} // if
				continue;
			}
			
			if (running) {
				if (c == 32 || c == 'n') { // spacebar
					sprintf (ip_buff, "next\n");
					lchange = true;
					collecting = false;
					stepped = true;
				// next-in
				} else if (c == 's') {
					sprintf (ip_buff, "step\n");
					lchange = true;
					collecting = false;
					stepped = true;
				}
			} else {
				if (c == 'r') {
					sprintf (ip_buff, "run");
					for (int i = 2; i < argc; i++) {
						strcat (ip_buff, " ");
						strcat (ip_buff, argv[i]);
					}
					strcat (ip_buff, "\n");
					log_msg ("running with [%s]\n", ip_buff);
					lchange = true;
					collecting = false;
					if (!running) {
						just_started_running = true;
					}
					running = true;
				}
			}
			
			// normal editor commands
			// TODO: anton -- this is a bit of a sprawling mess. consolidate later
			// when better structure becomes clear
			switch (c) {
				case KEY_UP: {
					// move line cursor
					y = MAX (0, y - 1);
					lchange = true;
					break;
				}
				case KEY_DOWN: {
					// move line cursor
					// not past end of doc even if room on pg
					y = MIN (lc - 1, y + 1);
					lchange = true;
					break;
				}
				case KEY_NPAGE: {
					y = MIN (lc - 1, y + 50);
					lchange = true;
					break;
				}
				case KEY_PPAGE: {
					y = MAX (0, y - 50);
					lchange = true;
					break;
				}
				case 'b': {
					sprintf (ip_buff, "break %li\n", y + 1);
					lchange = true;
					collecting = false;
					break;
				}
				/* TODO -- crashes for some reason?? case 'c': {
					sprintf (ip_buff, "continue\n");
					lchange = true;
					collecting = false;
					if (!running) {
						just_started_running = true;
					}
					stepped = true;
					running = true;
					break;
				}*/
				default: {
				} // default case
			} // switch
			
			if (lchange) {
				long int min = MAX (0, y - 24);
				long int max = MIN (lc, min + 48);
				write_blob_lines (min, max, file_text_buff, lc, lms,
					curr_source_file_name, y);
				redraw_line_nos (min + 1, max + 1, lc);
				redraw_bp_bar (min, max, lc, lms);
				move (CURS_Y, CURS_X);
				move (CURS_Y, CURS_X);
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
				char found_file_name[1024];
				found_file_name[0] = 0;
				// assume run finished if returned false
				if (!parse_running_line (op_buff, &line, found_file_name)) {
					just_started_running = false;
					running = false;
					stepped = false;
					continue;
				}
				
				// check if focussed file has changed
				if (strcmp (curr_source_file_name, found_file_name) != 0) {
					log_msg ("switching file...\n");
					strcpy (curr_source_file_name, found_file_name);
					
					// TODO put this repeated block in a function
					{
						write_child (pipes[1][1], "set listsize unlimited\n");
						read_child (pipes[0][0], op_buff);
						write_child (pipes[1][1], "list 1\n"); // 1 means start at top
						op_len = read_child (pipes[0][0], file_text_buff);
	
						lc = count_lines_in_blob (file_text_buff, op_len + 1);
						log_msg ("lines in file is %li\n", lc);
						
						if (lms) {
							free (lms);
							lms = NULL;
						}
						
						lms = get_line_meta_in_blob (file_text_buff, op_len + 1, lc);
						assert (lms);
					}
				}
				
				// update cursor location
				y = line - 1;
				log_msg ("nexted line = %i\n", line);
				
				// update display with new focus
				// TODO change these nums if gone off page
				long int min = MAX (0, line - 24);
				long int max = MIN (lc, min + 48);
				write_blob_lines (min, max, file_text_buff, lc, lms,
					curr_source_file_name, line - 1);
				redraw_line_nos (min + 1, max + 1, lc);
				redraw_bp_bar (min, max, lc, lms);

				// get stack trace
				write_child (pipes[1][1], "bt\n");
				op_len = read_child (pipes[0][0], op_buff);
				int nbtl = 0;
				split_st_mi_block (op_buff, bt_lines, &nbtl);
				write_stack_panel (bt_lines, nbtl);
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
			
		} else if ('b' == c) {
			char fn[256];
			memset (fn, 0, 256);
			int line = 0;
			if (parse_breakpoint (op_buff, fn, &line)) {
				// TODO -- toggle to set/unset
				toggle_bp (lms, line - 1, lc);
				long int min = MAX (0, y - 24);
				long int max = MIN (lc, min + 48);
				redraw_bp_bar (min, max, lc, lms);
			}
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

