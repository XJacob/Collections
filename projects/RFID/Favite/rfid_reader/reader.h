#ifndef __READ_H_
#define __READ_H_

#define START 0x5A
#define STOP 0x7E
#define WAIT_READ 0xff
#define CHECKSUM 0x00

#define TRUE 1
#define FALSE 0
#define CMD_LENGTH(x) ((x[3]<<8 | x[4]) + 7)  //payload length+other

#define INVENTORY_CMD_ID      1
#define STOP_INVENTORY_CMD_ID 2
#define SET_ANTENNA_ID        3
#define SET_ANTENNA_POWER     4
#define SET_DWELL_TIME        5
#define INVENTORY_NONCON_ID   6
#define EXIT_AP               9

struct rfid_cmd {
	char name[30];
	unsigned char data[100];
	unsigned int repeat_times;
	unsigned int delay;   // delay before executing this command
	int need_response;
};

struct tag_data {
	unsigned int antenna_port;
	unsigned int nb_rssi;
	unsigned int wb_rssi;
	unsigned int time_stamp; //ms
	unsigned char *epc_block;
	unsigned int len;
	unsigned int times;
};

struct __tag_list {
	struct tag_data *data;
	struct __tag_list *next;
};

struct antenna_info {
	unsigned int ports;
	unsigned int sleep_time;
};

#endif
