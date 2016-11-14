#ifndef __THINGMAGIC_FN_H
#define __THINGMAGIC_FN_H

#define CMD_GET_FW_VERSION	0xE0
#define CMD_DEVICE_REBOOT	0xE1
#define CMD_EN_UPDATE	    0xE2
#define CMD_SET_GPIO	    0xE3
#define CMD_GET_GPIO	    0xE4
#define CMD_MODULE_RESET	0xE5
#define CMD_SET_NETWORK	    0xE6
#define CMD_SET_LED	    	0xE7
//#define UART_DEV "tmr:///dev/ttymxc3"
//#define ThingsMagic_FW "/home/update/m6e_fw/M6eFW.sim"
#define numberof(x) (sizeof((x))/sizeof((x)[0]))

typedef struct tagdb_table
{
	/** The tag that was read */
	TMR_TagData tag;
	char *epc;
	/** TID of the tag */
	char *tid;
	/** Tag response phase */
	uint16_t phase;
	/** Antenna where the tag was read */
	uint8_t antenna;
	/** Strength of the signal recieved from the tag */
	int32_t rssi;
	/** RF carrier frequency the tag was read with */
	uint32_t frequency;
	/** Absolute time of the read (32 least-significant bits), in milliseconds since 1/1/1970 UTC */
	uint32_t timestampLow;
	/** Absolute time of the read (32 most-significant bits), in milliseconds since 1/1/1970 UTC */
	uint32_t timestampHigh;

	uint32_t tids;
	int32_t counts;
	struct tagdb_table *next;
} tagdb_table;

extern int command_en;
extern int region_cfg;
#endif
