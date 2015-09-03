#include <gdb.h>

int main (int argc, char** argv) {
	printf ("debugging %s\n", argv[1]);
	gdb_init ();

	return 0;
}
