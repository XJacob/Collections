/**
 * @file
 * @brief Control the functions on the digital boards
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "default_val.h"
#include "digital_board.h"

#define OUT_GPIO1 16
#define OUT_GPIO2 17
#define OUT_GPIO3 20
#define OUT_GPIO4 21
#define IN_GPIO1 18
#define IN_GPIO2 19

#define SCAN_LED_ONESHOT_DELAY 50 //ms
pthread_t shutdown_thd_id;

static int open_gpio(int num)
{
	char buf[30];
	sprintf(buf,"/sys/class/gpio/gpio%d/value", num);
	return open(buf, O_RDWR);
}

static char read_val(int num)
{
	unsigned char ret = -1;
	int fd = open_gpio(num);

	if (fd < 0) {
		printf("open file failed");
		return ret;
	}
	read(fd, &ret, 1);
	close(fd);
	return ret;
}

static char write_val(int num, char *enable)
{
	int fd = open_gpio(num);

	if (fd < 0) {
		printf("open file failed\n");
		return -1;
	}
	write(fd, enable, 1);
	close(fd);
	return 0;
}

char __module_reset(void)
{
	printf("module reset!!!\n");
	write_val(136,"1");
	usleep(1000000);
	write_val(136,"0");
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

static unsigned short polling_time;
static int currently_led = 3;
static pthread_t led_thread_id;
static int leds[4];
static int enable_ant(int num)
{
	char buf[30];
	int fd;
	sprintf(buf,"/sys/class/drchw/ant");
	fd = open(buf, O_RDWR);
	sprintf(buf, "%d", num);
	write(fd, buf, 1);
	close(fd);
}

static int enable_led(unsigned int led)
{
	switch(led)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			enable_ant(led+1);
			currently_led = led;
			break;
		default:
			enable_ant(0);
			currently_led = 3;
			break;
	}
}

static int find_next_avail_led(int id)
{
	int i = (id+1) % 4;
	while(!leds[i] && (i != id)) {
		i++;
		i %= 4;
	}
	return leds[i] ? i : -1;
}

static void *polling_thread(void *data)
{
	int led;
	while (polling_time) {
		led = find_next_avail_led(currently_led);
		if (led >= 0) {
			enable_led(led);
		}
		usleep(polling_time * 1000);
	}
}

static char set_antenna_polling(unsigned short time)
{
	DEBUG("Set polling time to %dms. %dms\n", time, polling_time);
	if (!time) {
			enable_led(-1);
			polling_time = time;
			if (led_thread_id)
				pthread_join(led_thread_id, NULL);
			led_thread_id = 0;
			return;
	}

	if (polling_time == time) return;
	polling_time = 0;
	if (led_thread_id)
		pthread_join(led_thread_id, NULL); //make sure the previous thread is stoped

	polling_time = time;
	pthread_create(&led_thread_id, NULL, (void *)*polling_thread, NULL);
}


void __set_ant_led(unsigned char led1, unsigned char led2, unsigned char led3, unsigned char led4, unsigned short time)
{
	DEBUG("set led:%d %d %d %d %d\n", led1, led2, led3, led4, time);
	leds[0] = led1;
	leds[1] = led2;
	leds[2] = led3;
	leds[3] = led4;

	if (!time || !(led1|led2|led3|led4)) {
		set_antenna_polling(0);
	} else {
		set_antenna_polling(time);
	}
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

/**
 * config format->  key:value
 */
static int find_value_from_conf(const char *filename, const char *key,
		char *result, int limit_len)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *value, *sperator;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("open file error!!\n");
		return 0;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		if (line[0] != '#') {
			printf("%s", line);

			sperator = strchr((const char *)line, (int)':');
			if ((strlen(key) == (sperator-line)) &&
				!strncmp(line, key, sperator-line)) {
				strncpy(result, sperator+1, limit_len);
				break;
			} else
				printf("different!");
		}
	}

	fclose(fp);
	if (line)
		free(line);
	return 1;
}

unsigned char __get_fw_version(void)
{
	unsigned char tmp[16];
	unsigned char ret;

	if (!find_value_from_conf(SYS_CONFIG_FILE, "Firmware", tmp, 16)) {
		printf("ERROR!! can't find system config file!\n");
		return 0;
	}
	sscanf(tmp, "%d", &ret);
	return ret;
}

static unsigned int func;
static void *shutdown_thd(void *data)
{
	if (func == 1)
		printf("shutdown thread starting..will reboot after 2 seconds...\n");
	else
		printf("system update thread starting..will reboot after 2 seconds...\n");

	sleep(2);

	if (func == 1)
		system("reboot");
	else
		system("/home/root/bin/update.sh");
		printf("system update thread starting..will reboot after 2 seconds...\n");
}

void __device_reboot()
{
	func = 1;
	pthread_create(&shutdown_thd_id, NULL, (void *)*shutdown_thd, NULL);
}

void __system_update()
{
	func = 2;
	pthread_create(&shutdown_thd_id, NULL, (void *)*shutdown_thd, NULL);
}

void __set_system_ip(const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;
	struct in_addr dest_ip, dest_mask, dest_gateway;
	struct stat st = {0};
	unsigned char tmp[30]={0}, tmp_mask[30]={0}, tmp_gateway[30]={0};
	int fd, str_ip, str_mask, str_gateway;

	dest_ip.s_addr = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
	dest_mask.s_addr = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
	dest_gateway.s_addr = buf[8] | buf[9] << 8 | buf[10] << 16 | buf[11] << 24;
	printf("Will set ip addr to :%s\n", inet_ntoa(dest_ip));
	printf("   & set netmask to :%s\n", inet_ntoa(dest_mask));
	printf("   & set gateway to :%s\n", inet_ntoa(dest_gateway));
	str_ip = sprintf(tmp, "address:%s\n", inet_ntoa(dest_ip));
	str_mask = sprintf(tmp_mask, "netmask:%s\n", inet_ntoa(dest_mask));
	str_gateway = sprintf(tmp_gateway, "gateway:%s", inet_ntoa(dest_gateway));

	if (stat(CONFIG_DIR, &st) == -1) {
		mkdir(CONFIG_DIR, 0700);
	}

	fd = open(SAVED_IP_FILE, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (fd > 0) {
		write(fd, tmp, str_ip);
		write(fd, tmp_mask, str_mask);
		write(fd, tmp_gateway, str_gateway);
		close(fd);
	}

	printf("IP addr changed to %s!!\n", inet_ntoa(dest_ip));
	printf("Netmask changed to %s!!\n", inet_ntoa(dest_mask));
	printf("Gateway changed to %s!!\n", inet_ntoa(dest_gateway));

	func = 1;
	pthread_create(&shutdown_thd_id, NULL, (void *)*shutdown_thd, NULL);
}

