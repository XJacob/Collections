#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include "antenna.h"

//#define DEBUG_ON_PC
#define ANT_GPIO1 7
#define ANT_GPIO2 8
#define ANT_GPIO3 9
#define SCAN_GPIO 10
#define OUT_GPIO3 16
#define OUT_GPIO4 17
#define IN_GPIO1 18
#define IN_GPIO2 19

int enable_scan(int on_off)
{
	char buf[30];
	int fd;
	sprintf(buf,"/sys/class/drchw/scan");
	fd = open(buf, O_RDWR);
	sprintf(buf, "%d", on_off);
	write(fd, buf, 1);
	close(fd);
}

void set_antenna_port(int port)
{
#ifdef DEBUG_ON_PC
	printf("enable port:%d\n", port);
#else
#endif
}
