#include <inttypes.h>
#include <stdio.h>

int main() {

	FILE * fp;
	fp = fopen ("datasizes.h", "w");
   
	fprintf(fp,"#define SIZEOF_INT %zd\n", sizeof(int));
	fprintf(fp,"#define SIZEOF_LONG %zd\n", sizeof(long));
	fprintf(fp,"#define SIZEOF_LONG_DOUBLE %zd\n", sizeof(long double));
	fprintf(fp,"#define SIZEOF_VOID_P %zd\n", sizeof(void *));
	
	fclose(fp);
	return 0;
}