#ifndef __DEFAULT_H__
#define __DEFAULT_H__

/**
 *@file
 *
 * Put all the default values in this file
 */

/*! \cond PRIVATE */
int debug_flag;
#define DEBUG(format, args...) do { \
								if(debug_flag)  \
	                                 printf("[%s:%d] "format, __FILE__, __LINE__, ##args); \
							   } while(0);

//#define DEBUG_ON_PC

#define true 1
#define false 0

static inline print_buf(const unsigned char *name, const unsigned char *buf, int len)
{
	int i;
	printf("%s:\n", name);
	for(i=0; i<len; i++) {
		printf(" %02x ", buf[i]);
		if (i && !i%16)
			printf("\n");
	}
	printf("\n");
}
/*! \endcond */

/** default network port number */
#define DEFAULT_NETPORT 50005

/** network buffer default 1K bytes */
#define NETWORK_BUFFER_SIZE 1 * 1024
/** network maximum connections */
#define NETWORK_LISTEN_NUM 1

/** server software version */
#define SERVER_VER_MAJOR 1
#define SERVER_VER_MINOR 2

#define DEFAULT_IP_ADDRESS "127.0.0.1"

/* If user input wrong IP address, will use this default ip*/
#define DEFAULT_SET_IP_ADDRESS "172.16.8.226"

/* The user's config IP will be written in the file */
#define CONFIG_DIR    "/home/update/config"
#define SAVED_IP_FILE "/home/update/config/ip_addr.txt"


/** UART parameters, for module */
#define BAUD_RATE B115200
#define MODULE_PATH "/dev/ttymxc3"
#endif
