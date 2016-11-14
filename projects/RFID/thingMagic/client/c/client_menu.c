#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "client_reader.h"


//main function
#define START_INV          1
#define STOP_INV           2
#define SET_POWER          3
#define CLEAR_DB           4
#define SET_TID_READ       5
//minor function
#define SET_GEN2_PROTOCOL 11
#define GET_TEMPERATURE   12
#define WRITE_TAGS        13
#define MODULE_RESET      14
#define SET_MULTI_POWER   15
#define CONFIG_OP         16
//reader function
#define SET_NETWORK       21
#define FW_RELOAD         22
#define SYSTEM_REBOOT     23
#define SYSTEM_UPDATE     24
#define SHOW_FW_VERSION   25
#define SET_GPIO          26
#define GET_GPIO          27

#define EXIT_AP           30

int command_en=1;
uint8_t *ant = NULL;

void start_inv(void)
{
	uint8_t ant_cnt=0, i, input_num, tmp1;
	uint32_t tmp;
	char buf[10], inp[10];

	printf("Set Antenna poart(1234):");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%x\n", &tmp);

	input_num = strlen(buf)-1;
	ant = malloc(sizeof(uint8_t)*input_num);

	for(i=0; i<input_num; i++) {
		ant[i] = tmp & 0xf;

		if (ant[i] < 1 || ant[i] > 4) {
			printf("Error :Incorrect Antenna Port : %d \n", ant[i]);
			free(ant);
			return;
		}
		ant_cnt++;
		tmp >>= 4;
	}

	printf("Enable/Disable fastsearch (Y/N)[N]");
	fflush(stdout);
	fgets(inp, sizeof(inp), stdin);
	if (inp[0] == 'y' || inp[0] == 'Y')
		tmp1 = 1;
	else
		tmp1 = 0;

	tm_startInv(ant, ant_cnt, tmp1);
}

void stop_inv(void)
{
	tm_stopInv();

	if (NULL != ant){
		free(ant);
		ant = NULL;
	}
}

void set_power(void)
{
	uint32_t tmp;
	char buf[10];

	printf("currently power is %.1lf dB.\n", ((double)tm_getpower()/100));
	printf("Set power [30.0 dB](MAX : 31.5 dB):");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%d\n", &tmp);
	if (tmp <= 0 || tmp > 315)
		tmp = 300;
	printf("Will set power to %.1lf dB\n\n", (double)tmp/10);
	tm_setpower(tmp*10);
}

void clear_db(void)
{
	tm_clear_db();
}

void set_gen2_protocol_ui(void)
{
	char buf[10];
	int tmp;

	printf("Enable/Disable max data rate mode (Y/N)[N]");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	if (buf[0] == 'y' || buf[0] == 'Y')
		tmp=1;
	else
		tmp=0;
	set_gen2_protocol(tmp);
}

void get_temperature(void)
{
	char buf[10], en_temp;

	printf("Enable/Disable Temperature Display ? (Y/N)[N]");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	if (buf[0] == 'y' || buf[0] == 'Y'){
		en_temp=1;
		tm_gettemperature(en_temp);
	}else {
		en_temp=0;
		tm_gettemperature(en_temp);
	}
}

void write_tags_ui(void)
{
	char buf[32];
	uint32_t tmp, i;
	int start = 2, epc_len, tmpData, bank =1;
	uint16_t *writeData;

	printf("Start address[2]:");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%x\n", &start);
	if (start<1 || start >16)
		start=2;
	printf("Start address is %d\n", start);
	printf("How many bytes(even)[12]?");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%d\n", &epc_len);
	if (epc_len<2 || epc_len >32)
		epc_len=12;
	epc_len = epc_len/2;
	printf("Epc length is %d byte(s)\n", epc_len*2);
	writeData = malloc(sizeof(uint16_t)*epc_len);

	for (i=0; i<epc_len; i++)
	{
		printf("%d & %d bytes(ex:500A):",(i+1)*2-1,(i+1)*2);
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		sscanf(buf, "%x\n", &tmpData);
		writeData[i] = (tmpData & 0xffff);
	}

	printf("Write tag epc:");
	for (i=0; i<epc_len; i++)
		printf("%5x", writeData[i]);
	printf("\n");

	printf("Set Antenna poart(1234)[1]:");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%x\n", &tmp);
	ant = malloc(sizeof(uint8_t));
	ant[0] = tmp & 0xf;
	printf("Antenna Port : %d \n", ant[0]);
	write_tags(ant, start, epc_len, writeData, bank);
}

static void set_module_reset(const char* tcp_buf)
{
	char inp[10];
	int tmp;
	printf("Set module reset(Y/N)[N] :\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__set_module_reset(tcp_buf);
	}
}

void set_multi_power(void)
{
	int i, *powerlist;
	uint32_t tmp;
	char buf[10];
	powerlist =malloc(sizeof(int)*4);

	tm_get_multi_power();
	for (i=0;i<4;i++)
	{
		printf("Set port %d power [30.0 dB](MAX : 31.5 dB):", i+1);
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		sscanf(buf, "%d\n", &tmp);
		if (tmp < 0 || tmp > 315)
			tmp = 300;
		powerlist[i] = tmp*10;
		memset(buf, '0', 10);
		tmp= 0;
	}
	for (i=0;i<4;i++)
		printf("Will set port %d power to %.1lf dB\n", i+1, (double)powerlist[i]/100);

	tm_set_multi_power(powerlist);
}

void module_config_ui(const char* tcp_buf)
{
	char inp[10];
	int tmp;
	printf("Config operation :\n[1] Show currently config\n[2] Save config\n[3] Restore config\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	tm_config_op(tmp, tcp_buf);
}

static void set_network(void)
{
	char inp[10];
	int tmp;
	printf("Set Network?(Y/N)[N] :\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__set_network();
	}
}

void fw_reload(void)
{
	char buf[10];

	printf("Do you want reload it? (Y/N)[N]");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	if (buf[0] == 'y' || buf[0] == 'Y'){
		tm_fw_reload();
	}
}

static void system_reboot(void)
{
	char inp[10];
	int tmp;
	printf("System Reboot?(Y/N)[N] :\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__system_reboot();
	}
}

static void system_update(void)
{
	char inp[10];
	int tmp;
	printf("System Update?(Y/N)[N] :\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%d\n", &tmp);
	if(inp[0] == 'y' || inp[0] == 'Y') {
		__system_update();
	}
}

static void show_fw_version(void)
{
	__show_fw_version();
}

static void set_gpio(void)
{
	char inp[10];
	int tmp;

	printf("Input gpio set value (port:1234) [0000]:\n");
	fgets(inp, sizeof(inp), stdin);
	sscanf(inp, "%x\n", &tmp);
	printf("Will set value to : %x\n", tmp);
	__set_gpio((tmp>>12)&0xf, (tmp>>8)&0xf, (tmp>>4)&0xf, tmp&0xf);
}

static void get_gpio(void)
{
	__get_gpio();
}

static void set_tid_read(void)
{
	char buf[10], en_temp;

	printf("Enable(Y)/Disable(N) TID Display ? (Y/N)[N]");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	if (buf[0] == 'y' || buf[0] == 'Y'){
		en_temp=1;
		__set_tid_read(en_temp);
	}else {
		en_temp=0;
		__set_tid_read(en_temp);
	}
}
char main_menu(const char* tcp_buf)
{
	int ret = 0;
	char inp[10];

	printf("ThingMagic RFID reader V%d.\n", APP_VERSION);
	tmr_reader_init(tcp_buf);
	while (1) {
		printf("input commands:\n");
		printf("--------Module main function-----\n");
		printf("[%02d] start inventory\n", START_INV);
		printf("[%02d] stop inventory\n", STOP_INV);
		printf("[%02d] set power\n", SET_POWER);
		printf("[%02d] clear data base\n", CLEAR_DB);
		printf("[%02d] set tid read\n", SET_TID_READ);
		printf("\n");
		printf("[%02d] enable/disable high data rate setting\n", SET_GEN2_PROTOCOL);
		printf("[%02d] get current temperature\n", GET_TEMPERATURE);
		printf("[%02d] write tag\n", WRITE_TAGS);
		printf("[%02d] reset module(shutdown pin)\n", MODULE_RESET);
		printf("[%02d] set power for each antenna port\n", SET_MULTI_POWER);
		printf("[%02d] show/save/restore module config\n", CONFIG_OP);
		printf("--------Reader function-----------\n");
		printf("[%02d] set network\n", SET_NETWORK);
		printf("[%02d] module firmware reload\n", FW_RELOAD);
		printf("[%02d] reboot system\n", SYSTEM_REBOOT);
		printf("[%02d] update system\n", SYSTEM_UPDATE);
		printf("[%02d] show fw version\n", SHOW_FW_VERSION);
		printf("[%02d] set gpio\n", SET_GPIO);
		printf("[%02d] get gpio\n", GET_GPIO);
		printf("[%02d] exit\n", EXIT_AP);

		memset(inp, 0, sizeof(inp));
		fgets(inp, sizeof(inp), stdin);
		ret = atoi((const char *)inp);
		switch(ret) {
			case START_INV:
				tm_check_antenna();
				start_inv();
				break;
			case STOP_INV:
				stop_inv();
				break;
			case SET_POWER:
				set_power();
				break;
			case CLEAR_DB:
				clear_db();
				break;
			case SET_TID_READ:
				set_tid_read();
				break;
			case SET_GEN2_PROTOCOL:
				set_gen2_protocol_ui();
				break;
			case GET_TEMPERATURE:
				get_temperature();
				break;
			case WRITE_TAGS:
				write_tags_ui();
				break;
			case MODULE_RESET:
				set_module_reset(tcp_buf);
				break;
			case SET_MULTI_POWER:
				set_multi_power();
				break;
			case CONFIG_OP:
				module_config_ui(tcp_buf);
				break;
			case SET_NETWORK:
				set_network();
				break;
			case FW_RELOAD:
				fw_reload();
				break;
			case SYSTEM_REBOOT:
				system_reboot();
				break;
			case SYSTEM_UPDATE:
				system_update();
				break;
			case SHOW_FW_VERSION:
				show_fw_version();
				break;
			case SET_GPIO:
				set_gpio();
				break;
			case GET_GPIO:
				get_gpio();
				break;
			case EXIT_AP:
				printf("exit AP\n");
				exit(1);
				break;
			default:
				break;
		}
		printf("\n");
	}
	return ret;
}
