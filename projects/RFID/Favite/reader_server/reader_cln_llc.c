#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "default_val.h"
#include "favite_llc.h"
#include "reader_cln_llc.h"

static pthread_t thread_id;
static pthread_mutex_t tag_data_lock;
static int is_inventoried;
static int socketfd;
static struct __tag_list *head, *end;
static unsigned int list_len;
static unsigned int total_tag_num;
static unsigned int first_tag_time;
static unsigned int end_tag_time;
static time_t last_duptag_time;
static unsigned int checksum_err;
static int ant_count;

#define DELAY_TIME 500
#define INVENTORY_SIZE 3
#define THERMAL_DETECTION_SIZE 2
#define TID_CODE 6
#define EPC_CODE_LEN 12
//Read Module Register Message Code
#define GET_TEMPRATURE 0x02

static struct rfid_cmd inventory_start[INVENTORY_SIZE] = {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Operation Mode", {START, 0x00, 0x0E, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Run Tag Inventory", {START, 0x00, 0x50, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, FALSE}
};

static struct rfid_cmd read_TID_start[INVENTORY_SIZE] = {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Operation Mode", {START, 0x00, 0x0E, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Run Tag TID Inventory", {START, 0x00, 0xF3, 0x00, 0x08,0x02,0x04,0x07,0x01,0x00,0x00,0x00,0x00, CHECKSUM, STOP}, 1, DELAY_TIME, FALSE}
};

static struct rfid_cmd set_antenna_power = {
	"Set Antenna Port Power", {START, 0x00, 0x14, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x64, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE
};

static struct rfid_cmd set_write_tags = {
	"Set Write Tags EPC",{START,0x00,0x51,0x00,0x0A,0x01,0x00,0x02,0x00,0x00,0x00,0x00,0x07,0x00,0x00,CHECKSUM,STOP},1,DELAY_TIME, TRUE
};

static struct rfid_cmd enable_mask[5] = {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Toggle Target Disable(FixQ)", {START, 0x00, 0x22, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Toggle Target Disable(DynamicQ)", {START, 0x00, 0x28, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Selection Action Enable", {START, 0x00, 0x40, 0x00, 0x05, 0x01,0x02,0x00,0x00,0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE}
};

static struct rfid_cmd run_write_tags = {
	"Run Write Tags EPC Code", {START, 0x00, 0x52, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE
};

static struct rfid_cmd disable_mask[3] = {
	{"Set Toggle Target Enable(FixQ)", {START, 0x00, 0x22, 0x00, 0x01, 0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Toggle Target Enable(DynamicQ)", {START, 0x00, 0x28, 0x00, 0x01, 0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Selection Action Disable", {START, 0x00, 0x40, 0x00, 0x05, 0x01,0x02,0x00,0x00,0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE}
};

static struct rfid_cmd antenna_dwell_time = {
	"antenna dwell time", {START, 0x00, 0x15, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x58, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd set_write_tags_mask[2] = {
	{"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Set Selection Mask Command", {START, 0x00, 0x40, 0x00, 0x24,0x00,0x01,0x20,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE}
};

static struct rfid_cmd antenna_set_port = {
	"Set Antenna Port Status", {START, 0x00, 0x12, 0x00, 0x02, 0x00, 0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE
};

static struct rfid_cmd antenna_get_port = {
	"antenna get port", {START, 0x00, GET_ANTENNA_PORT, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd show_sw_version = {
	"show sw version", {START, 0x00, READ_SW_VERSION, 0x00, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd inventory_stop = {
	"Stop Operation", {START, 0x00, 0x5F, 0x00, 0x00, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE,
};

static struct rfid_cmd setup_ip = {
	"Set IP", {START, 0x00, SET_IP, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd raw_data = {
	"Raw data mode", {START, 0x00, RAW_DATA_MODE, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd debug = {
	"Enable debug", {START, 0x00, DEBUG_MODE, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd update = {
	"system update", {START, 0x00, SYSTEM_UPDATE, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd module_reset = {
	"module reset", {START, 0x00, MODULE_RESET, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd gpio_set = {
	"gpio set", {START, 0x00, SET_GPIO, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd gpio_get = {
	"gpio get", {START, 0x00, GET_GPIO, 0x00, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd system_reboot = {
	"system reboot", {START, 0x00, SYSTEM_REBOOT, 0x00, 0x01, 0x00, CHECKSUM, STOP}, 1, 0, TRUE
};

static struct rfid_cmd Get_Temp[THERMAL_DETECTION_SIZE] = {
	{"Enable Favite Debug CMD", {START, 0x00, 0xF0, 0x00, 0x01,0x01, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE},
	{"Get Module Temprature", {START, 0x00, 0x02, 0x00, 0x02, 0x0B, 0x06, CHECKSUM, STOP}, 1, DELAY_TIME, TRUE}
};

static struct rfid_cmd llc_test = {
	"llc test", {0}, 1, 0, TRUE
};


static void fill_checksum(unsigned char *data, int len)
{
	int j;

	if (!data)
		return;
	for (j=1; j<len-2; j++)
		data[len-2] ^= data[j];
}

void send_single_cmd(struct rfid_cmd cmd, int fd)
{
	unsigned int len, i, j;
	unsigned char *tmp;

	for(i=0; !cmd.repeat_times || i<cmd.repeat_times; i++) {
		len = CMD_LENGTH(cmd.data);
		tmp = malloc(len);
		memcpy(tmp, cmd.data, len);
		for (j=0; j<len; j++) {
			printf(" %02x ", tmp[j]);
		}
		fill_checksum(tmp, len);
		printf("\ncmd name: %s, length:%d data:\n", cmd.name, len);
		for (j=0; j<len; j++) {
			printf(" %02x ", tmp[j]);
		}
		fflush(stdout);
		write(fd, tmp, len);
		free(tmp);
		if (cmd.delay > 0)
			usleep(cmd.delay * 1000);
	}
}

void show_server_version(int sockfd)
{
	send_single_cmd(show_sw_version, sockfd);
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

static void free_tag(struct tag_data *tag)
{
	if (tag && tag->epc_block) {
		free(tag->epc_block);
	if (tag && tag->tid_block) {
		free(tag->tid_block);
	}
	 free(tag);
	}
}

/* tag list should be protected before calling this function */
static void show_tag_list_data(struct __tag_list *head)
{
	struct __tag_list *tmp = head;
	unsigned int rate, i, j;

	printf("\n==================================================\n");
	printf("Found [%d] / [%d] tags::(%d ms), ChecksumError:%d\n",
			list_len, total_tag_num, end_tag_time - first_tag_time, checksum_err);
	printf("[tag num][[antenna: counts]][nb_rssi] [pc] [crc] : [epc_block] [tid_block]\n");
	for(j=0; j<list_len; j++) {
		printf("[%d][[%d: %d]]",j, tmp->data->antenna_port, tmp->data->times);
		printf("[%d] : ", tmp->data->nb_rssi);
		for (i=0; i<2; i++)
			printf("[%x] ", tmp->data->pc_block[i]);
		for (i=0; i<2; i++)
			printf("[%x] ", tmp->data->crc_block[i]);
		printf("\n EPC :");
		for (i=0; i<tmp->data->epc_len; i++)
			printf(" %02x ", tmp->data->epc_block[i]);
		if (tmp && tmp->data->tid_block) {
			printf("\n TID :");
			for (i=0; i<tmp->data->tid_len; i++)
				printf(" %02x ", tmp->data->tid_block[i]);
		}
		tmp = tmp->next;
		printf("\n");
	}
	printf("==================================================\n");
	printf("\n");
}

static void clear_tag_database(void)
{
	struct __tag_list *tmp = head, *tmp1;
	unsigned int j;

	pthread_mutex_lock(&tag_data_lock);
	for(j=0; j<list_len; j++) {
		if (!tmp)
			break;
		tmp1 = tmp->next;
		free_tag(tmp->data);
		free(tmp);
		tmp = tmp1;
	}
	head = NULL;
	list_len = total_tag_num = end_tag_time = first_tag_time = checksum_err = 0;
	pthread_mutex_unlock(&tag_data_lock);
}

static void push_tag_to_database(struct tag_data *tag)
{
	struct __tag_list *tmp = malloc(sizeof(struct __tag_list));

	if (!tmp) {
		printf("tag list malloc failed!\n");
		return;
	}
	tmp->data = tag;
	pthread_mutex_lock(&tag_data_lock);
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
	pthread_mutex_unlock(&tag_data_lock);
}

// only check epc block
static int is_tag_different(struct tag_data *tag1, struct tag_data *tag2)
{
	int i = tag1->epc_len;

	if (i != tag2->epc_len) return 1;
	if (tag1->antenna_port != tag2->antenna_port) return 1;
	while(i > 0) {
		if (tag1->epc_block[i] != tag2->epc_block[i]) return 1;
		i--;
	}
	return 0;
}

//return 0 if it's new tag
static int handle_tag_duplicated(struct tag_data *tag)
{
	int i;
	struct __tag_list *tmp = head;
	time_t now_time = time(0);

	pthread_mutex_lock(&tag_data_lock);
	if (!head) {
		pthread_mutex_unlock(&tag_data_lock);
		return 0;
	}
	for(i=0; i<list_len; i++) {
		if(!is_tag_different(tag, tmp->data)) {
			tmp->data->times++;
			tmp->data->nb_rssi = tag->nb_rssi;
			if ((int)difftime(now_time, last_duptag_time) >= UPDATE_TAG_TIME_PERIOD) {
				show_tag_list_data(head);
				last_duptag_time = time(0);
			}
			pthread_mutex_unlock(&tag_data_lock);
			return 1;
		}
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&tag_data_lock);
	return 0;
}

/* boundary check before call this function */
static int process_tag_data(const unsigned char *buf, unsigned int len)
{
	struct tag_data *tag;

	tag = malloc(sizeof(struct tag_data));
	if (!tag) {
		printf("malloc tag failed!\n");
		return -1;
	}
	tag->times = 1;
	if (buf[2] == EPC_MC){
		int len_epc = len - 9 - 7 - 4;
		tag->epc_block = malloc(len_epc);
		if (!tag->epc_block) {
			printf("malloc failed!EPC size:%d\n", len_epc);
			return -1;
		}
		tag->antenna_port = buf[5]+1;
		tag->nb_rssi = buf[6] << 8 | buf[7];
		tag->wb_rssi = buf[8] << 8 | buf[9];
		tag->time_stamp = buf[10] << 24 | buf[11] << 16 | buf[12] << 8 | buf[13];
		memcpy(tag->pc_block, buf+14, 2);
		memcpy(tag->epc_block, buf+16, len_epc);
		memcpy(tag->crc_block, buf+16 + len_epc, 2);
		tag->tid_block = NULL;
		tag->epc_len = len_epc;
		tag->tid_len = 0;
		total_tag_num++;
	} else if(buf[2] == TID_MC){
		int TID_CODE_LEN = read_TID_start[INVENTORY_SIZE-1].data[TID_CODE]*2;
		tag->tid_block = malloc(TID_CODE_LEN);
		int len_epc = len - 5 - 7 - 4 - TID_CODE_LEN;
		tag->epc_block = malloc(EPC_CODE_LEN);
		if (!tag->epc_block) {
			printf("malloc failed!EPC size:%d\n", len_epc);
			return -1;
		}
		if (!tag->tid_block) {
			printf("malloc failed!TID size:%d\n", TID_CODE_LEN);
			return -1;
		}
		tag->antenna_port = buf[9]+1;
		tag->nb_rssi = 0;
		tag->wb_rssi = 0;
		tag->time_stamp =  0;
		memcpy(tag->pc_block, buf+10, 2);
		memcpy(tag->epc_block, buf+12, EPC_CODE_LEN);
		memcpy(tag->crc_block, buf+12+ EPC_CODE_LEN, 2);
		if (buf[4] == 9 + EPC_CODE_LEN + TID_CODE_LEN ){
			memcpy(tag->tid_block, buf+14+ EPC_CODE_LEN, TID_CODE_LEN);
		}else {
			TID_CODE_LEN = 0;
			tag->tid_block = NULL;
		}
		tag->epc_len = EPC_CODE_LEN;
		tag->tid_len = TID_CODE_LEN;
		total_tag_num++;
	}
	end_tag_time = tag->time_stamp;
	if (!handle_tag_duplicated(tag))
		push_tag_to_database(tag); //new tag
	else
		free_tag(tag);
}

static void handle_buffer(const unsigned char *buf, int len)
{
	if (is_packet_correct(buf, len)) {
		if (buf[1] == TYPE_RET) {
			switch(buf[2])
			{
				case READ_SW_VERSION:
					printf("\nServer version: %d.%d  FW version:%d\n", buf[5], buf[6], buf[7]);
					break;
				case GET_ANTENNA_PORT:
					printf("\nantenna port %d is %d\n", ant_count, buf[7]);
					ant_count++;
					break;
				case GET_GPIO:
					printf("\nInput gpio status: %d  %d\n", buf[5], buf[6]);
					break;
				case GET_TEMPRATURE:
					printf("\nFavite Module Temperature is %d.\n",buf[10]);
				default:
					print_buf("\nResponse: ", buf, len);
					break;
			}
		} else if (buf[1] == TYPE_NOF) {
			if (buf[2] != 82)
				process_tag_data(buf, len);
			//	print_buf("\nNotification: ", buf, len);
		} else {
			print_buf("\nRaw packet: ", buf, len);
		}
	} else {
		checksum_err++;
		print_buf("\nRaw packet: ", buf, len);
	}
}

static int process_buf(unsigned char *buf, int len)
{
	unsigned int cur = 0, j, cmd_len, buf_len = len;

	while (buf_len && (buf_len - cur) > 6) {
		if (buf[cur] == PREAMBLE) {
			cmd_len = CMD_LENGTH((buf + cur));
			if ((cur + cmd_len) > buf_len)
				break;
			handle_buffer(buf + cur, cmd_len);
			cur += cmd_len;
		} else {
			cur = find_next_start(buf, cur, buf_len);
			if (!cur) {
				break;
			}
		}
	}
	return buf_len;
}

void start_inventory(int fd)
{
	int i;

	if (is_inventoried) {
		stop_inventory(fd);
		is_inventoried = false;
		printf("Inventory is running! Will stop it!!\n");
	}
	for(i=0; i < INVENTORY_SIZE; i++)
		send_single_cmd(inventory_start[i], fd);
	is_inventoried = true;
}

void start_read_tid(int fd)
{
	int i;

	if (is_inventoried) {
		stop_inventory(fd);
		is_inventoried = false;
		printf("Read TID is running! Will stop it!!\n");
	}
	for(i=0; i < INVENTORY_SIZE; i++)
		send_single_cmd(read_TID_start[i], fd);
	is_inventoried = true;
}

void __antenna_set_power(int fd, int *port, int power)
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

void __set_epc_mask(int fd, int bank, int count, int mask[41])
{
	int i ;

	set_write_tags_mask[1].data[6] = (bank & 0xff);
	set_write_tags_mask[1].data[8] = ((count*4) & 0xff);
	for (i=1; i<=count; i++){
		set_write_tags_mask[1].data[9+2*(i-1)] = (mask[2*(i-1)] & 0xff);
		set_write_tags_mask[1].data[10+2*(i-1)] = (mask[2*(i-1)+1] & 0xff);
	}
	for(i=0; i < 2; i++)
		send_single_cmd(set_write_tags_mask[i], fd);
};
void __run_write_tags(int fd, int mask_sw, int start, int end, int epc[41])
{
	int i,j;

	if (mask_sw==1){
		for (i=0;i<5;i++)
			send_single_cmd(enable_mask[i],fd);
	}
	for (j=start;j<=end;j++){
		set_write_tags.data[7] = (j & 0xff);
		set_write_tags.data[13] = epc[2*(j-1)];
		set_write_tags.data[14] = epc[2*(j-1)+1];
		send_single_cmd(set_write_tags,fd);
		send_single_cmd(run_write_tags,fd);
	}
	if (mask_sw==1){
		for (i=0;i<3;i++)
			send_single_cmd(disable_mask[i],fd);
	}
}

void stop_inventory(int fd)
{
	send_single_cmd(inventory_stop, fd);
	is_inventoried = false;
}

void read_thread(void *data)
{
	unsigned char buf[NETWORK_BUFFER_SIZE];
	int socketfd = *(int *)data;
	int n;

	while(1) {
		n = read(socketfd, buf, NETWORK_BUFFER_SIZE);
		if (n < 0)
			perror("Read error");
		else
			process_buf(buf, n);
	}
}

void create_read_thread(int fd)
{
	socketfd = fd;
	last_duptag_time = time(0);
	if (pthread_mutex_init(&tag_data_lock, NULL) != 0) {
		printf("thread mutex tag data init failed!!");
	}
	pthread_create(&thread_id, NULL, (void *)*read_thread, &socketfd);
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

void __antenna_get_port(int fd)
{
	int ant_num=0;

	ant_count=1;
	for (ant_num=0;ant_num<4;ant_num++){
		antenna_get_port.data[5] = ant_num;
		send_single_cmd(antenna_get_port, fd);
	}
}

void __set_ip_address(int fd,  struct in_addr *dest_ip, struct in_addr *dest_mask, struct in_addr *dest_gateway)
{
	unsigned int tmp = dest_ip->s_addr;
    //ip address
	setup_ip.data[5] = tmp & 0xff;
	setup_ip.data[6] = (tmp & 0xff00) >> 8;
	setup_ip.data[7] = (tmp & 0xff0000) >> 16;
	setup_ip.data[8] = (tmp & 0xff000000) >> 24;
	//netmask
	tmp = dest_mask->s_addr;
	setup_ip.data[9] = tmp & 0xff;
	setup_ip.data[10] = (tmp & 0xff00) >> 8;
	setup_ip.data[11] = (tmp & 0xff0000) >> 16;
	setup_ip.data[12] = (tmp & 0xff000000) >> 24;
    //gateway
	tmp = dest_gateway->s_addr;
 	setup_ip.data[13] = tmp & 0xff;
	setup_ip.data[14] = (tmp & 0xff00) >> 8;
	setup_ip.data[15] = (tmp & 0xff0000) >> 16;
	setup_ip.data[16] = (tmp & 0xff000000) >> 24;
	send_single_cmd(setup_ip, fd);
}

void __set_raw_mode(int fd, int enable)
{
	raw_data.data[5] = enable;
	send_single_cmd(raw_data, fd);
}

void __clear_tag_data(void)
{
	clear_tag_database();
	printf("Clear the tag data!!!!!\n");
}

void __set_debug_msg(int fd, int enable)
{
	debug.data[5] = enable;
	send_single_cmd(debug, fd);
}

void __set_system_update(int fd)
{
	send_single_cmd(update, fd);
}

void __set_module_reset(int fd)
{
	send_single_cmd(module_reset, fd);
}

void __set_system_reboot(int fd)
{
	send_single_cmd(system_reboot, fd);
}

void __thermal_detection(int fd)
{
	int i;

	for(i=0; i < THERMAL_DETECTION_SIZE; i++)
		send_single_cmd(Get_Temp[i], fd);
}

void __llc_test(int fd,unsigned char cmd_data[100],int data_len)
{
	memcpy(llc_test.data,cmd_data,data_len);
	send_single_cmd(llc_test, fd);
}

void __set_gpio(int fd, unsigned char gpio1, unsigned char gpio2, unsigned char gpio3, unsigned char gpio4)
{
	gpio_set.data[5] = !!gpio1;
	gpio_set.data[6] = !!gpio2;
	gpio_set.data[7] = !!gpio3;
	gpio_set.data[8] = !!gpio4;
	send_single_cmd(gpio_set, fd);
}

void __get_gpio(int fd)
{
	send_single_cmd(gpio_get, fd);
}
