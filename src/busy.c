#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>

#define CPU_MAX 4096

static pid_t cpus[CPU_MAX];
static int cpuno;

static void sig_intr(int signo);
static void chld_busy(void);

void run_busy(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	int c = 0;
	for (; c < ncpus; c++) {
		cpus[c] = fork();
		if (!cpus[c]) {
			if (!c)
				setpgid(0, 0);
			else
				setpgid(0, cpus[0]);
			chld_busy();
			_exit(1);
		} else if(cpus[c] > 0)
			cpuno++;
	}
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
