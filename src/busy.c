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
static sigset_t initial_mask;

static void sig_intr(int signo);
static void chld_busy(void);
static int crt_bglead(void);
static int crt_bgchilds(long ncpus, long *created);
static void stop_bg(void);

void run_busy(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	long created = 0;
	sigset_t blocked;
	if (ncpus <= 0) {
		fprintf(stderr, ECPU_MSG);
		exit(EXIT_FAILURE);
	}
	sigemptyset(&blocked);
	sigaddset(&blocked, SIGINT);
	sigaddset(&blocked, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &blocked, &initial_mask) == -1) {
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}
	int rc = crt_bglead();
	if (rc) {
		fprintf(stderr, ELEAD_MSG);
		exit(EXIT_FAILURE);
	}
	rc = crt_bgchilds(ncpus, &created);
	if (rc)
		fprintf(stderr, EFORK_MSG);
	if (created == 0) {
		stop_bg();
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, sig_intr);
	if (sigprocmask(SIG_SETMASK, &initial_mask, NULL) == -1) {
		perror("sigprocmask");
		stop_bg();
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
	int c = 0;
	kill(-bgpgid, SIGINT);
}

static int crt_bglead(void)
{
	pid_t pid = fork();
	if (!pid) {
		if (setpgid(0, 0) == -1)
			_exit(EXIT_FAILURE);
		if (sigprocmask(SIG_SETMASK, &initial_mask, NULL) == -1)
			_exit(EXIT_FAILURE);
		pause();
		_exit(EXIT_SUCCESS);

	} else if (pid == -1)
		return -1;
	else {
		if (setpgid(pid, pid) == -1) {
			kill(pid, SIGKILL);
			while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
			return -1;
		}
		bgpgid = pid;
	}
	return 0;
}

static int crt_bgchilds(long ncpus, long *created)
{
	long c = 0;
	int rc = 0;
	pid_t pid;
	for (; c < ncpus; c++) {
		pid = fork();
		if (!pid) {
			if (setpgid(0, bgpgid) == -1)
				_exit(EXIT_FAILURE);
			if (sigprocmask(SIG_SETMASK, &initial_mask, NULL) == -1)
				_exit(EXIT_FAILURE);
			chld_busy();
			_exit(EXIT_SUCCESS);
		} else if (pid > 0) {
			if (setpgid(pid, bgpgid) == -1) {
				kill(pid, SIGKILL);
				while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
				rc = -1;
			} else
				(*created)++;
		} else
			rc = -1;
	}
	return rc;
}

static void stop_bg(void)
{
	kill(-bgpgid, SIGKILL);
	while (wait(NULL) > 0 || errno == EINTR);
}
