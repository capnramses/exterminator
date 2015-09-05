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
	char buffer[4096];
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

	{ // send "list" for default file
		write (pipes[1][1], "set listsize unlimited\n", 23);
		buffer[0] = '\0';
		read (pipes[0][0], buffer, sizeof (buffer) - 1);
		write (pipes[1][1], "list\n", 5);
		buffer[0] = '\0';
		int count = read (pipes[0][0], buffer, sizeof (buffer) - 1);
		if (count > 0) {
			buffer[count] = 0;
			log_msg ("%s", buffer);
		}
		long int lc = count_lines_in_blob (buffer, count + 1);
		log_msg ("lines in file is %li\n", lc);
		Line_Meta* lms = get_line_meta_in_blob (buffer, count + 1, lc);
		assert (lms);
		write_blob_lines (0, 49, buffer, lc, lms, "???", 0);
		log_msg ("wrote em\n");
		redraw_line_nos (1, 48, lc);
		redraw_bp_bar (0, 49, lc, lms);
	}

	while (1) {
		buffer[0] = '\0';

		bool collecting = true;
		while (collecting) {
			int c = getch ();
			
			if (27 == c) {
				log_msg ("quit normally by user\n");
				return;
			}
			
			input_buff[input_l] = c;
			input_l++;
			
			if (c == '\n') {
				input_buff[input_l] = '\0';
				input_l = 0;
				collecting = false;
			} // if
		} // while
		write (pipes[1][1], input_buff, strlen (input_buff));
		log_msg ("-->%s\n", input_buff);
		
		int count = read (pipes[0][0], buffer, sizeof (buffer) - 1);
		if (count > 0) {
			buffer[count] = 0;
			log_msg ("%s", buffer);
			write_gdb_op (buffer);
		}
	} // while
	
} // func

