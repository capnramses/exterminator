#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main (int argc, char** argv) {
	int sz = atoi (argv[1]);
	printf ("generating file of size %i\n", sz);
	FILE* f = fopen ("ascii.txt", "w");
	int ll = 0;
	for (int i = 0; i < sz; i++) {
		if (ll == 79) {
			char c = '\n';
			fwrite (&c, 1, 1, f);
			ll = 0;
			continue;
		}
		ll++;
		char c = rand () % 94 + 32;
		assert (c >= 32 && c <= 126);
		fwrite (&c, 1, 1, f);
	}
	//char c = '\0';
	//fwrite (&c, 1, 1, f);
	fclose (f);
}
