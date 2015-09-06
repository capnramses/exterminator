//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "ipc.h"
#include "utils.h"
#include "wins.h"
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
	char buffer[4096], blob[4096];
	char input_buff[256];
	input_buff[0] = '\0';
	int input_l = 0;
		
	// close unneeded fds
	close (pipes[1][0]); // child's reading end
	close (pipes[0][1]); // child's writing end

	// wait for child's exec to get going
	sleep (1);
	
	// TODO
	// before normal loop do
	// 1. "ls" to fetch buffer for first text and code lines windows
	
	{ // get the boring intro and don't display it
		buffer[0] = '\0';
		int count = read (pipes[0][0], buffer, sizeof (buffer) - 1);
		if (count > 0) {
			buffer[count] = 0;
			log_msg ("%s", buffer);
		}
	}

	Line_Meta* lms = NULL;
	{ // send "list" for default file
		write (pipes[1][1], "set listsize unlimited\n", 23);
		blob[0] = '\0';
		read (pipes[0][0], blob, sizeof (blob) - 1);
		write (pipes[1][1], "list\n", 5);
		blob[0] = '\0';
		int count = read (pipes[0][0], blob, sizeof (blob) - 1);
		if (count > 0) {
			blob[count] = 0;
			log_msg ("%s", blob);
		}
		long int lc = count_lines_in_blob (blob, count + 1);
		log_msg ("lines in file is %li\n", lc);
		lms = get_line_meta_in_blob (blob, count + 1, lc);
		assert (lms);
		write_blob_lines (0, 49, blob, lc, lms, "???", 0);
		log_msg ("wrote em\n");
		redraw_line_nos (1, 48, lc);
		redraw_bp_bar (0, 49, lc, lms);
		int input_y = 60;
		int input_x = 2;
		move (input_y, input_x);
	}

	long int y = 0, start_ln = 0;
	// HACK TODO: fix later. maybe code goes elsewhere?
	long int lc = 25;
	while (1) {
		buffer[0] = '\0';

		bool collecting = true;
		bool gdb_entry = false;
		while (collecting) {
			int c = getch ();
			
			bool collected = false;
			bool lchange = false;
			
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
				mvprintw (50, 0, "(gdb) \n");
				continue;
			}
			
			if (gdb_entry) {
				// break gdb entry on esc
				if (c == 27) {
					gdb_entry = false;
					collected = false;
					input_l = 0; // reset buffer
					mvprintw (50, 0, "leaving gdb command entry\n");
				} else
				// ignore invalid keys
				if (c > 31 && c < 128) {
					input_buff[input_l] = c;
					input_l++;
					collected = true;
				} else
				// submit buffer on enter key
				if (c == '\n') {
					input_buff[input_l] = '\n';
					input_l++;
					input_buff[input_l] = '\0';
					input_l = 0;
					collecting = false;
					collected = true;
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
					write_blob_lines (0, 49, blob, lc, lms, "???", y);
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
					write_blob_lines (0, 49, blob, lc, lms, "???", y);
					break;
				}
				case KEY_NPAGE: {
					start_ln = MAX (MIN (start_ln + 50, lc - 49), 0);
					lchange = true;
					write_blob_lines (0, 49, blob, lc, lms, "???", y);
					break;
				}
				case KEY_PPAGE: {
					start_ln = MAX (start_ln - 50, 0);
					lchange = true;
					write_blob_lines (0, 49, blob, lc, lms, "???", y);
					break;
				}
				case 32: {
					sprintf (input_buff, "break %li\n", y + 1);
					lchange = true;
					collecting = false;
					break;
				}
				case 's': {
					sprintf (input_buff, "step\n");
					lchange = true;
					collecting = false;
					break;
				}
				case 'n': {
					sprintf (input_buff, "next\n");
					lchange = true;
					collecting = false;
					break;
				}
				default: {
					
				} // default case
			} // switch
			
			if (lchange) {
				int input_y = 60;
				int input_x = 2;
				move (input_y, input_x);
				refresh ();
			}
		} // while collecting input
		
		write (pipes[1][1], input_buff, strlen (input_buff));
		log_msg ("-->[%s]\n", input_buff);
		
		int count = read (pipes[0][0], buffer, sizeof (buffer) - 1);
		if (count > 0) {
			buffer[count] = 0;
			log_msg ("%s", buffer);
			write_gdb_op (buffer);
			
			// TODO parse this to find breakpoints etc, don't assume it worked
			// this also means manual commands will set visual break
			
			// line by line
			const char * curr_line = buffer;
			while (curr_line) {
				char* next_line = strchr (curr_line, '\n');
				if (next_line) {
					*next_line = '\0'; // temporary stopper on the \n
				}
				if (strncmp ("Breakpoint", curr_line, strlen ("Breakpoint")) == 0) {
					int bpn = 0;
					char addr_str[16];
					char file_name[32]; // has comma at end to strip
					long int line = -1;
					int r = sscanf (curr_line, "Breakpoint %i at %s file %s line %li.\n",
						&bpn, addr_str, file_name, &line);
					// set a breakpoint
					if (r == 4) {
						log_msg ("bp %i at %s %s:%li\n", bpn, addr_str, file_name,
							line);
						// TODO NOTE: assuming same file here
						assert (toggle_bp (lms, line - 1, lc));
						redraw_bp_bar (0, 49, lc, lms);
					// running to a breakpoint
					} else {
					
					}
				}
				// unstopper
				if (next_line) {
					*next_line = '\n';
					curr_line = next_line + 1;
				} else {
					curr_line = NULL;
				}
			}
		}
	} // while
	
} // func

