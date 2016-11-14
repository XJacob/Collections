/**
 * @file
 * @brief Raw module read/write
 *
 * All module raw read/write function should be defined \n
 * in this file
 */
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include "favite_llc.h"
#include "default_val.h"
#include "module.h"

static int is_opened;
static int fd;
static int socket_fd;
static int b_exit;
static int raw_data_en;
static pthread_t thread_id;
static struct read_thread_param param;

/**
 *Pass the buffer data to module by a opened fd
 *
 * @param buf the data
 * @param len the data len
 * @return how many bytes have been written
 */
int write_module(const char *buf, int len)
{
	int n;

	if (!is_module_open()) {
		printf("Need to open module before write to it!\n");
		return 0;
	}
	n = write(fd, buf, len);
	if (debug_flag) {
		int i;
		printf("Write: ");
		for(i=0; i<len; i++) {
			printf(" 0x%x ", buf[i]);
			if (!i%16)
				printf("\n");
		}
		printf("\n return %d bytes.\n", n);
	}
	return n;
}

/**
 * check if module is opened.
 *
 * @return 0 if module is not opened
 */
int is_module_open(void)
{
	return is_opened;
}

/**
 * Setup uart configuration by the default_val
 *
 * @return the opened UART device fd number
 */
static int setup_uart(void)
{
	int fd;
	struct termios options;

	fd = open(MODULE_PATH, O_RDWR | O_NOCTTY);
	if (fd <= 0)
		return -1;
#ifndef DEBUG_ON_PC
	tcgetattr(fd, &options);
	cfsetispeed(&options, BAUD_RATE);
	cfsetospeed(&options, BAUD_RATE);
	cfmakeraw(&options);
	options.c_cflag |= (CLOCAL | CREAD);   // Enable the receiver and set local mode
	options.c_cflag &= ~CSTOPB;            // 1 stop bit
	options.c_cflag &= ~CRTSCTS;           // Disable hardware flow control
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSIZE;
	options.c_iflag &= ~(IXON | IXOFF | IXANY );
	options.c_cflag |= CS8;
	options.c_cc[VTIME]=5;  //0.1 * 5 = 0.5s timeout
	options.c_cc[VMIN]=128; //buffer size 256 bytes
	tcsetattr(fd, TCSANOW, &options);
#endif
	return fd;
}

static unsigned int find_next_start(unsigned char *buf, unsigned int pos, unsigned int end)
{
	if ((pos > end) || ((pos - end) < 2)) {
		printf("process data error, position:%d\n", pos);
		return 0;
	}
	while(pos < end) {
		if (buf[pos] == END_MARK && buf[pos+1] == PREAMBLE)
			return pos+1;
		else
			pos++;
	}
	printf("failed to find start byte!!\n");
	return 0;
}

static inline int is_tag_notify(unsigned char *buf)
{
	return (buf[0] == PREAMBLE) && (buf[1] == TYPE_NOF) && (buf[2] == EPC_MC || buf[2] == TID_MC);
}


/**
 * process buffer data
 *
 * @param buf  buffer data from module
 * @param len  buffer data len
 * @return buffer length \n
 *         if return -1, this buffer won't send to client
 */
static int process_buf(unsigned char *buf, int len)
{
	unsigned int cur = 0, j, cmd_len, buf_len = len;

	if (raw_data_en)
		return len;
	while (buf_len && (buf_len - cur) > 8) {
		if (buf[cur] == PREAMBLE) {
			cmd_len = CMD_LENGTH((buf + cur));
			if ((cur + cmd_len) > buf_len)
				break;
			if (is_tag_notify(buf + cur)) {
				cur += cmd_len;
				oneshot_scan_led(1);
			} else
				cur += cmd_len;
		} else {
			cur = find_next_start(buf, cur, buf_len);
			if (!cur) {
				break;
			}
		}
	}
	DEBUG("Drop bytes: %d\n", len - buf_len);
	//We didn't change buffer len
	return buf_len;
}

/**
 * A read thread to monitor module
 *
 * We need to create a read thread to monitor module's output.\n
 * The output should be directly pass to client via socket.\n
 * Besides, we alos need to consider the thread termination condition.\n
 * That's why I use the select function to read module. \n
 * \n
 * Before passing the data to user, we need to do some modification in process_buf functions.\n
 *
 * @return NULL
 */
void *read_thread(void *parameter)
{
	struct read_thread_param *param = parameter;
	int ret, n, socketfd, modulefd;
	struct timeval timeout;
	fd_set set, test_set;
	unsigned char buf[NETWORK_BUFFER_SIZE];

	if (!param->modulefd || !param->sockfd) {
		printf("READ thread: get wrong fd!\n");
		pthread_exit(NULL);
	}
	socketfd = param->sockfd;
	modulefd = param->modulefd;
	FD_ZERO(&set);
	FD_SET(modulefd, &set);
	while(!b_exit) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000 * 1000;
		test_set = set;
		ret = select(modulefd + 1, &test_set, NULL, NULL, &timeout);
		if(ret == -1)
			perror("read_thread:");
		else if(ret == 0) {
			/* timeout, do nothing */
		} else {
			n = read(modulefd, buf, NETWORK_BUFFER_SIZE);
			if (n < 0)
				perror("read module error");
			DEBUG("read %d bytes from module.\n", n);
			n = process_buf(buf, n);
			if (n > 0) {
				n = write_to_client(socketfd, buf, n);
				DEBUG("write %d bytes to socket.\n", n);
				if ( n < 0)
					perror("write to socket error.");
			}
		}
	}
	pthread_exit(NULL);
}

void module_enable_raw_data(int enable)
{
	if (raw_data_en != enable) {
		printf("Raw data enable=%d\n", enable);
		raw_data_en = enable;
	}
}

/**
 * Open the connection between module and CPU
 *
 * In our case, the connection port is UART. \n
 * Besides, we need to create a pthread to handle  \n
 * the notify information from module. \n
 * We don't know when the module will pass the notify \n
 * So we need to create a thread to monitor it. \n
 *
 * @param sockfd  a opened fd. All read data will be write to this fd.
 * @return the opened UART device fd number
 */
int open_module(int sockfd)
{
	int client_fd = sockfd;

	if (is_opened) {
		printf("Warnning!! open module twice!\n");
		close_module();
	}
	is_opened = true;
	fd = setup_uart();
	if (fd < 0) {
		printf("open uart failed!!\n");
		is_opened = false;
	}
	param.sockfd = sockfd;
	param.modulefd = fd;
	b_exit = false;
	pthread_create(&thread_id, NULL, (void *)*read_thread, &param);
	printf("open module!\n");
	return fd;
}

int close_module(void)
{
	printf("close module!\n");
	is_opened = false;
	b_exit = true;
	pthread_join(thread_id, NULL);
	close(fd);
}
