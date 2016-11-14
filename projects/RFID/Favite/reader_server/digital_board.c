/**
 * @file
 * @brief Control the functions on the digital boards
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include "default_val.h"
#include "digital_board.h"

#define OUT_GPIO1 16
#define OUT_GPIO2 17
#define OUT_GPIO3 20
#define OUT_GPIO4 21
#define IN_GPIO1 18
#define IN_GPIO2 19

#define SCAN_LED_ONESHOT_DELAY 50 //ms
pthread_t ant_thread_id;

static int open_gpio(int num)
{
	char buf[30];

	sprintf(buf,"/sys/class/gpio/gpio%d/value", num);
	return open(buf, O_RDWR);
}

static char read_val(int num)
{
#ifndef DEBUG_ON_PC
	unsigned char ret = -1;
	int fd = open_gpio(num);

	if (fd < 0) {
		printf("open file failed");
		return ret;
	}
	read(fd, &ret, 1);
	close(fd);
	return ret;
#else
	return 0;
#endif
}

static char write_val(int num, char *enable)
{
#ifndef DEBUG_ON_PC
	int fd = open_gpio(num);

	if (fd < 0) {
		printf("open file failed\n");
		return -1;
	}
	write(fd, enable, 1);
	close(fd);
#else
	return 0;
#endif
}

char __module_reset(void)
{
	write_val(135,"0");
	usleep(10000);
	write_val(135,"1");
	usleep(10000);
	write_val(135,"0");
	return 0;
}

void oneshot_scan_led(int on)
{
	char buf[30];
	static int fd;

	sprintf(buf,"/sys/class/drchw/oneshot_scan");
	if (fd <= 0)
		fd = open(buf, O_WRONLY);
	if (on)
		sprintf(buf, "%d", SCAN_LED_ONESHOT_DELAY);
	write(fd, buf, 1);
	// For performance, we won't close this file!!
	//close(fd);
}

void __set_gpio(unsigned char gpio1, unsigned char gpio2, unsigned char gpio3, unsigned char gpio4)
{
	write_val(OUT_GPIO1, gpio1? "1":"0");
	write_val(OUT_GPIO2, gpio2? "1":"0");
	write_val(OUT_GPIO3, gpio3? "1":"0");
	write_val(OUT_GPIO4, gpio4? "1":"0");
}

void __get_gpio(unsigned char *gpio1, unsigned char *gpio2)
{
	*gpio1 = read_val(IN_GPIO1) - 0x30; // ascii code to int
	*gpio2 = read_val(IN_GPIO2) - 0x30;
}
