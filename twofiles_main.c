#include "twofiles.h"
#include <stdio.h>

int main () {
	for (int i = 0; i < 10; i++) {
		int r = add (i, i * 2);
		printf ("r=%i\n", r);
	}
	return 0;
}

