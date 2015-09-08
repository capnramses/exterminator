//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#include "utils.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#define LOG_FILE "exterminator.log"

// log is appended to - this clears it and prints the date
// returns false on error
bool restart_log () {
	FILE* f = fopen (LOG_FILE, "w");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE log file %s for writing\n", LOG_FILE);
		return false;
	}
	time_t now = time (NULL);
	char* date = ctime (&now);
	fprintf (f, "LOG_FILE log. local time %s\n", date);
	fprintf (f, "build version: %s %s\n", __DATE__, __TIME__);
	fclose (f);
	return true;
}

// print to log in printf() style
// returns false on error
bool log_msg (const char* message, ...) {
	va_list argptr;
	FILE* f = fopen (LOG_FILE, "a");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE %s file for appending\n", LOG_FILE);
		return false;
	}
	va_start (argptr, message);
	vfprintf (f, message, argptr);
	va_end (argptr);
	fclose (f);
	return true;
}

// same as gl_log except also prints to stderr
// returns false on error
bool log_err (const char* message, ...) {
	va_list argptr;
	FILE* f = fopen (LOG_FILE, "a");
	if (!f) {
		fprintf (stderr,
			"ERROR: could not open LOG_FILE %s file for appending\n", LOG_FILE);
		return false;
	}
	va_start (argptr, message);
	vfprintf (f, message, argptr);
	va_end (argptr);
	va_start (argptr, message);
	vfprintf (stderr, message, argptr);
	va_end (argptr);
	fclose (f);
	return true;
}

// reads an entire ascii file and returns pointer
// to heap memory where it resides
// or NULL if there was a disaster
char* read_entire_file (const char* file_name, long int* sz) {
	FILE* f = fopen (file_name, "r");
	assert (f);
	
	// get file size
	int r = fseek (f, 0, SEEK_END);
	assert (r == 0);
	*sz = ftell (f);
	rewind (f);
	printf ("size is %li\n", *sz);
	
	// mallocate
	char* p = (char*)malloc (*sz);
	assert (p);
	
	size_t rr = fread (p, *sz, 1, f);
	assert (rr == 1);
	
	fclose (f);
	return p;
}

// count lines in a blob
long int count_lines_in_blob (char* blob, long int sz) {
	long int lc = 0;
	bool start_of_line = true;
	for (long int i = 0; i < sz; i++) {
		if (start_of_line) {
			if (blob[i] == '~') {
				lc++;
			}
			start_of_line = false;
		}
		if (blob[i] == '\n') {
			start_of_line = true;
		}
	}
	
	return lc;
}

// get pointer to each line and a char count
Line_Meta* get_line_meta_in_blob (char* blob, long int sz, long int lc) {
	Line_Meta* lms = (Line_Meta*)malloc (lc * sizeof (Line_Meta));
	// zero this as last line might be empty and won't be set to anything in this
	// loop
	memset (lms, 0, lc * sizeof (Line_Meta));
	
	int l = 0;
	int c = 0;
	int o = 0;
	int num_trim = 0;
	bool start_of_line = true;
	bool code_line = false;
	bool found_num_end = false;
	for (long int i = 0; i < sz; i++) {
		// skip non-code lines
		if (start_of_line) {
			c = 0;
			if (blob[i] == '~') {
				code_line = true;
				found_num_end = false;
				num_trim = 0;
				o = i;
			} else {
				code_line = false;
			}
			start_of_line = false;
		}
		// TODO replace tabs with spaces
		
		// TODO escape chars

		if (code_line && !found_num_end) {
			if (blob[i] == '\\') {
				found_num_end = true;
			} else {
				num_trim++;
			}
		}
		
		c++;
		if (blob[i] == '\n') {
			if (code_line) {
				lms[l].cc = c - 3 - num_trim; // trim \ and n, two quotes, and line start char
				lms[l].offs = o + num_trim;
				l++;
			}
			// next line starts on the next char
			start_of_line = true;
		}
	}
	
	return lms;
}

// print lines of a blob to stdout
void print_lines (Line_Meta* lms, char* blob, long int lc) {
	char tmp[1024]; // surely no line is > 1024. SURELY
	for (long int i = 0; i < lc; i++) {
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		assert (cc < 1023);
		bool trimmed = false;
		long int j, t = 0;
		for (j = 0; j < cc; j++) {
			// trim gdb's line num and tab from start
			if (!trimmed) {
				if (blob[offs + j] == '\t') {
					trimmed = true;
				}
				continue;
			}
			tmp[t] = blob[offs + j];
			t++;
		}
		tmp[t] = '\0';
		printf ("[%s]", tmp);
	}
}

bool toggle_bp (Line_Meta* lms, long int line, long int lc) {
	if (line < 0 || line >= lc) {
		return false;
	}
	lms[line].bp = !lms[line].bp;
	return true;
}

//
// add a node to the front of a singly-linked list
// list_ptr is the pointer to the front of the list or null
// data is the contents to hold in the node
// sz is the size of that data in bytes
// returns ptr to new node or NULL on error
// note: data pointer is not freed by this function
//
SLL_Node* sll_add_to_front (SLL_Node** list_ptr, const void* data,
	size_t sz) {
	SLL_Node* node = (SLL_Node*)malloc (sizeof (SLL_Node));
	assert (node);
	if (!node) {
		fprintf (stderr, "ERROR: could not alloc memory for ssl node struct\n");
		return NULL;
	}
	node->next = *list_ptr;
	node->data = malloc (sz);
	node->sz = sz;
	node->data = memcpy (node->data, data, sz);
	*list_ptr = node;
	return node;
}

//
// add a node after another node of a singly-linked list
// prev_ptr is the pointer to the node to go after
// data is the contents to hold in the node
// sz is the size of that data in bytes
// returns ptr to new node or NULL on error
//
SLL_Node* sll_insert_after (SLL_Node* prev_ptr, const void* data,
	size_t sz) {
	// this is far more likely to be a user mistake than anything - should warn
	if (!prev_ptr) {
		fprintf (stderr, "ERROR: could not insert sll node, prev_ptr was NULL\n");
		return NULL;
	}
	SLL_Node* node = (SLL_Node*)malloc (sizeof (SLL_Node));
	if (!node) {
		fprintf (stderr, "ERROR: could not alloc memory for sll node struct\n");
		return NULL;
	}
	node->next = prev_ptr->next;
	node->data = malloc (sz);
	node->sz = sz;
	node->data = memcpy (node->data, data, sz);
	prev_ptr->next = node;
	return node;
}

//
// delete node pointed to by ptr
// function searches list from list_ptr to ptr to find prev node
// prev->next will then point to ptr->next
// ptr is freed and then set to NULL
// list_ptr is a pointer to the start of the list (or any node prior to ptr)
// ptr is the node to delete
// returns false on error
//
bool sll_delete_node (SLL_Node** list_ptr, SLL_Node* ptr) {
	if (!*list_ptr) {
		fprintf (stderr, "ERROR: can not delete sll node, list_ptr is NULL\n");
		return false;
	}
	if (!ptr) {
		fprintf (stderr, "ERROR: can not delete sll node, ptr is NULL\n");
		return false;
	}
	// find prev node to ptr so can adjust
	SLL_Node* p = *list_ptr;
	while (p) {
		// p is first node in list, so adjust list ptr
		if (p == ptr) {
			*list_ptr = ptr->next;
			break;
		}
		// make prev->next equal to ptr->next
		if (p->next == ptr) {
			p->next = ptr->next;
			break;
		}
		p = p->next;
	} // endwhile
	
	assert (ptr);
	assert (ptr->data);
	free (ptr->data);
	ptr->data = NULL; // pointless, not used again
	free (ptr);
	ptr = NULL; // pointless, not used again
	
	return true;
}

//
// list_ptr is any node in the list - the farther along, the shorter the
// search
// returns NULL if list is empty
//
SLL_Node* sll_find_end_node (SLL_Node* list_ptr) {
	if (!list_ptr) {
		return NULL;
	}
	SLL_Node* p = list_ptr;
	while (p) {
		if (!p->next) {
			break;
		}
		p = p->next;
	}
	return p;
}

//
// recursively deletes and entire list, starting from ptr
// sets ptr to NULL afterwards
// returns false on error
// note: figured there was no point inlining this
//
bool sll_recursive_delete (SLL_Node** ptr) {
	if (!*ptr) {
		fprintf (stderr, "ERROR: could not recursive delete sll, node was NULL\n");
		return false;
	}
	if ((*ptr)->next) {
		sll_recursive_delete (&(*ptr)->next);
	}
	
	assert (*ptr);
	assert ((*ptr)->data);
	free ((*ptr)->data);
	(*ptr)->data = NULL; // pointless, not used again
	free (*ptr);
	*ptr = NULL; // pointless, not used again
	return true;
}

bool add_to_watch (const char* var_name, const char* value_str,
	SLL_Node** list_ptr) {
	char a[512];
	char b[512];
	strncpy (a, var_name, 511);
	strncpy (b, value_str, 511);
	char tmp[2048];
	SLL_Node* node_ptr = NULL;
	sprintf (tmp, "%s %s", a, b);
	//log_msg ("a:[%s] b:[%s] tmp:[%s]\n", a, b, tmp);
	if (*list_ptr) {
		SLL_Node* ep = sll_find_end_node (*list_ptr);
		node_ptr = sll_insert_after (ep, tmp, strlen (tmp) + 1);
	} else {
		node_ptr = sll_add_to_front (list_ptr, tmp, strlen (tmp) + 1);
	}
	assert (node_ptr);
	return true;
}

bool add_to_stack (const char* str, SLL_Node** list_ptr) {
	char tmp[64];
	strncpy (tmp, str, 64);
	SLL_Node* node_ptr = sll_add_to_front (list_ptr, tmp, 64);
	assert (node_ptr);
	return true;
}

