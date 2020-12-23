/* Test our copy of qsort_r against the system one. */

#include <stdio.h>
#include <stdlib.h>
#include "qsort-r.c"

int comp (void * unused, const void * a, const void * b)
{
    int ia;
    int ib;
    ia = * (int *) a;
    ib = * (int *) b;
    return ib - ia;
}

int main ()
{
    int n = 1000;
    int r[n];
    int s[n];
    int i;

    for (i = 0; i < n; i++) {
	int j;
	j = random ();
	r[i] = j;
	s[i] = j;
    }
    qsort_r (r, n, sizeof (int *), 0, comp);
    json_create_qsort_r (s, n, sizeof (int *), 0, comp);

    for (i = 0; i < n; i++) {
	if (r[i] != s[i]) {
	    printf ("Disagreement at %d %d != %d.\n", i, r[i], s[i]);
	}
    }
    return 0;
}
