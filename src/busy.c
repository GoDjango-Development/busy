#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <malloc.h>
#include <errno.h>

/* Fallback for CPUs number */
#define CPU_MAXDEF 4096

/* Errors */
#define EMEM -1
#define ELIMIT -2
#define EMEM_MSG "Out of memory for PIDs.\n"
#define ELIMIT_MSG "Fork limit reached.\n"

extern int errno;

static int cpu_max;
static pid_t *cpus;
static int cpuno;

static void sig_intr(int signo);
static void chld_busy(void);

void run_busy(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	int c = 0;
	if (ncpus <= 0)
		ncpus = CPU_MAXDEF;
	cpus = malloc(sizeof(pid_t) * ncpus);
	if (!cpus) {
		fprintf(stderr, EMEM_MSG);
		exit(EXIT_FAILURE);
	}
	int fst = 0;
	for (; c < ncpus; c++) {
		cpus[c] = fork();
		if (!cpus[c]) {
			if (!c)
				setpgid(0, 0);
			else
				setpgid(0, cpus[0]);
			chld_busy();
			_exit(EXIT_SUCCESS);
		} else if(cpus[c] > 0)
			cpuno++;
		else if (cpus[c] == -1)
			if (errno == EAGAIN)
				fst = ELIMIT;
			else if (errno == ENOMEM)
				fst = EMEM;
	}
	if (fst == EMEM)
		fprintf(stderr, EMEM_MSG);
	else if (fst == ELIMIT)
		fprintf(stderr, ELIMIT_MSG);
	signal(SIGINT, sig_intr);
	while (wait(NULL) > 0);
}

static void chld_busy(void)
{
	while (1);
}

static void sig_intr(int signo)
{
	int c = 0;
	for (; c < cpuno; c++)
		kill(cpus[c], SIGINT);
}
