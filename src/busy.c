#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <busy.h>

/* Errors */
#define EFORK_MSG "Worker creation failed. Workload stopped.\n"
#define ELEAD_MSG "Error creating leader.\n"
#define ECPU_MSG "Unable to obtain CPU quantity.\n"
#define ESIG_MSG "Error installing signal handlers.\n"
#define ECHLD_MSG "A worker process exited unexpectedly.\n"
#define EWAIT_MSG "Error waiting for worker processes.\n"

static pid_t bgpgid;
static volatile sig_atomic_t stop_signo;
static sigset_t initmask;

static const int shutsigs[] = { SIGINT, SIGTERM, SIGHUP, SIGQUIT };
#define NSHUTSIGS ((int)(sizeof shutsigs / sizeof shutsigs[0]))

static void sig_stop(int signo);
static void chld_busy(void);
static int cst_sigs(void);
static int chld_dfls(void);
static int crt_bglead(void);
static int crt_bgchilds(long ncpus);
static void kill_bg(void);
static int wait_bg(void);
static void stop_reraise(void);

void run_busy(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	int rc;

	if (ncpus <= 0) {
		fprintf(stderr, ECPU_MSG);
		exit(EXIT_FAILURE);
	}
	if (cst_sigs() == -1) {
		fprintf(stderr, ESIG_MSG);
		exit(EXIT_FAILURE);
	}
	if (crt_bglead() == -1) {
		fprintf(stderr, ELEAD_MSG);
		exit(EXIT_FAILURE);
	}
	rc = crt_bgchilds(ncpus);
	if (rc == -1) {
		kill_bg();
		wait_bg();
		fprintf(stderr, EFORK_MSG);
		exit(EXIT_FAILURE);
	}
	if (sigprocmask(SIG_SETMASK, &initmask, NULL) == -1) {
		kill_bg();
		wait_bg();
		fprintf(stderr, ESIG_MSG);
		exit(EXIT_FAILURE);
	}
	rc = wait_bg();
	stop_reraise();
	if (rc == -1)
		exit(EXIT_FAILURE);
}

static void chld_busy(void)
{
	while (1);
}

static void sig_stop(int signo)
{
	stop_signo = signo;
	if (bgpgid > 0)
		(void)kill(-bgpgid, signo);
}

static int cst_sigs(void)
{
	sigset_t set;
	struct sigaction sa;
	int c;

	sigemptyset(&set);
	for (c = 0; c < NSHUTSIGS; c++)
		sigaddset(&set, shutsigs[c]);
	if (sigprocmask(SIG_BLOCK, &set, &initmask) == -1)
		return -1;

	sa.sa_handler = sig_stop;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	for (c = 0; c < NSHUTSIGS; c++)
		if (sigaction(shutsigs[c], &sa, NULL) == -1)
			return -1;
	return 0;
}

static int chld_dfls(void)
{
	struct sigaction dfl;
	int c;

	dfl.sa_handler = SIG_DFL;
	dfl.sa_flags = 0;
	sigemptyset(&dfl.sa_mask);
	for (c = 0; c < NSHUTSIGS; c++)
		if (sigaction(shutsigs[c], &dfl, NULL) == -1)
			return -1;
	return sigprocmask(SIG_SETMASK, &initmask, NULL) == -1 ? -1 : 0;
}

static int crt_bglead(void)
{
	pid_t pid = fork();

	if (pid == -1)
		return -1;
	if (pid == 0) {
		if (setpgid(0, 0) == -1)
			_exit(EXIT_FAILURE);
		if (chld_dfls() == -1)
			_exit(EXIT_FAILURE);
		(void)pause();
		_exit(EXIT_SUCCESS);
	}
	if (setpgid(pid, pid) == -1) {
		(void)kill(pid, SIGKILL);
		while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
			;
		return -1;
	}
	bgpgid = pid;
	return 0;
}

static int crt_bgchilds(long ncpus)
{
	long c;
	pid_t pid;

	for (c = 0; c < ncpus; c++) {
		pid = fork();
		if (pid == -1)
			return -1;
		if (pid == 0) {
			if (setpgid(0, bgpgid) == -1)
				_exit(EXIT_FAILURE);
			if (chld_dfls() == -1)
				_exit(EXIT_FAILURE);
			chld_busy();
			_exit(EXIT_SUCCESS);
		}
		if (setpgid(pid, bgpgid) == -1) {
			(void)kill(pid, SIGKILL);
			while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
				;
			return -1;
		}
	}
	return 0;
}

static void kill_bg(void)
{
	if (bgpgid > 0)
		(void)kill(-bgpgid, SIGKILL);
}

static int wait_bg(void)
{
	int st;
	int rc = 0;

	for (;;) {
		if (wait(&st) > 0) {
			if (WIFEXITED(st) && WEXITSTATUS(st) != EXIT_SUCCESS) {
				if (rc == 0)
					fprintf(stderr, ECHLD_MSG);
				kill_bg();
				rc = -1;
			}
			continue;
		}
		if (errno == EINTR)
			continue;
		if (errno != ECHILD) {
			fprintf(stderr, EWAIT_MSG);
			rc = -1;
		}
		return rc;
	}
}

static void stop_reraise(void)
{
	struct sigaction dfl;
	sigset_t set;

	if (stop_signo == 0)
		return;
	dfl.sa_handler = SIG_DFL;
	dfl.sa_flags = 0;
	sigemptyset(&dfl.sa_mask);
	(void)sigaction(stop_signo, &dfl, NULL);
	sigemptyset(&set);
	sigaddset(&set, stop_signo);
	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
	(void)raise(stop_signo);
}
