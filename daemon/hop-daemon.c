#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>  
#include <signal.h>

#include <dirent.h>
#include <sys/poll.h>

#define DAEMON_NAME	"hop-daemon"
#define HOP_DEV_DIR	"/dev/hop"
#define HOP_DEV_LEN	8
#define MAX_MON_PTS	255
#define WAIT_TIME	1000
#define BUFF_SIZE	1024

static unsigned alive = 1;
static unsigned num_mon_pts;
static struct pollfd mon_fds[MAX_MON_PTS];
static char data[BUFF_SIZE];

static void sig_exit(int sig)
{
	alive = 0;
	sleep(2);
	exit(EXIT_FAILURE);
}// sig_exit

static void read_fds(void)
{
	int id;
	for (id = 0; id < num_mon_pts; ++id) {
		if (!(mon_fds[id].revents & POLLIN))
			goto close;
		if (!read(mon_fds[id].fd, data, BUFF_SIZE))
			syslog (LOG_NOTICE, 
				"Cannot read from file [%u]", id);
close:
		close(mon_fds[id].fd);
	}
}// read_fds

int main(void)
{
	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);

	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) exit(EXIT_SUCCESS);

	/* Change the file mode mask */
	umask(0);

	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog (DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	syslog (LOG_NOTICE, "start daemon");

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		syslog (LOG_NOTICE, "Cannot get new SID, exit");
		/* Log the failure */
		goto err;
	}

	/* Change the current working directory */
	// TODO: setup a valid working directory
	if ((chdir("/")) < 0) {
		syslog (LOG_NOTICE, "Error while changing the working direcotry, exit");
		/* Log the failure */
		goto err;
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Daemon-specific initialization goes here */

	// signal(SIGINT, sig_exit);
	// signal(SIGQUIT, sig_exit);
	// signal(SIGTERM, sig_exit);
	// signal(SIGHUP, sig_exit);

	DIR *dir;
	struct dirent *chr;
	int fd;
	int rv;

	// syslog (LOG_NOTICE, "5");
	syslog (LOG_NOTICE, "start operating");

	while (alive) {
		// dir = opendir(HOP_DEV_DIR);
		// char path[16];

		// if (!dir) {
		// 	syslog (LOG_NOTICE, "Cannot oper dir %s, exit", HOP_DEV_DIR);
		// 	goto err;
		// }

		// /* read all the files and fill the fds array */
		// while ((chr = readdir(dir))) {
		// 	if (chr->d_type != DT_CHR) continue;
		// 	if (chr->d_name[0] == 'c') continue;	/* avoid ctl device */

		// 	sprintf(path, "%s/%s", HOP_DEV_DIR, chr->d_name);
		// 	fd = open(path, 0666);
		// 	if (fd < 0) {
		// 		syslog(LOG_NOTICE, "Error, cannot open %s\n", path);
		// 	} else {
		// 		mon_fds[num_mon_pts].fd = fd;
		// 		/* check for normal data */
		// 		mon_fds[num_mon_pts++].events = POLLIN;
		// 	}
		// }
		// closedir(dir);

		fd = open("/dev/hop/3892", 0666);
		if (fd < 0) {
			syslog(LOG_NOTICE, "Error, cannot open %s\n", "3892");
		} else {
			mon_fds[num_mon_pts].fd = fd;
			/* check for normal data */
			mon_fds[num_mon_pts++].events = POLLIN;
		}


		if (!num_mon_pts) {
			syslog(LOG_NOTICE, "No files in %s, waiting", HOP_DEV_DIR);
			sleep(5);
			continue;
		}
		/* poll on files */
		rv = poll(mon_fds, num_mon_pts, WAIT_TIME);

		if (rv < 0) {
			syslog(LOG_NOTICE, "Error during poll");
			continue;
		} else if (rv == 0) {
			syslog(LOG_NOTICE, "Timeout during poll");
			continue;
		}

		read_fds();
	}

	goto end;
err:
	closelog();
	exit(EXIT_FAILURE);
end:
	closelog();
	exit(EXIT_SUCCESS);
}