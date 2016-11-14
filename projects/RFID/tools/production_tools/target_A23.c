#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <termios.h>
#include "pro_tools.h"
#include "target_A23.h"


static int open_gpio(int num)
{
	char buf[30];
	sprintf(buf,"/sys/class/gpio/gpio%d/value", num);
	return open(buf, O_RDWR);
}

static unsigned char read_val(int num)
{
	unsigned char ret = -1;
	int fd = open_gpio(num);

	if (fd < 0) {
		print("ERROR!! open gpio%d failed\n", num);
		return 0;
	}
	read(fd, &ret, 1);
	close(fd);
	return ret;
}

static int write_val(int num, char *enable)
{
	int fd = open_gpio(num);
	int ret;

	if (fd < 0) {
		print("ERROR!! open gpio%d failed\n", num);
		return 0;
	}
	ret = write(fd, enable, 1);
	close(fd);
	return ret;
}

static inline void clear_screen(void)
{
	int i;
	for(i = 0; i < CLEAN_SCRREN_LINE; i++)
		print("\n");
}

// pass: GPIO_IN2 --> high
static bool is_gpio_in_pass(const int num)
{
	return read_val(num) != '0' ? true : false;
}

// these gpios will make LEDs light
static bool setup_gpio_out(void)
{
	bool ret = true;
	ret &= write_val(GPIO_OUT3, "1");
	ret &= write_val(GPIO_OUT4, "1");
	ret &= write_val(GPIO_OUT5, "1");
	ret &= write_val(GPIO_OUT6, "1");
	return ret;
}

// check if eth0 device exist..
static bool is_net_test_pass()
{
	int socId, rv;
	struct ifreq if_req;

	socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (socId < 0) {
		print("Socket failed. Errno = %d\n", errno);
		return false;
	}

	strncpy(if_req.ifr_name, NET_NAME, sizeof(if_req.ifr_name));
	rv = ioctl(socId, SIOCGIFFLAGS, &if_req);
	close(socId);

	if (rv < 0) {
		print("Ioctl failed. Errno = %d\n", errno);
		return false;
	}
	return (if_req.ifr_flags & IFF_UP) && (if_req.ifr_flags & IFF_RUNNING);
}

// try to open usb auto mount disk, test a if test.txt exist
static bool is_usb_test_pass(void)
{
	int fd = open(USB_MOUNT_PATH, O_RDONLY);
	if (fd > 0) {
		close(fd);
		return true;
	} else
		return false;
}

static int setup_uart(void)
{
	int fd;
	struct termios t;

	fd = open(MODULE_PATH, O_RDWR | O_NOCTTY);
	if (fd <= 0)
		return -1;

	tcgetattr(fd, &t);
	t.c_iflag &= ~(ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXANY
			| IXON | IXOFF | PARMRK);
	t.c_oflag &= ~OPOST;
	t.c_cflag &= ~(CRTSCTS | CSIZE | CSTOPB | PARENB);
	t.c_cflag |= CS8 | CLOCAL | CREAD | HUPCL;
	t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	cfmakeraw(&t);
	tcsetattr(fd, TCSANOW, &t);
	return fd;
}

// test module uart loopback test
static bool is_uart_test_pass()
{
	bool ret = true;
	int fd, num;
	struct timeval timeout;
	fd_set set;
	char buf[sizeof(TEST_PATTERN)];

	fd = setup_uart();
	if (fd < 0) {
		print("open uart failed!!\n");
		return false;
	}

	num = write(fd, TEST_PATTERN, sizeof(TEST_PATTERN));
	if (num != sizeof(TEST_PATTERN)) {
		print("uart write test failed!\n");
		goto out;
	}

	FD_ZERO(&set);
	FD_SET(fd, &set);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	num = select(fd + 1, &set, NULL, NULL, &timeout);
	if(num == -1) {
		print("select error!!\n");
		ret = false;
		goto out;
	} else if (num == 0) {
		print("Uart test timeout!!\n");
		ret = false;
		goto out;
	} else
		read(fd, buf, sizeof(TEST_PATTERN));

	if (strncmp(TEST_PATTERN, buf, sizeof(TEST_PATTERN))) {
		print("uart read match failed!!\n");
		ret = false;
	}
out:
	close(fd);
	return ret;
}

// return true if jig is inserted
bool is_jig_in(void)
{
	return read_val(GPIO_JIG_DET) != '0' ? true : false;
}


//start the whole test
bool test_start(void)
{
	bool ret = true;
	int item_num = 1;

	print("Start test program...wait for %d ms...\n", START_DELAY/1000);
	usleep(START_DELAY);
	clear_screen();
	print("\n\n\n");
	print("%s======== Test Start ========\n", LEFT_MARGIN);
	print("%s====== %d. GPIO OUT Test     ==== (%c)\n", LEFT_MARGIN, item_num++, setup_gpio_out() ? 'O' : 'X');
	print("%s====== %d. GPIO1 IN Test     ==== (%c)\n", LEFT_MARGIN, item_num++, is_gpio_in_pass(GPIO_IN1) ? 'O' : 'X');
	print("%s====== %d. GPIO2 IN Test     ==== (%c)\n", LEFT_MARGIN, item_num++, is_gpio_in_pass(GPIO_IN2) ? 'O' : 'X');
	print("%s====== %d. USB Test          ==== (%c)\n", LEFT_MARGIN, item_num++, is_usb_test_pass() ? 'O' : 'X');
	print("%s====== %d. Network Test      ==== (%c)\n", LEFT_MARGIN, item_num++, is_net_test_pass() ? 'O' : 'X');
	print("%s====== %d. Module uart Test  ==== (%c)\n", LEFT_MARGIN, item_num++, is_uart_test_pass() ? 'O' : 'X');
	print("\n\n\n");
	return ret;
}
