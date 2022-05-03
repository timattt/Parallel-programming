#include "Integrator.h"

//==========================================================================================
// STARTER
int main (int argc, char * argv[]) {
	//ENABLE_DEBUG;

	if (argc != 3) {
		printf("Strange quantity of arguments!\n");
		return 0;
	}
	
	int cpuCount = 0;
	double result = 0;
	double dx = 0;
	
	LOUD_CALL(cpuCount = STRING_TO_INT(argv[1]));
	LOUD_CALL(dx = STRING_TO_INT(argv[2]));
	
	if (cpuCount <= 0) {
		printf("bad arg!\n");
		exit(0);
	}
	
	LOUD_CALL(result = integrate(cpuCount, A, B, dx));
	printf("%f\n", result);

	return 0;
}

//==========================================================================================
