/**
 * @file
 * @brief RFID reader handle LLC command
 *
 * All the LLC command will be processed in this file. \n
 * We folllow FAVITE's rule to create our own LLC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "default_val.h"
#include "favite_llc.h"

/**
 * Package the payload to be FAVITE LLC format
 *
 * @param sockfd client's fd
 * @param code  LLC Message Code
 * @param payload_len the length of payload
 * @param payload unsigned char buffer of payload
 *
 * @return the number of wrote bytes
 */
static int package_data_to_client(int sockfd, unsigned char code,
		unsigned short payload_len, unsigned char *payload)
{
	int len, ret;
	unsigned char *buf;

	len = payload_len + 7; // start_bit + header(4 bytes) + checksum + end
	buf = malloc(len);
	buf[0] = PREAMBLE;
	buf[1] = TYPE_RET;
	buf[2] = code;
	buf[3] = (payload_len & 0xff00) >> 8;
	buf[4] = payload_len & 0xff;
	if (payload_len)
		memcpy(&buf[5], payload, payload_len);
	buf[len-2] = calc_checksum(buf, len);
	buf[len-1] = END_MARK;
	ret = write_to_client(sockfd, buf, len);
	free(buf);
	return ret;
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

/**
 * Write the server software version to client
 *
 * The software version will package to LLC format before \n
 * sending to client
 *
 * @param sockfd client's fd
 */
static void return_sw_version(int sockfd)
{
	unsigned char payload[3] = {SERVER_VER_MAJOR, SERVER_VER_MINOR, 0};
	unsigned char tmp[16];
	int ret;

	if (find_value_from_conf(SYS_CONFIG_FILE, "Firmware", tmp, 16)) {
		printf("can't find system config file!\n");
	}
	sscanf(tmp, "%d", payload+2);
	ret = package_data_to_client(sockfd, READ_SW_VERSION,
			sizeof(payload) / sizeof(unsigned char), payload);
	if (ret <=0 )
		printf("return sw version error!!\n");
	printf("Return software version %d.%d, FW version: %d\n",
		payload[0], payload[1], payload[2]);
}

static void enable_raw_data_mode(int sockfd, const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;

	if (buf[LOAD_OFFSET] == 1)
		module_enable_raw_data(1);
	else
		module_enable_raw_data(0);
	package_data_to_client(sockfd, RAW_DATA_MODE, 1, &ok);
}

static void module_reset(int sockfd, const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;

	__module_reset();
	package_data_to_client(sockfd, MODULE_RESET, 1, &ok);
}

static void system_reboot(int sockfd, const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;

	package_data_to_client(sockfd, SYSTEM_REBOOT, 1, &ok);
	printf("Client request, System will reboot.....\n");
	system("reboot");
}
static void system_update(int sockfd, const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;

	package_data_to_client(sockfd, RAW_DATA_MODE, 1, &ok);
	printf("Starting to run system update flow.....\n");
	system("/home/root/bin/update.sh");
}

static void set_debug_mode(int sockfd, const unsigned char *buf, int len)
{
	debug_flag = buf[LOAD_OFFSET];
	printf("debug_flag == %d\n", debug_flag);
}

static void set_gpio(int sockfd, const unsigned char *buf, int len)
{
	unsigned char payload[4];

	__set_gpio(buf[LOAD_OFFSET], buf[LOAD_OFFSET+1], buf[LOAD_OFFSET+2], buf[LOAD_OFFSET+3]);
	payload[0] = buf[LOAD_OFFSET];
	payload[1] = buf[LOAD_OFFSET+1];
	payload[2] = buf[LOAD_OFFSET+2];
	payload[3] = buf[LOAD_OFFSET+3];
	package_data_to_client(sockfd, SET_GPIO, sizeof(payload), payload);
}

static void get_gpio(int sockfd, const unsigned char *buf, int len)
{
	unsigned char payload[2];

	__get_gpio(payload, payload+1);
	package_data_to_client(sockfd, GET_GPIO, sizeof(payload), payload);
}

static void set_system_ip(int sockfd, const unsigned char *buf, int len)
{
	unsigned char ok = 0x00;
	struct in_addr dest_ip, dest_mask, dest_gateway;
	struct stat st = {0};
	unsigned char tmp[30]={0}, tmp_mask[30]={0}, tmp_gateway[30]={0};
	int fd, str_ip, str_mask, str_gateway;

	package_data_to_client(sockfd, SET_IP, 1, &ok);
	dest_ip.s_addr = buf[LOAD_OFFSET] | buf[LOAD_OFFSET+1] << 8
		| buf[LOAD_OFFSET+2] << 16 | buf[LOAD_OFFSET+3] << 24;
	dest_mask.s_addr = buf[LOAD_OFFSET+4] | buf[LOAD_OFFSET+5] << 8
		| buf[LOAD_OFFSET+6] << 16 | buf[LOAD_OFFSET+7] << 24;
	dest_gateway.s_addr = buf[LOAD_OFFSET+8] | buf[LOAD_OFFSET+9] << 8
		| buf[LOAD_OFFSET+10] << 16 | buf[LOAD_OFFSET+11] << 24;
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
	printf("System Reboot!");
	system("reboot");
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

/**
 * Handle the buffer sent from client
 *
 * We need to analyze the LLC command to identify if we \n
 * need to process the command or just pass it to FAVITE module
 *
 * @param sockfd client's fd
 * @param buf    the buffer sent from client
 * @param len    the buffer len
 *
 * @return
 * 		return 0 means success
 */
static int handle_buf(int sockfd, unsigned char *buf, int len)
{
	int i;

	if (!buf) {
		printf("got empty buffer!\n");
		return 0;
	}
	if (debug_flag)
		print_buf("Got buf::", buf, len);
	if (is_packet_correct(buf, len)) {
		switch(buf[CODE_OFFSET])
		{
			case READ_SW_VERSION:
				return_sw_version(sockfd);
				break;
			case SET_IP:
				set_system_ip(sockfd, buf, len);
				break;
			case RAW_DATA_MODE:
				enable_raw_data_mode(sockfd, buf, len);
				break;
			case DEBUG_MODE:
				set_debug_mode(sockfd, buf, len);
				break;
			case SYSTEM_UPDATE:
				system_update(sockfd, buf, len);
				break;
			case MODULE_RESET:
				module_reset(sockfd, buf, len);
				break;
			case SYSTEM_REBOOT:
				system_reboot(sockfd, buf, len);
				break;
			case SET_GPIO:
				set_gpio(sockfd, buf, len);
				break;
			case GET_GPIO:
				get_gpio(sockfd, buf, len);
				break;
			default:
				write_module(buf, len);
				break;
		}
	} else
		print_buf("Wrong packet data", buf, len);
	return 0;
}

int process_buf_svr(int sockfd, unsigned char *buf, int len)
{
	unsigned int cur = 0, j, cmd_len, buf_len = len;

	while (buf_len && (buf_len - cur) > 6) {
		if (buf[cur] == PREAMBLE) {
			cmd_len = CMD_LENGTH((buf + cur));
			if ((cur + cmd_len) > buf_len)
				break;
			handle_buf(sockfd, buf + cur, cmd_len);
			cur += cmd_len;
			usleep(10 * 1000);
		} else {
			cur = find_next_start(buf, cur, buf_len);
			if (!cur) {
				break;
			}
		}
	}
	return buf_len;
}
