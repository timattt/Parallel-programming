#include "Integrator.h"

//==========================================================================================
// Structs
typedef struct CalculateData {
    double a;
    double b;
    double result;
    cpu_set_t cpuset;
	double dx;
} CalculateData;

typedef struct SysInfo {
    char * is_virtual_cpu_available;
    unsigned total_available_virtual_cpus;
    cpu_set_t * virtual_cpu_sets;
} SysInfo;
//==========================================================================================

#define MAX(A, B) (A > B ? A : B)

//==========================================================================================
// Local functions
long STRING_TO_INT(char *str) {
	long input = 0;
	char *strptr = str, *endptr = NULL;
	input = strtol(strptr, &endptr, 10);

	if (endptr == strptr || *endptr != '\0') {
		return -1;
	}

	if (errno == ERANGE && (input == LONG_MAX || input == LONG_MIN)) {
		return -1;
	}

	return input;
}

double STRING_TO_DOUBLE(char *str) {
	double input = 0;
	char *strptr = str, *endptr = NULL;
	input = strtod(strptr, &endptr);

	if (endptr == strptr || *endptr != '\0') {
		return -1;
	}

	if (errno == ERANGE && (input == DBL_MAX || input == DBL_MIN)) {
		return -1;
	}

	return input;
}


int fillVirtualCpu(SysInfo * si, unsigned cpuCount) {
	SAFE_PTR(si);

	si->total_available_virtual_cpus = get_nprocs();

	si->is_virtual_cpu_available = malloc(MAX(cpuCount, si->total_available_virtual_cpus));
	si->virtual_cpu_sets = malloc(MAX(cpuCount, si->total_available_virtual_cpus) * sizeof(cpu_set_t));

	if ((si->is_virtual_cpu_available == NULL) || (si->virtual_cpu_sets == NULL)) {
		return -1;
	}

	memset(si->is_virtual_cpu_available, 0, MAX(cpuCount, si->total_available_virtual_cpus));
	memset(si->virtual_cpu_sets, 0, MAX(cpuCount, si->total_available_virtual_cpus) * sizeof(cpu_set_t));

	// I
	// read cpu/online
	//--------------------
	int fd = 0;
	char text[MAX_SYS_FILE_LENGTH] = { 0 };
	int textLen = 0;

	LOUD_CALL(fd = open(CPU_ONLINE_FILE_PATH, O_RDONLY));
   	LOUD_CALL(textLen = read(fd, text + 1, MAX_SYS_FILE_LENGTH - 2));
	LOUD_CALL(close(fd));

	text[0] = ',';
	text[textLen] = '\0';


	//--------------------

	// II
	//--------------------
	unsigned commaIndex = 0;
	unsigned block_start = 0;
	unsigned block_end = 0;
	unsigned hyphen_pos = 0;
	char * first_str = NULL;
	char * second_str = NULL;
	unsigned first = 0;
	unsigned second = 0;

	while (commaIndex < textLen) {
		// determining block sizes
		//--------------------------
		block_start = commaIndex;
		commaIndex++;

		while (text[commaIndex] != ',' && text[commaIndex] != '\0') {
			commaIndex++;
		}

		block_end = commaIndex;

		//--------------------------

		// Determining hyphen position
		//--------------------------
		for (hyphen_pos = block_start;
			(hyphen_pos < block_end) && (text[hyphen_pos] != '-'); hyphen_pos++) {}

		//--------------------------

		// Converting
		//--------------------------
		first_str = text + block_start + 1;
		second_str = text + hyphen_pos + 1;

		text[block_end] = '\0';
		text[hyphen_pos] = '\0';


		LOUD_CALL(first = STRING_TO_INT(first_str));
		LOUD_CALL(second = STRING_TO_INT(second_str));

		//--------------------------

		// Editing sysInfo
		//--------------------------
		for (unsigned i = first; i <= second; i++) {
			si->is_virtual_cpu_available[i] = 1;
		}
		//--------------------------

	}


	//--------------------


	return 0;
}

int clearCpuSets(SysInfo * si, unsigned cpuCount) {
	SAFE_PTR(si);
	for (unsigned i = 0; i < MAX(cpuCount, si->total_available_virtual_cpus); i++) {
		CPU_ZERO(&(si -> virtual_cpu_sets[i]));
	}
	return 0;
}

int initCpus(SysInfo * si, unsigned cpuCount) {
	SAFE_PTR(si);

	SAFE_CALL(fillVirtualCpu(si, cpuCount));
	SAFE_CALL(clearCpuSets(si, cpuCount));

	return 0;
}

double func(double x) {
	return x * x;
}

void* calc(void* args) {
	CalculateData * data = ((CalculateData*)args);

	cpu_set_t cpuset = ((CalculateData*)args)->cpuset;
	pthread_t current_thread = pthread_self();
	double start = ((CalculateData*)args)->a;
	double finish = ((CalculateData*)args)->b;
	double delta = ((CalculateData*)args)->dx;
	double result = 0;

	if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) == 0) {
		perror("bad");
		exit(-1);
	}

	data->result = 0;

	double x = start;

	 for (; x < finish; x += delta) {
		((CalculateData*)args)->result /*result*/ += func(x) * delta;
	}

	return NULL;
}

int cache_line_size()
{
	FILE* cache_info = cache_info = fopen("/sys/bus/cpu/devices/cpu0/cache/index0/coherency_line_size", "r");
	if (cache_info == NULL) {
		return -1;
	}

	int line_size = 0;
	LOUD_CALL(fscanf(cache_info, "%d", &line_size));
	fclose(cache_info);

	return line_size;
}

int createThreadsBuffers(int total, char ** dat, int * bufSize) {
	int line_size = 0;
	LOUD_CALL(line_size = cache_line_size());

	int info_size = sizeof(CalculateData);

	if (info_size <= line_size) {
		info_size = 2 * line_size;
	} else {
		info_size = (info_size / line_size + 2) * line_size;
	}

	*bufSize = info_size;

	*dat = (char*)malloc(total * info_size);

	if (*dat == NULL) {
		return -1;
	}

	memset(*dat, 0, total * info_size);

	return 0;
}

double runCalculations(SysInfo * si, unsigned cpuCount, double a, double b, double dx) {
	SAFE_PTR(si);

	pthread_t * threads = calloc(cpuCount, sizeof(pthread_t));
	if (!threads) {
		return -1;
	}
	int bufSize = 0;
	char * data = NULL;
	LOUD_CALL(createThreadsBuffers(cpuCount, &data, &bufSize));

	double len = 1/a - 1/b;//длина отрезка в синусе
	int N = len / (2*3.1415);//кол периодов синуса
	

	for (unsigned i = 0; i < cpuCount; i++) {
		CalculateData * cur = (CalculateData*) (data + i * bufSize);

		*cur = (CalculateData) {
				1.0 / (1/b + len*(i+1)/N),
				1.0 / (1/b + len*i/N),
				0,
				si->virtual_cpu_sets[i % si->total_available_virtual_cpus], dx
            		};

        }

	// Start threads
	//----------------------------
	for (unsigned i = 0; i < cpuCount; i++) {
	CalculateData * cur = (CalculateData*) (data + i * bufSize);
		if (pthread_create(&threads[i], NULL, &calc, (data + i * bufSize)) != 0)
    {
      perror("bad create");
      exit(-1);
    }
	}
	//----------------------------

	// Finish threads
	//----------------------------
	double result = 0;
	for (unsigned i = 0; i < MAX(cpuCount, si->total_available_virtual_cpus); i++) {
		LOUD_CALL(pthread_join(threads[i], NULL));
		CalculateData * cur = (CalculateData*) (data + i * bufSize);
		if (i < cpuCount) {
			result += cur->result;
		}
	}
	//----------------------------

	free(data);

	return result;
}

int cleanup(SysInfo * si) {
	free(si->is_virtual_cpu_available);
	free(si->virtual_cpu_sets);
	free(si);
	return 0;
}

//==========================================================================================



//==========================================================================================
// Global functions

double integrate(unsigned cpuCount, double a, double b, double dx) {
	double result = 0;
	SysInfo * info = calloc(1, sizeof(SysInfo));

	LOUD_CALL(initCpus(info, cpuCount));
	LOUD_CALL(result = runCalculations(info, cpuCount, a, b, dx));
	LOUD_CALL(cleanup(info));

	return result;
}
