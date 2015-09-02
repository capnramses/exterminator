//
// making sure i get file reading right on small stuff before i make a big mess
// this system is based on the assumption that reading the file once,
// and allocating all the memory once is the fastest solution
//

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

//
// meta-data about where each line is in the blob (binary large object)
typedef struct Line_Meta Line_Meta;
struct Line_Meta {
	long int cc;
	long int offs;
};

//
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

long int count_lines_in_blob (char* blob, long int sz) {
	long int lc = 0;
	for (long int i = 0; i < sz; i++) {
		if (blob[i] == '\n') {
			lc++;
		}
	}
	// add first line too!
	if (sz > 0) {
		lc++;
	}	
	
	return lc;
}

// get pointer to each line
// and a char count
Line_Meta* get_line_meta_in_blob (char* blob, long int sz, long int lc) {
	Line_Meta* lms = (Line_Meta*)malloc (lc * sizeof (Line_Meta));

	int l = 0;
	int c = 0;
	int o = 0;
	for (long int i = 0; i < sz; i++) {
		c++;
		if (blob[i] == '\n') {
			lms[l].cc = c;
			lms[l].offs = o;
			// next line starts on the next char
			o = i + 1;
			c = 0;
			l++;
		}
	}

	return lms;
}

void print_lines (Line_Meta* lms, char* blob, long int lc) {
	char tmp[1024]; // surely no line is > 1024. SURELY
	for (long int i = 0; i < lc; i++) {
		long int cc = lms[i].cc;
		long int offs = lms[i].offs;
		assert (cc < 1023);
		long int j;
		for (j = 0; j < cc; j++) {
			tmp[j] = blob[offs + j];
		}
		tmp[j] = '\0';
		printf ("[%s]", tmp);
	}
}

int main (int argc, char** argv) {
	if (argc < 2) {
		printf ("usage: ./parse_test FILE.txt\n");
		return 0;
	}
	
	printf ("reading file %s\n", argv[1]);
	long int sz = 0;
	char* blob = read_entire_file (argv[1], &sz);
	assert (blob);
	
	long int lc = count_lines_in_blob (blob, sz);
	printf ("lines in blob is %li\n", lc);

	Line_Meta* lms = get_line_meta_in_blob (blob, sz, lc);
	assert (lms);

	print_lines (lms, blob, lc);

	free (lms);
	free (blob);
	
	return 0;
}
