#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#include "reader.h"
#include "antenna.h"
#define UART  "/dev/ttymxc2"

//#define DEBUG_ON_PC
#define DELAY_TIME 0  //ms
#define REC_BUF_SIZE 4 * 1024
#define TIME_PERIOD 1

#define APP_VERSION  4

#define INVENTORY_SIZE 5
#define INV_POWER_OFFSET_ROW 2
#define INV_POWER_OFFSET_COL 8
static struct rfid_cmd Inventory[INVENTORY_SIZE]= {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Operation Mode", {START, 0x00, 0x0E, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Run Tag Inventory", {START, 0x00, 0x50, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, FALSE},
	{"Waiting read", {WAIT_READ}, 0, DELAY_TIME, TRUE},
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
};

#define INVENTORYNONCON_SIZE 4
#define INV_DELAY_ROW 5
static struct rfid_cmd InventoryNONCON[INVENTORY_SIZE]= {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Operation Mode", {START, 0x00, 0x0E, 0x00, 0x01, 0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Run Tag Inventory", {START, 0x00, 0x50, 0x00, 0x00, CHECKSUM, STOP}, 0, DELAY_TIME, TRUE},
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
};

static struct rfid_cmd antenna_dwell_time = {
	"antenna dwell time", {START, 0x00, 0x15, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x58, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd set_antenna_power = {
	"Set Antenna Port Power", {START, 0x00, 0x14, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x64, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE
};

static struct rfid_cmd antenna_set_port = {
	"Set Antenna Port Status", {START, 0x00, 0x12, 0x00, 0x02, 0x00, 0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE
};
static struct rfid_cmd StopOperation[1]= {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
};

static struct __tag_list *head, *end;
static unsigned int list_len;
static unsigned int total_tag_num;
static unsigned int first_tag_time;
static unsigned int end_tag_time;
static unsigned int do_process_data;
static unsigned int verbose;
static unsigned int antenna_port;
static time_t last_duptag_time;

unsigned int find_next_start(unsigned char *buf, unsigned int pos, unsigned int end)
{
	if ((pos > end) || ((pos - end) < 2)) {
		printf("process data error, position:%d\n", pos);
		return 0;
	}

	while(pos < end) {
		if (buf[pos] == STOP && buf[pos+1] == START)
			return pos+1;
		else
			pos++;
	}
	printf("failed to find start byte!!\n");
	return 0;
}

/* boundary check before call this function */
void output_reponse_data(unsigned char *buf, unsigned int len)
{
	unsigned int i;
	printf("response::");
	for (i=0; i<len; i++)
		printf(" %02x ", buf[i]);
	printf("\n");
}

int is_tag_notify(unsigned char *buf)
{
	return buf[0] == START && buf[1] == 0x02 && buf[2] == 0x50;
}

void show_tag_list_data(struct __tag_list *head)
{
	struct __tag_list *tmp = head;
	unsigned int rate, i, j;

	printf("\n==================================================\n");
	printf("Found [%d] / [%d] tags::(%d ms)\n",
			list_len, total_tag_num, end_tag_time - first_tag_time);

	if (verbose) {
		printf("[tag num][[antenna: counts]][nb_rssi] : [epc_block]\n");
		for(j=0; j<list_len; j++) {
			printf("[%d][[%d: %d]]",j, tmp->data->antenna_port, tmp->data->times);
			printf("[%d] : ", tmp->data->nb_rssi);
			for (i=0; i<tmp->data->len; i++)
				printf(" %02x ", tmp->data->epc_block[i]);
			tmp = tmp->next;
			printf("\n");
		}
	}
	printf("==================================================\n");
	printf("\n");
	fflush(stdout);
}


void push_tag_to_database(struct tag_data *tag)
{
	struct __tag_list *tmp = malloc(sizeof(struct __tag_list));

	if (!tmp) {
		printf("tag list malloc failed!\n");
		return;
	}

	tmp->data = tag;

	if (!head) {
		head = tmp;
		end = tmp;
		first_tag_time = tag->time_stamp;
	} else {
		end->next = tmp;
		end = tmp;
	}

	list_len++;
	show_tag_list_data(head);
}

void free_tag(struct tag_data *tag)
{
	if (tag && tag->epc_block) {
		free(tag->epc_block);
		free(tag);
	}
}

// only check epc block
int is_tag_different(struct tag_data *tag1, struct tag_data *tag2)
{
	int i = tag1->len;
	if (i != tag2->len) return 1;
	if (tag1->antenna_port != tag2->antenna_port) return 1;
	while(i > 0) {
		if (tag1->epc_block[i] != tag2->epc_block[i]) return 1;
		i--;
	}
	return 0;
}

//return 0 if it's new tag
int handle_tag_duplicated(struct tag_data *tag)
{
	int i;
	struct __tag_list *tmp = head;
	time_t now_time = time(0);

	for(i=0; i<list_len; i++) {
		if(!is_tag_different(tag, tmp->data)) {
			tmp->data->times++;
			tmp->data->nb_rssi = tag->nb_rssi;
			if ((int)difftime(now_time, last_duptag_time) >= TIME_PERIOD) {
				show_tag_list_data(head);
				last_duptag_time = time(0);
			}
			return 1;
		}
		tmp = tmp->next;
	}
	return 0;
}

/* boundary check before call this function */
int process_tag_data(unsigned char *buf, unsigned int len)
{
	struct tag_data *tag;
	int epc_len = len - 9 - 7;

	tag = malloc(sizeof(struct tag_data));
	if (!tag) {
		printf("malloc tag failed!\n");
		return -1;
	}
	tag->antenna_port = antenna_port;
	tag->nb_rssi = buf[6] << 8 | buf[7];
	tag->wb_rssi = buf[8] << 8 | buf[9];
	tag->time_stamp = buf[10] << 24 | buf[11] << 16 | buf[12] << 8 | buf[13];
	tag->epc_block = malloc(epc_len);
	memcpy(tag->epc_block, buf+14, epc_len);
	tag->times = 1;
	tag->len = epc_len;

	/*
	 *printf("tag ant:%d nb_rssi:%x wb_rssi:%x time_stamp:%x\n",
	 *        tag->antenna_port, tag->nb_rssi, tag->wb_rssi, tag->time_stamp);
	 */

	if (!tag->epc_block) {
		printf("malloc failed! size:%d\n", epc_len);
		return -1;
	}

	total_tag_num++;
	end_tag_time = tag->time_stamp;

	if (!handle_tag_duplicated(tag))
		push_tag_to_database(tag); //new tag
	else
		free_tag(tag);
}

void process_response_data(unsigned char *buf, unsigned int end)
{
	unsigned int cur = 0, j, cmd_len, buf_len = end;
	unsigned char *tmp_buf;
	int ret, drop_buf = 0;

	if (!buf) {
		printf("receive null pointer!");
		return;
	}

	// avoid antenna port confused, give up this data

	tmp_buf = buf;

	while ((buf_len - cur) > 8) {
		if (tmp_buf[cur] == START) {
			cmd_len = CMD_LENGTH((tmp_buf + cur));
			if ((cur + cmd_len) > buf_len)
				break;

			if (is_tag_notify(tmp_buf + cur)) {
				if (!drop_buf)
					process_tag_data(tmp_buf + cur, cmd_len);
			} else
				output_reponse_data(tmp_buf + cur, cmd_len);
			cur += cmd_len;
		} else {
			cur = find_next_start(tmp_buf, cur, buf_len);
		}
	}
}

int setup_uart(char *uart)
{
	int fd;
	struct termios options;

	fd = open(UART, O_RDWR | O_NOCTTY);
	if (fd <= 0)
		return -1;

	tcgetattr(fd, &options);
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	cfmakeraw(&options);
	options.c_cflag |= (CLOCAL | CREAD);   // Enable the receiver and set local mode
	options.c_cflag &= ~CSTOPB;            // 1 stop bit
	options.c_cflag &= ~CRTSCTS;           // Disable hardware flow control
	options.c_iflag &= ~(IXON | IXOFF | IXANY );
	options.c_cflag |= CS8;
	options.c_cc[VTIME]=5;  //0.1 * 5 = 0.5s timeout
	options.c_cc[VMIN]=255; //buffer size 256 bytes
	tcsetattr(fd, TCSANOW, &options);
	return fd;
}

int get_response(int fd)
{
	int bytes, j;
	unsigned char buf[REC_BUF_SIZE];

    bytes = read(fd, buf, REC_BUF_SIZE);
	if (!do_process_data)
		printf("\nreceive %d bytes:\n", bytes);

	if (do_process_data)
		process_response_data(buf, bytes);
	else {
		for(j=0; j < bytes; j++) {
			printf(" %02x ", buf[j]);
			if(buf[j] == 0x7E)
				printf("\n");
		}
	}
	fflush(stdout);
}

void fill_checksum(unsigned char *data, int len)
{
	int j;
	for (j=1; j<len-2; j++)
		data[len-2] ^= data[j];
}

void send_single_cmd(struct rfid_cmd cmd, int fd)
{
	unsigned int len, i, j;
	unsigned char *tmp;

	for(i=0; !cmd.repeat_times || i<cmd.repeat_times; i++) {
		if (cmd.data[0] != WAIT_READ) {
			len = CMD_LENGTH(cmd.data);
			tmp = malloc(len);
			memcpy(tmp, cmd.data, len);
			fill_checksum(tmp, len);

			printf("\ncmd name: %s, length:%d data:\n", cmd.name, len);
			for (j=0; j<len; j++) {
				printf(" %02x ", tmp[j]);
			}
#ifndef DEBUG_ON_PC
			write(fd, tmp, len);
#endif
			free(tmp);
		}

		if (cmd.delay > 0)
			usleep(cmd.delay * 1000);
		if (cmd.need_response)
			get_response(fd);
	}
}

void send_cmds(struct rfid_cmd *cmds, int size, int fd)
{
	int i;
	for(i=0; i < size; i++) {
		send_single_cmd(cmds[i], fd);
		printf("\n");
	}
}


void send_inventory_cmd(int fd, int noncontinuous)
{
	int tmp;
	char buf[10];
	pthread_t thread_id;
	struct antenna_info ant_info;

	printf("set power value(dec)[30db]:");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%d\n", &tmp);
	if (tmp <= 0 || tmp > 320)
		tmp = 300;
	printf("Will set power to %d.%d db\n\n", tmp/10, tmp%10);

#ifndef DEBUG_ON_PC
	Inventory[INV_POWER_OFFSET_ROW].data[INV_POWER_OFFSET_COL] = (tmp & 0xff00) >> 8;
	Inventory[INV_POWER_OFFSET_ROW].data[INV_POWER_OFFSET_COL+1] = (tmp & 0xff);
#endif

	if (!noncontinuous) {
		printf("process data(y/n)[n]: ");
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		if (buf[0] == 'y')
			do_process_data = TRUE;
		else {
			do_process_data = FALSE;
			printf("n\n");
		}

		if (do_process_data) {
			verbose = 1;
		}
	} else {
		do_process_data = FALSE;
		printf("set inventory delay time(ms): ");
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		sscanf(buf, "%d\n", &tmp);
		printf("Will set delay time to %d ms\n\n", tmp);

#ifndef DEBUG_ON_PC
		InventoryNONCON[INV_DELAY_ROW].delay = tmp;
#endif
	}

	if (!noncontinuous)
		send_cmds(Inventory, INVENTORY_SIZE, fd);
	else
		send_cmds(InventoryNONCON, INVENTORYNONCON_SIZE, fd);

}

void __antenna_set_port(int fd, int *port, int enable)
{
	int i;

	antenna_set_port.data[6] = enable;
	for (i=0;i<4;i++){
			if (port[i] <5 && port[i] >0){
						antenna_set_port.data[5] = port[i]-1;
						send_single_cmd(antenna_set_port, fd);
					}
			else
				break;
		}
}

static void __antenna_set_power(int fd, int *port, int power)
{
	int i;

	set_antenna_power.data[8] = (power & 0xff00) >> 8;
	set_antenna_power.data[9] = (power & 0xff);
	for (i=0;i<4;i++){
			if (port[i] <5 && port[i] >0){
						set_antenna_power.data[5] = port[i]-1;
						send_single_cmd(set_antenna_power, fd);
					}
			else
				break;
		}
}

void __antenna_dwell_time(int fd, int time)
{
	int i;

	antenna_dwell_time.data[6] = (time & 0xff000000) >> 24;
	antenna_dwell_time.data[7] = (time & 0xff0000) >> 16;
	antenna_dwell_time.data[8] = (time & 0xff00) >> 8;
	antenna_dwell_time.data[9] = (time & 0xff);
	for (i=0;i<4;i++){
			antenna_dwell_time.data[5]=i;
			send_single_cmd(antenna_dwell_time, fd);
			usleep(100000);
		}
}

static void set_dwell_time(int fd)
{
	char inp[10];
	int tmp = 600;

	printf("set antenna dwell time[600ms]:");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if (tmp < 0 || tmp > 20000)
		tmp = 600;
	printf("Will set dwell time to %dms\n\n", tmp);
	__antenna_dwell_time(fd, tmp);
}

static void set_antenna(fd)
{
	char inp[10];
	int i, tmp, input_num, *ant, ant_cnt, enable;

	printf("Set antenna port (1234):");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	input_num = strlen(inp)-1;
	ant = malloc(sizeof(unsigned char)*input_num);
	for(i=0; i<input_num; i++) {
		ant[i] = tmp & 0xf;
		if (ant[i] < 1 || ant[i] > 4) {
			printf("Error:Incorrect antenna port!");
			free(ant);
			return;
		}
		ant_cnt++;
		tmp >>= 4;
	}
	printf("Set Antenna port (enable:1/disable:0) [0]:");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &enable);
	if (enable != 0 && enable != 1 )
		return;
	__antenna_set_port(fd, ant, enable);
}

static int set_power_ui(int fd)
{
	char inp[10];
	int i, tmp, tmp1, *ant, input_num, ant_cnt;

	printf("Set antenna port:");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	input_num = strlen(inp)-1;
	ant = malloc(sizeof(unsigned char)*input_num);
	for(i=0; i<input_num; i++) {
			ant[i] = tmp & 0xf;
			if (ant[i] < 1 || ant[i] > 4) {
						printf("Error:Incorrect antenna port!");
						free(ant);
						return -1;
					}
			ant_cnt++;
			tmp >>= 4;
		}
	printf("Set power value (dec) [30dBm]:");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp1);
	if (tmp1 <0 || tmp1>320)
		tmp1=300;
	printf("Will set power to %d.%d dBm\n",tmp1/10,tmp1%10);
	__antenna_set_power(fd, ant, tmp1);
}


void send_stop_cmd(int fd)
{
	send_cmds(StopOperation, 1, fd);
	usleep(200 * 1000);
}

char main_menu(int fd)
{
	int ret = 0, val;
	char inp[10];

	printf("RFID reader V%d.\n", APP_VERSION);

	send_stop_cmd(fd);

	while (ret != -1) {
		printf("input commands:\n");
		printf("[%d] INVENTORY_CMD\n", INVENTORY_CMD_ID);
		printf("[%d] Stop INVENTORY_CMD\n", STOP_INVENTORY_CMD_ID);
		printf("[%d] Set Antenna port\n", SET_ANTENNA_ID);
		printf("[%d] Set Antenna port Power\n", SET_ANTENNA_POWER);
		printf("[%d] Set dwell time\n",SET_DWELL_TIME );
		printf("[%d] Inventory nonContinuous mode\n", INVENTORY_NONCON_ID);
		printf("[%d] exit\n", EXIT_AP);

		fflush(stdout);
		fgets(inp, sizeof(inp), stdin);
		sscanf(inp, "%d\n",&ret);
		switch(ret) {
			case INVENTORY_CMD_ID:
				send_inventory_cmd(fd, 0);
				break;
			case STOP_INVENTORY_CMD_ID:
				send_stop_cmd(fd);
				break;
			case SET_ANTENNA_ID:
				set_antenna(fd);
				break;
			case SET_ANTENNA_POWER:
				set_power_ui(fd);
				break;
			case SET_DWELL_TIME:
				set_dwell_time(fd);
				break;
			case INVENTORY_NONCON_ID:
				send_inventory_cmd(fd, 1);
				break;
			case EXIT_AP:
				printf("exit AP");
				ret = -1;
				break;
			default:
				ret = 0;
		}
		printf("\n");
	}
	return ret;
}

void main()
{
#ifdef DEBUG_ON_PC
	int fd = -1;
#else
	int fd = setup_uart(UART);
#endif
	char input;

	main_menu(fd);

#ifdef DEBUG_ON_PC
	close(fd);
#endif
}
