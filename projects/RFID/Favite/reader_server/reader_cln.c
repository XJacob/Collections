#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "default_val.h"
#include "favite_llc.h"
#include "reader_cln_llc.h"

//module main function
#define START_INVENTORY    1
#define STOP_INVENTORY     2
#define SET_POWER          3
#define ENABLE_ANTENNA     4
#define GET_ANTENNA        5
#define SET_DWELL_TIME     6
//module minor function
#define START_READ_TID     10
#define SET_EPC_MASK       11
#define RUN_WRITE_TAGS     12
#define THERMAL_DETECTION  13
#define LLC_TEST           14
//reader function
#define GPIO_OUT           20
#define GPIO_IN            21
#define SET_IP_ADDRESS     22
#define RAW_DATA           23
#define CLEAR_DATA         24
#define DEBUG_MSG          25
#define SERVER_VERSION     26
#define FUNC_SYSTEM_UPDATE 27
#define RFID_MODULE_RESET  28
#define RFID_SYSTEM_REBOOT 29

#define EXIT_AP            30

static void inventory_ui(int sockfd)
{
	start_inventory(sockfd);
}

static void read_tid_ui(int sockfd)
{
	start_read_tid(sockfd);
}

static int set_power_ui(int sockfd)
{
	char inp[10];
	int i, tmp, tmp1, *ant, input_num, ant_cnt;

	printf("Set antenna port:");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	input_num = strlen(inp)-1;
	ant = malloc(sizeof(uint8_t)*input_num);
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
	__antenna_set_power(sockfd, ant, tmp1);
}

static void set_dwell_time(int sockfd)
{
	char inp[10];
	int tmp = 600;

	printf("set antenna dwell time[600ms]:");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if (tmp < 0 || tmp > 20000)
		tmp = 600;
	printf("Will set dwell time to %dms\n\n", tmp);
	__antenna_dwell_time(sockfd, tmp);
}

static int set_antenna(int sockfd)
{
	char inp[10];
	int i, tmp, input_num, *ant, ant_cnt, enable;

	printf("Set antenna port (1234):");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	input_num = strlen(inp)-1;
	ant = malloc(sizeof(uint8_t)*input_num);
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
	printf("Set Antenna port (enable:1/disable:0) [0]:");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &enable);
	if (enable != 0 && enable != 1 )
		return 0;
	__antenna_set_port(sockfd, ant, enable);
}

static void get_antenna(int sockfd)
{
	__antenna_get_port(sockfd);
}

static void set_epc_mask(int sockfd)
{
	char inp[10];
	int i, tmp=1, tmp1=1, val=0, tmp_mask[41]={0};

	printf("Set mask bank (EPC:1/TID:2) [1]:");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if (tmp==2)
		printf("TID mask beginning from 3th word\n");
	printf("Set mask words[1]");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp1);
	for (i=1;i<=tmp1;i++){
		printf("Set %dth mask word 0000]:",i);
		fgets(inp, sizeof(inp), stdin);
		sscanf(inp, "%x\n", &val);
		printf("%x\n",val);
		tmp_mask[(i-1)*2]= (val & 0xff00) >> 8;
		tmp_mask[(i-1)*2+1]= (val & 0xff);
		val=0;
	}
	__set_epc_mask(sockfd, tmp, tmp1, tmp_mask);
}

static void run_write_tags_ui(int sockfd)
{
	char inp[10];
	int tmp0=0, tmp1=2, tmp2=6 ,i , val=0, tmp_epc[41]={0};

	printf("Set mask status (disable:0/enable:1) [0]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp0);
	printf("Set write start address[byte:2]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp1);
	printf("Set write end address[byte:6]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp2);
	for (i=tmp1;i<=tmp2;i++){
		printf("Set write %dth byte EPC[0000]:\n",i);
		fgets(inp, sizeof(inp), stdin);
		sscanf(inp, "%x\n", &val);
		tmp_epc[(i-1)*2]= (val & 0xff00) >> 8;
		tmp_epc[(i-1)*2+1]= (val & 0xff);
		val = 0;
	}
	__run_write_tags(sockfd, tmp0 ,tmp1, tmp2, tmp_epc);
}

static void set_raw_mode(int sockfd)
{
	char inp[10];
	int tmp, tmp1;

	printf("Enable raw data mode? (y/n) [n]:");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__set_raw_mode(sockfd, 1);
	} else
		__set_raw_mode(sockfd, 0);
}

static void set_ip_address(sockfd)
{
	struct in_addr tmp_addr_ip, tmp_addr_mask, tmp_addr_gateway;
	char inp[50];

	printf("Input ip address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_ip)) {
		printf("Get wrong IP address:%s\n", inp);
		return;
	}
	printf("Input netmask address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_mask)) {
		printf("Get wrong netmask address:%s\n", inp);
		return;
	}
	printf("Input gateway address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_gateway)) {
		printf("get wrong gateway address:%s\n", inp);
		return;
	}
	printf("Will set ip      to %s\n.", inet_ntoa(tmp_addr_ip));
	printf("   & set netmask to %s\n.", inet_ntoa(tmp_addr_mask));
	printf("   & set gateway to %s\n.", inet_ntoa(tmp_addr_gateway));
	printf("Are you sure? (y/n) [n]");
	fgets(inp, sizeof(inp), stdin);
	if (inp[0] == 'y' || inp[0] == 'Y') {
		__set_ip_address(sockfd, &tmp_addr_ip, &tmp_addr_mask, &tmp_addr_gateway);
	} else
		printf("Do nothing!\n");
}

static void set_system_update(sockfd)
{
	char inp[10];
	int tmp, tmp1;

	printf("Sysem update (Y/N) [N]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y')
		__set_system_update(sockfd, 1);
}

static void set_debug_mode(sockfd)
{
	char inp[10];
	int tmp, tmp1;

	printf("Enable debug message (Y/N) [N]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__set_debug_msg(sockfd, 1);
	} else
		__set_debug_msg(sockfd, 0);
}

static void set_gpio(sockfd)
{
	char inp[10];
	int tmp;

	printf("Input gpio set value (port:1234) [0000]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	printf("Will set value to : %x\n", tmp);
	__set_gpio(sockfd, (tmp>>12)&0xf, (tmp>>8)&0xf, (tmp>>4)&0xf, tmp&0xf);
}

static void get_gpio(sockfd)
{
	__get_gpio(sockfd);
}

static void set_module_reset(sockfd)
{
	char inp[10];
	int tmp, tmp1;

	printf("Set module reset (Y/N) [N]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__set_module_reset(sockfd);
	}
}

static void set_system_reboot(sockfd)
{
	char inp[10];

	printf("Set system reboot (Y/N) [N]:\n");
	fgets(inp, sizeof(inp), stdin);
	if(inp[0] == 'y' || inp[0] == 'Y'){
		__set_system_reboot(sockfd);
	}
}

static void thermal_detection(sockfd)
{
	__thermal_detection(sockfd);
}

static void llc_test(sockfd)
{
	int i, data_len;
	char inp[10];
	unsigned char tmp[100];

	printf("Please type in FAVITE LLC commands:\n");
	printf("[Example : 5A-00520000-007E]\n");
	scanf("%s",tmp);
	printf("Test LLC Command is : %s (Y/N) [N]\n",tmp);
	scanf("%s",inp);
	data_len=strlen(tmp);
	for(i=0;i<data_len;i++)
	{
		if(tmp[i]>='0' && tmp[i]<='9'){
			tmp[i]=tmp[i]-'0';
		}else if(tmp[i]>='A' && tmp[i]<='F'){
			tmp[i]=10+tmp[i]-'A';
		}else if(tmp[i]>='a' && tmp[i]<='f'){
			tmp[i]=10+tmp[i]-'a';
		}else{
			printf("Not hex data!\n");
			break;
		}
	}
	if(data_len%2){
		printf("CMD data format error! Data bytes should be even\n");
	}else{
		for(i=0;i<data_len/2;i++){
			tmp[i]=(tmp[2*i]<<4)+tmp[2*i+1];
		}
	}
	if(tmp[0]==90 && tmp[(data_len/2)-1]==126){
		if(inp[0] == 'y' || inp[0] == 'Y'){
			__llc_test(sockfd,tmp,data_len/2);
		}
	}else{
		printf("CMD data format error! Data Head or Tail should be 0x5A & 0x7E seperately!");
	}
}

static void main_menu(int sockfd)
{
	int ret = 0, val;
	char inp[10];

	create_read_thread(sockfd);
	while (ret != -1) {
		printf("\ninput commands:\n");
		printf("\n--------Module main function-----\n");
		printf("[%02d] Start inventory\n", START_INVENTORY);
		printf("[%02d] Stop inventory\n", STOP_INVENTORY);
		printf("[%02d] Set output power\n", SET_POWER);
		printf("[%02d] Enable antenna\n", ENABLE_ANTENNA);
		printf("[%02d] Get antenna\n", GET_ANTENNA);
		printf("[%02d] Set Antenna dwell time\n", SET_DWELL_TIME);
		printf("\n--------Module minor function-----\n");
		printf("[%02d] Start read TID\n", START_READ_TID);
		printf("[%02d] Set EPC mask\n",SET_EPC_MASK);
		printf("[%02d] Run write tags\n",RUN_WRITE_TAGS);
		printf("[%02d] Get RFID module temprature\n", THERMAL_DETECTION);
		printf("[%02d] Test FAVITE LLC\n", LLC_TEST);
		printf("\n--------Reader function-----------\n");
		printf("[%02d] Set gpio output\n", GPIO_OUT);
		printf("[%02d] Get gpio intput\n", GPIO_IN);
		printf("[%02d] Set IP address\n", SET_IP_ADDRESS);
		printf("[%02d] Raw data mode\n", RAW_DATA);
		printf("[%02d] Clear tag data\n", CLEAR_DATA);
		printf("[%02d] Server debug message\n", DEBUG_MSG);
		printf("[%02d] Show server version\n", SERVER_VERSION);
		printf("[%02d] System update\n", FUNC_SYSTEM_UPDATE);
		printf("[%02d] Set RFID module reset\n", RFID_MODULE_RESET);
		printf("[%02d] Set RFID system reboot\n", RFID_SYSTEM_REBOOT);
		printf("[%02d] exit\n", EXIT_AP);
		printf("\ninput:");

		memset(inp, 0, sizeof(inp));
		fgets(inp, sizeof(inp), stdin);
		ret = atoi((const char *)inp);

		switch(ret) {
			case SERVER_VERSION:
				show_server_version(sockfd);
				break;
			case START_INVENTORY:
				inventory_ui(sockfd);
				break;
			case START_READ_TID:
				read_tid_ui(sockfd);
				break;
			case STOP_INVENTORY:
				stop_inventory(sockfd);
				break;
			case ENABLE_ANTENNA:
				set_antenna(sockfd);
				break;
			case GET_ANTENNA:
				get_antenna(sockfd);
				break;
			case SET_DWELL_TIME:
				set_dwell_time(sockfd);
				break;
			case SET_IP_ADDRESS:
				set_ip_address(sockfd);
				break;
			case RAW_DATA:
				set_raw_mode(sockfd);
				break;
			case CLEAR_DATA:
				__clear_tag_data();
				break;
			case DEBUG_MSG:
				set_debug_mode(sockfd);
				break;
			case FUNC_SYSTEM_UPDATE:
				set_system_update(sockfd);
				break;
			case RFID_MODULE_RESET:
				set_module_reset(sockfd);
				break;
			case GPIO_OUT:
				set_gpio(sockfd);
				break;
			case GPIO_IN:
				get_gpio(sockfd);
				break;
			case EXIT_AP:
				printf("exit AP");
				ret = -1;
				break;
			case RFID_SYSTEM_REBOOT:
				set_system_reboot(sockfd);
				break;
			case THERMAL_DETECTION:
				thermal_detection(sockfd);
				break;
			case LLC_TEST:
				llc_test(sockfd);
				break;
			case SET_EPC_MASK:
				set_epc_mask(sockfd);
				break;
			case RUN_WRITE_TAGS:
				run_write_tags_ui(sockfd);
				break;
			case SET_POWER:
				set_power_ui(sockfd);
				break;
			default:
				ret = 0;
		}
		printf("\n\n\n\n\n");
	}
}

/*
 * open network device or serial console
 */
int open_device(int b_network, int port_no, const char *file)
{
	int fd = -1, portno;
	struct sockaddr_in dest;

	if (b_network) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			printf("open socket: %s\n", strerror(errno));
			return -1;
		}
		//initialize the struct of serv_addr
		memset(&dest, 0, sizeof(dest));
		dest.sin_family = AF_INET;
		dest.sin_port = htons(port_no);
		if (inet_aton(file, (struct in_addr *) &dest.sin_addr.s_addr) == 0) {
			printf("GET IP: %s\n", strerror(errno));
			close(fd);
			return -1;
		}
		if (connect(fd, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
			printf("connect: %s\n", strerror(errno));
			close(fd);
			return -1;
		}
	} else {
		/*
		 * TODO: open uart port here
		 */
	}
exit:
	return fd;
}

void usage()
{
	printf("reader_cln [-p port_num] [-i ip_address]\n");
}

int main(int argc, char *argv[])
{
	int opt, fd, b_network = true, portno;
	char buf[30];

	portno = DEFAULT_NETPORT;
	strncpy(buf, DEFAULT_IP_ADDRESS, 30);
	while((opt = getopt(argc, argv, "i:p:s:h")) != -1 ) {
		switch(opt) {
			case 's':
				b_network = false;
				strncpy(buf, optarg, 30);
				break;
			case 'i':
				strncpy(buf, optarg, 30);
				break;
			case 'p':
				portno = atoi(optarg);
				break;
			case 'h':
			case '?':
				usage();
				break;
		}
	}
	fd = open_device(b_network, portno, buf);
	if (fd < 0)
		return 0;
	main_menu(fd);
	close(fd);
	return 0;
}
