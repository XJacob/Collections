#ifndef __MODULE_H__
#define __MODULE_H__


#define CMD_GET_FW_VERSION	0xE0
#define CMD_DEVICE_REBOOT	0xE1
#define CMD_EN_UPDATE	    0xE2
#define CMD_SET_GPIO	    0xE3
#define CMD_GET_GPIO	    0xE4
#define CMD_MODULE_RESET	0xE5
#define CMD_SET_NETWORK	    0xE6
#define CMD_SET_LED	    	0xE7
#define CMD_BAUDRATE_RPOBE 	0xE8

#define HEAD_LEN	0x07

//status
#define STATUS_WORNG_BITS   0x107
#define TM_SUCCESS          0x0

struct read_thread_param {
	int sockfd;
	int modulefd;
};

extern int is_module_open(void);
extern int open_module(int sockfd);
extern int close_module(void);
extern int write_module(const char *buf, int len);
extern void module_enable_raw_data(int enable);
#endif
