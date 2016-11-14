#ifndef _READER_CLN_LLC_H__
#define _READER_CLN_LLC_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define START 0x5A
#define STOP 0x7E
#define CHECKSUM 0x00
#define TRUE 1
#define FALSE 0

struct rfid_cmd {
	char name[50];
	unsigned char data[150];
	unsigned int repeat_times;
	unsigned int delay;   // delay before executing this command
	int need_response;
};

struct tag_data {
	unsigned int antenna_port;
	unsigned int nb_rssi;
	unsigned int wb_rssi;
	unsigned int time_stamp; //ms
	unsigned char pc_block[2];
	unsigned char *epc_block;
	unsigned char *tid_block;
	unsigned char crc_block[2];
	unsigned int epc_len;
	unsigned int tid_len;
	unsigned int times;
};

struct __tag_list {
	struct tag_data *data;
	struct __tag_list *next;
};

/**
 * Get server software version
 *
 * @param fd connected socket fd
 */
extern void show_server_version(int fd);

/**
 * Enable rfid inventory mode
 *
 * @param fd connected socket fd
 */
extern void start_inventory(int fd);

/**
 * Enable rfid read TID mode
 *
 * @param fd connected socket fd
 */
extern void start_read_tid(int fd);

/**
 * stop rfid inventory mode
 *
 * @param fd connected socket fd
 *
 */
extern void stop_inventory(int fd);

/**
 * We can't expect the recevice message is notify or \n
 * command's response. So we need to create a thread \n
 * to identify it.
 *
 */
extern void create_read_thread(int fd);

/**
 * Set antenna dwell time
 *
 * @param fd connected socket fd
 * @param time antenna dwell time(ms)
 */
extern void __antenna_dwell_time(int fd, int time);

/**
 * Set antenna port
 *
 * @param fd connected socket fd
 * @paprm *port slect antenna port list
 * @paprm enable set antenna to enable or disable
 */
extern void __antenna_set_port(int fd, int *port, int enable);

/**
 * Get antenna port
 *
 * @param fd connected socket fd
 */
extern void __antenna_get_port(int fd);

/**
 * Set server ip address
 *
 * @param fd connected socket fd
 * @param dest the confirmed ip address strcut
 */
extern void __set_ip_address(int fd, struct in_addr *dest_ip, struct in_addr *dest_mask, struct in_addr *dest_gateway);

/**
 * set raw mode
 *
 * @param fd connected socket fd
 * @param enable set 1 will enable raw data mode
 */
extern void __set_raw_mode(int fd, int enable);

/**
 * set module reset
 *
 * @param fd connected socket fd
 */
extern void __set_module_reset(int fd);

/**
 * set gpio
 *
 * @param fd connected socket fd
 * @param gpio1 set value for gpio1
 * @param gpio2 set value for gpio2
 * @param gpio3 set value for gpio3
 * @param gpio4 set value for gpio4
 */
extern void __set_gpio(int fd, unsigned char gpio1, unsigned char gpio2, unsigned char gpio3, unsigned char gpio4);

/**
 * set system reboot
 *
 * @param fd connected socket fd
 */
extern void __set_system_reboot(int fd);

/**
 * set thermal detection
 *
 * @param fd connected socket fd
 */
extern void __thermal_detection(int fd);

/**
 * LLC Test
 *
 * @param fd connected socket fd
 */
extern void __llc_test(int fd,unsigned char cmd_data[100],int data_len);

/**
 * get gpio
 *
 * @param fd connected socket fd
 */
extern void __get_gpio(int fd);

/**
 * Set EPC mask
 *
 * @param fd connected socket fd
 * @param mask set mask
 * @param count set mask count(word)
 * @param bank select EPC or TID mask
 */
extern void __set_epc_mask(int fd,int bank, int count, int mask[41]);

/*
 * Run write tags
 *
 * @param fd connected socket fd
 * @param power write tags power [0~30 dB]
 * @param start write address start
 * @param end write address end
 * @param *tmp_epc EPC data
 * @param mask_sw mask disable or enable
 */
extern void __run_write_tags(int fd, int mask_sw, int start, int end, int tmp_epc[41]);

/*
 * Set antenna port power
 *
 * @param fd connected socket fd
 * @paprm port set antenna port (1234)
 * @param power write tags power [0~32 dB]
 */
extern int __set_antenna_power(int fd, int port, int power);

#endif
