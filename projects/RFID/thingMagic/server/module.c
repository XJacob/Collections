/**
 * @file
 * @brief Raw module read/write
 *
 * All module raw read/write function should be defined \n
 * in this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>

#include "default_val.h"
#include "module.h"
#include "digital_board.h"

static int is_opened;
static int fd;
static int socket_fd;
static int b_exit;
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
	return n;
}

/**
 * check if module is opened.
 *
 * @return
 * 	 return 0 if module is not opened
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

/**
 * A read thread to monitor module
 *
 * We need to create a read thread to monitor module's output.\n
 * The output should be directly pass to client via socket.\n
 * Besides, we alos need to consider the thread termination condition.\n
 * That's why I use the select function to read module. \n
 * \n
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
		timeout.tv_usec = 100 * 1000;
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

			if (is_opened) {
				oneshot_scan_led(1);
				write_to_client(socketfd, buf, n);
			}
		}
	}
	pthread_exit(NULL);
}


// don't care the module uart speed. Just set uart speed to 921600
// Assume the uart speed is 115200 or 921600
static void force_setup_module_speed(int fd)
{
	//module command to set speed to 921600
	unsigned char speed_9216[] = {0xff, 0x04, 0x06, 0x00, 0x0e, 0x10, 0x00, 0x87, 0x8f};
	struct termios t;

	tcgetattr(fd, &t);
	cfsetispeed(&t, B115200);
	cfsetospeed(&t, B115200);
	tcsetattr(fd, TCSANOW, &t);

	write(fd, speed_9216, sizeof(speed_9216));

	tcgetattr(fd, &t);
	cfsetispeed(&t, B921600);
	cfsetospeed(&t, B921600);
	tcsetattr(fd, TCSANOW, &t);
	usleep(10 * 1000);
	printf("set baudrate to 921600!\n");
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
	if (is_opened) {
		//wait the previous connection closing...
		sleep(1);
		if (is_opened) {
			printf("Warnning!! open module twice!\n");
			close_module();
		}
	}

	fd = setup_uart();
	if (fd < 0) {
		printf("open uart failed!!\n");
		is_opened = false;
	}

	param.sockfd = sockfd;
	param.modulefd = fd;
	b_exit = false;
	pthread_create(&thread_id, NULL, (void *)*read_thread, &param);
	force_setup_module_speed(fd);
	is_opened = true;

	printf("open module!\n");
	return fd;
}

int close_module(void)
{
	b_exit = true;
	pthread_join(thread_id, NULL);
	is_opened = false;
	__set_ant_led(0,0,0,0,0);
	close(fd);
	printf("close module!\n");
}

/*
 * ThingMagic-mutated CRC used for messages.
 * Notably, not a CCITT CRC-16, though it looks close.
 */
static short crctable[] =
{
  0x0000, 0x1021, 0x2042, 0x3063,
  0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

static unsigned short tm_crc(unsigned char *u8Buf, unsigned char len)
{
  unsigned short crc;
  int i;

  crc = 0xffff;

  for (i = 0; i < len ; i++)
  {
    crc = ((crc << 4) | (u8Buf[i] >> 4))  ^ crctable[crc >> 12];
    crc = ((crc << 4) | (u8Buf[i] & 0xf)) ^ crctable[crc >> 12];
  }

  return crc;
}

static inline int is_CRC_wrong(unsigned char *buf, unsigned char len)
{
	unsigned short crc_bytes = buf[len-2] << 8 | buf[len-1];
	return crc_bytes != tm_crc(buf + 1, len - 3);
}

static void fill_header(unsigned char *data, unsigned char data_len,
		unsigned char op_code, unsigned short status)
{
	unsigned short crc;
	data[0] = 0xff;
	data[1] = data_len;
	data[2] = op_code;
	data[3] = (status & 0xff00) >> 8;
	data[4] = status & 0xff;
	crc = tm_crc(data + 1, data_len + 4);
	data[data_len + 5] = (crc & 0xff00) >> 8;
	data[data_len + 6] = crc & 0xff;
}

static int create_unheader_package(unsigned char **data, unsigned char op_code,
		unsigned char *buf, unsigned int buf_len)
{
	int data_len;
	unsigned char *tmp;
	unsigned char t1,t2;

	switch(op_code) {
		case CMD_GET_FW_VERSION:
			data_len = 1;

			*data = malloc(data_len + HEAD_LEN);
			tmp = *data;
			tmp[HEAD_LEN - 2] = __get_fw_version();
			break;
		case CMD_DEVICE_REBOOT:
			data_len = 0;

			*data = malloc(data_len + HEAD_LEN);
			__device_reboot();
			break;
		case CMD_EN_UPDATE:
			data_len = 0;

			*data = malloc(data_len + HEAD_LEN);
			__system_update();
			break;
		case CMD_SET_GPIO:
			data_len = -1;
			__set_gpio(buf[3], buf[4], buf[5], buf[6]);
			break;
		case CMD_GET_GPIO:
			data_len = 2;

			__get_gpio(&t1, &t2);
			*data = malloc(data_len + HEAD_LEN);
			tmp = *data;
			tmp[5] = t1;
			tmp[6] = t2;
			break;
		case CMD_MODULE_RESET:
			data_len = -1;
			__module_reset();
			break;
		case CMD_SET_NETWORK:
			data_len = 0;

			*data = malloc(data_len + HEAD_LEN);
			__set_system_ip(buf + 3, buf_len-5);
			break;
		case CMD_SET_LED:
			data_len = 0;
			__set_ant_led(buf[3], buf[4], buf[5], buf[6], buf[7]<<8 | buf[8]);

			*data = malloc(data_len + HEAD_LEN);
			break;

		/* Currently our server will use buadrate 921600 to communicate to module.
		 * If the baudrate is changed by client, this command should be called
		 * to make baudrate back to 921600 */
		case CMD_BAUDRATE_RPOBE:
			data_len = -1;
			force_setup_module_speed(fd);
			break;
	}
	return data_len;
}

/*
 * receive format:
 * 0xff data_len op_code data...  crc1 crc2
 *
 * sent format(twice):
 * 0xff data_len op_code status1 status2 crc1 crc2
 * data....
 *
 * */
static int handle_drc_command(int sockfd, unsigned char *buf, int buf_len)
{
	unsigned char data_len = buf[1];
	unsigned char op_code = buf[2];
	unsigned char *package;
	unsigned int pkg_len = HEAD_LEN;
	int len;

	if (buf_len < (data_len + 5) || is_CRC_wrong(buf, data_len + 5)) {
		// avoid client will occur external reset error, don't return the error status
		/*
		 *data_len = 0;
		 *package = malloc(HEAD_LEN);
		 *fill_header(package, 0, op_code, STATUS_WORNG_BITS);
		 */
		printf("Error! receive the wrong buffer.\n");
		return 0;
	} else {
		len = create_unheader_package(&package, op_code, buf, buf_len);
		// return -1 to reconnect server
		if (op_code==CMD_MODULE_RESET)
		  return -1;
		// if len < 0, don't ack anything
		if (len < 0)
		  return 0;

		fill_header(package, len, op_code, TM_SUCCESS);
		pkg_len += len;
	}

	write_to_client(sockfd, package, pkg_len);
	free(package);
	return 0;
}

static inline is_drc_need_ret_cmd(unsigned char opcode)
{
	switch(opcode) {
		case CMD_GET_FW_VERSION:
		case CMD_EN_UPDATE:
		case CMD_GET_GPIO:
		case CMD_SET_NETWORK:
		case CMD_SET_LED:
			return 1;
		default:
			return 0;
	}
}

static inline is_drc_noreturn_cmd(unsigned char opcode)
{
	switch(opcode) {
		case CMD_DEVICE_REBOOT:
		case CMD_SET_GPIO:
		case CMD_MODULE_RESET:
		case CMD_BAUDRATE_RPOBE:
			return 1;
		default:
			return 0;
	}
}

#define MIN_PACKAGE_SIZE 5
int handle_combined_drc_cmd(int sockfd, unsigned char *buf, int len)
{
	int i = 0;
	int tmp_opcode;
	int tmp_len;
	int final_buf_len = len;

	while(i <= (len - MIN_PACKAGE_SIZE)) {
		if (buf[i] == 0xff) { //start of package
			tmp_len = buf[i+1];
			tmp_opcode = buf[i+2];

			// if no return, handle it then remove it
			if (is_drc_noreturn_cmd(tmp_opcode)) {
				handle_drc_command(sockfd, buf + i, tmp_len + 5);
				memmove(buf + i, buf + i + tmp_len + 5, len-(i+tmp_len+5));
				final_buf_len -= tmp_len + 5;
			} else if (is_drc_need_ret_cmd(tmp_opcode)) {
				// drc command need return value, should not combine with other buffer
				return handle_drc_command(sockfd, buf + i, tmp_len + 5);
			}
			i += tmp_len + 5;
		} else {
			i++;
		}
	}
	return final_buf_len;
}

int handle_buf(int sockfd, unsigned char *buf, int len)
{
	int i;
	int modified_len;

	if (!buf || len < 3) {
		printf("got empty buffer!\n");
		return 0;
	}

	if (debug_flag)
		print_buf("Got buf::", buf, len);

	// handle if the drc command is combined with other commands
	// the buffer len may be changed after this function
	modified_len = handle_combined_drc_cmd(sockfd, buf, len);

	if (debug_flag && len != modified_len)
		print_buf("modified buf::", buf, modified_len);

	if (modified_len > 0)
		write_module(buf, modified_len);
	return modified_len;
}


