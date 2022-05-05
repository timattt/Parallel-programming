#pragma once

// Includes
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/time.h>  
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <math.h>
#include <float.h>

//==========================================================================================
// Defines
#define A 1.0
#define B 10.0
#define MAX_SYS_FILE_LENGTH 256
#define MAX_THREADS 10000
#define DX 0.0000001
#define CPU_ONLINE_FILE_PATH "/sys/devices/system/cpu/online"
#define CALC_FUNCTION(X) sin(1/X)
#define SAFE_CALL(COND) do { if ((COND) == -1) { return -1;} } while (0)
#define LOUD_CALL(COND) do { if ((COND) == -1) { printf("Loud call error in file %s on line %d\n\n", __FILE__, __LINE__); /*perror ("");*/ exit(0);} } while (0)
#define SAFE_PTR(PARAM) do { if ((PARAM) == NULL) {errno = EFAULT; return -1;} } while (0)
//==========================================================================================

// Prototypes
double integrate(unsigned cpuCount, double a, double b, double dx);
long STRING_TO_INT(char *str);
double STRING_TO_DOUBLE(char *str);

