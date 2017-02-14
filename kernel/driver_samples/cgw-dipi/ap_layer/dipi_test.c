#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DI_EN_FILE "/sys/class/misc/cgw_dipi_port/DI1_EN"
#define DI_CNT_FILE "/sys/class/misc/cgw_dipi_port/DI1_CNT"
#define PI1_EN_FILE "/sys/class/misc/cgw_dipi_port/PI1_EN"
#define PI1_CNT_FILE "/sys/class/misc/cgw_dipi_port/PI1_CNT"
#define PI2_EN_FILE "/sys/class/misc/cgw_dipi_port/PI2_EN"
#define PI2_CNT_FILE "/sys/class/misc/cgw_dipi_port/PI2_CNT"

#define ENABLE "1"
#define DISABLE "0"
#define CLEAN   "0"

static bool monitor_stop;
static pthread_t monitor_thd;

static int inline open_dev_file(const char *name, int flag)
{
	int fd;
	fd = open(name, flag);

	if (fd < 0) {
		fprintf(stderr, "Error with something %s : ", name);
		perror("");
	}

	return fd;
}

static int inline clean_count(const char *name)
{
	int fd = open_dev_file(name, O_WRONLY);
	if (fd > 0) {
		write(fd, CLEAN, sizeof(CLEAN));
		close(fd);
	}
}

static int inline enable_func(const char *name, bool on_off)
{
	int fd = open_dev_file(name, O_RDWR);
	if (fd < 0)
		return fd;

	if (on_off)
		write(fd, ENABLE, sizeof(ENABLE));
	else
		write(fd, DISABLE, sizeof(DISABLE));
	close(fd);
}

static void monitor_thread(void *data)
{
	int ret, cnt;
	char buf[10];
	struct pollfd ufds[3];

	ufds[0].fd = open_dev_file(DI_CNT_FILE, O_RDONLY);
	if (ufds[0].fd < 0)
		return;

	ufds[1].fd = open_dev_file(PI1_CNT_FILE, O_RDONLY);
	if (ufds[1].fd < 0)
		return;

	ufds[2].fd = open_dev_file(PI2_CNT_FILE, O_RDONLY);
	if (ufds[2].fd < 0)
		return;

	for(ret=0; ret<3; ret++) {
		ufds[ret].events = POLLPRI | POLLERR;
		ufds[ret].revents = 0;
	}

	memset(buf, 0, sizeof(buf));
	printf("monitor file:%s, byte=%d data=%s\n", DI_CNT_FILE, read(ufds[0].fd, buf, sizeof(buf)), buf);

	memset(buf, 0, sizeof(buf));
	printf("monitor file:%s, byte=%d data=%s\n", PI1_CNT_FILE, read(ufds[1].fd, buf, sizeof(buf)), buf);

	memset(buf, 0, sizeof(buf));
	printf("monitor file:%s, byte=%d data=%s\n", PI2_CNT_FILE, read(ufds[2].fd, buf, sizeof(buf)), buf);

	while (!monitor_stop) {
		if ((ret = poll(ufds, 3, 3000)) < 0) {
			perror("poll error");
		/*
		 *} else if (!ret) {
		 *    printf("Timeout occurred!\n");
		 */
		} else if (ufds[0].revents & (POLLPRI|POLLERR)) {
			lseek(ufds[0].fd, 0, SEEK_SET);
			memset(buf, 0, sizeof(buf));
			cnt = read(ufds[0].fd, buf, sizeof(buf) );
			printf("DI trigger: value: (%s)\n", buf);
		} else if (ufds[1].revents & (POLLPRI|POLLERR)) {
			lseek(ufds[1].fd, 0, SEEK_SET);
			memset(buf, 0, sizeof(buf));
			cnt = read(ufds[1].fd, buf, sizeof(buf) );
			printf("P1-Input trigger: value: (%s)\n", buf);
		} else if (ufds[2].revents & (POLLPRI|POLLERR)) {
			lseek(ufds[2].fd, 0, SEEK_SET);
			memset(buf, 0, sizeof(buf));
			cnt = read(ufds[2].fd, buf, sizeof(buf) );
			printf("P2-Input trigger: value: (%s)\n", buf);
		}
	}
	close(ufds[0].fd);
	close(ufds[1].fd);
	close(ufds[2].fd);
}

void main()
{
	// turn on digital input pin function
	enable_func(DI_EN_FILE, true);
	enable_func(PI1_EN_FILE, true);
	enable_func(PI2_EN_FILE, true);

	// create a pthread to monitor the driver trigger
	pthread_create(&monitor_thd, NULL, (void *)*monitor_thread, NULL);
	monitor_stop = false;


	// monitor is running on another thread
	sleep(30);
	printf("Clean all counts!!\n");
	clean_count(DI_CNT_FILE);
	clean_count(PI1_CNT_FILE);
	clean_count(PI2_CNT_FILE);
	sleep(30);
	monitor_stop = true;

	// turn off digital input pin function
	enable_func(DI_EN_FILE, false);
	enable_func(PI1_EN_FILE, false);
	enable_func(PI2_EN_FILE, false);
	pthread_join(monitor_thd, NULL);
	return;
}
