#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <malloc.h>
#include <errno.h>

/* Errors */
#define EFORK_MSG "Forking error. Program may continue running.\n"
#define ELEAD_MSG "Error creating leader.\n."
#define ECPU_MSG "Unable to obtaing CPUs quantity.\n"

static pid_t bgpgid;

static void sig_intr(int signo);
static void chld_busy(void);
static int crt_bglead(void);
static int crt_bgchilds(int ncpus);

void run_busy(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	struct sigaction sa;
	if (ncpus <= 0) {
		fprintf(stderr, ECPU_MSG);
		exit(EXIT_FAILURE);
	}
	int rc = crt_bglead();
	if (rc) {
		fprintf(stderr, ELEAD_MSG);
		exit(EXIT_FAILURE);
	}
	rc = crt_bgchilds(ncpus);
	if (rc)
		fprintf(stderr, EFORK_MSG);
	sa.sa_handler = sig_intr;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1 ||
	    sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction");
		kill(-bgpgid, SIGKILL);
		while (wait(NULL) > 0 || errno == EINTR);
		exit(EXIT_FAILURE);
	}
	while (wait(NULL) > 0);
}

static void chld_busy(void)
{
	while (1);
}

static void sig_intr(int signo)
{
	int saved_errno = errno;
	kill(-bgpgid, signo);
	errno = saved_errno;
}

static int crt_bglead(void)
{
	pid_t pid = fork();
	if (!pid) {
		if (setpgid(0, 0) == -1)
			return -1;
		pause();
		_exit(EXIT_SUCCESS);

	} else if (pid == -1)
		return -1;
	else {
		if (setpgid(pid, pid) == -1)
			return -1;
		bgpgid = pid;
	}
	return 0;
}

static int crt_bgchilds(int ncpus)
{
	int c = 0;
	int rc = 0;
	pid_t pid;
	for (; c < ncpus; c++) {
		pid = fork();
		if (!pid) {
			if (setpgid(0, bgpgid) == -1)
				_exit(EXIT_FAILURE);
			chld_busy();
			_exit(EXIT_SUCCESS);
		} else if (pid > 0) {
			if (setpgid(pid, bgpgid) == -1) {
				kill(pid, SIGINT);
				rc = -1;
			}
		} else
			rc = -1;
	}
	return rc;
}
