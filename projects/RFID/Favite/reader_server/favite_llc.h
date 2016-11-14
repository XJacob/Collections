#ifndef __SERVER_LLC_H_
#define __SERVER_LLC_H_

/**
 *@file
 *
 * We define our own LLC(low level command) here. \n
 * The command follows FAVITE's LLC format.\n
 * \n
 * Message formate as follow:
 *
  | Name          | Length | default|
  | :----:        | :----: | :----: |
  | Preamble      | 1 byte | 0x5A   |
  | Message type  | 1 byte | -      |
  | Message code  | 1 byte | -      |
  | Payload length| 2 bytes| -      |
  | Payload       | N bytes| -      |
  | Checksum      | 1 byte | -      |
  | End mark      | 1 byte | 0x7E   |
 *
 * \n
 * Message type format:
  | Message Type  | Value  |
  | :----:        | :----: |
  | Command       | 0x00   |
  | Response      | 0x01   |
 *
 *\n Checksum: \n
 * The checksum is the result of XOR the whole packet except the Preamble and End Mark. \n
 * For example, the packet of command "get reader sw version" is: \n
 *  0x5A 0x00 0xD0 0x00 0x00 0x7E \n
 * So the checksum is 0x00 ^ 0xD0 ^ 0x00 ^ 0x00 = 0xD0 \n
 *
 */
#include "default_val.h"

/*! \cond PRIVATE */
struct __server_llc {
	unsigned char preamble;
	unsigned char type;
	unsigned char code;
	unsigned short length;
	unsigned char *payload;
	unsigned char checksum;
	unsigned char endmark;
};

#define TYPE_OFFSET 1
#define CODE_OFFSET 2
#define LEN_OFFSET 3
#define LOAD_OFFSET 5

#define PREAMBLE 0x5A
#define CHECKSUM 0x00
#define END_MARK 0x7E

#define EPC_MC 0x50
#define TID_MC 0xF3
#define SET_ANTENNA_PORT 0x12
#define GET_ANTENNA_PORT 0x13

#define TYPE_CMD 0x00
#define TYPE_RET 0x01
#define TYPE_NOF 0x02

#define PAYLOAD_LEN(x) (x[3]<<8 | x[4])
#define CMD_LENGTH(x) (PAYLOAD_LEN(x) + 7)  //payload length+other

extern int process_buf_svr(int sockfd, unsigned char *buf, int len);
extern int write_to_client(int sockfd, const unsigned char *buf, int len);

/* calcuate checksum for complete raw packet */
static inline unsigned char calc_checksum(const unsigned char *buf, int len)
{
	int j, tmp=0;

	for (j=1; j<len-2; j++)
		tmp ^= buf[j];
	return tmp;
}

static inline int is_packet_correct(const unsigned char *buf, const int len)
{
	return len < 7 ? false : (calc_checksum(buf, len) == buf[len-2]);
}

/*! \endcond */

/**
 * Read reader sw version \n
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD0  |
| Palyoad Len   | 0x00  |
 *
 * Example:
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD0 |0x00    |0x00   |       |-       |0x7E    |
\n
 *
 * @return
 * The server will return the 4 bytes version information major: FW and SW. \n
 *
 * Example: (return SW version v0.2, FW version v1): \n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD0 |0x00    |0x04   |       |-       |0x7E    |
 * <pre> </pre>
 *\n
 * Payload format(3 bytes):\n
| SW-Major | SW-Minor | FW-version |
| :----:   | :----:   | :----:     |
| 0x00     | 0x02     | 0x01       |
 *
 */
#define READ_SW_VERSION 0xD0

/**
 * Set IP address.
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD4  |
| Palyoad Len   | 0x04  |
 *
 * Payload format:\n
|IP addr1|IP addr2|IP addr3|IP addr4|MASK addr1|MASK addr2|MASK addr3|MASK addr4|Gateway addr1|Gateway addr2|Gateway addr3|Gateway addr4|
|:----:  |:----:  |:----:  |:----:  |:----:    |:----:    |:----:    |:----:    |:----:       |:----:       |:----:       |:----:       |
|0x00    |0x00    |0x00    |0x00    |0x00      |0x00      |0x00      |0x00      |0x00         |0x00          |0x00        |0x00         |

 *
 * After writing this command, the server IP & netmask & gateway will be changed. \n
 * Client should reconnect to server with the new IP & netmask & gateway address. \n
 * Please make sure the IP & netmask & gateway address is workable before writing it.\n
 *
 * Example:  Set IP & netmask & gateway address to 10.1.100.20, netmask to 255.255.0.0, gateway to 1.2.3.4 \n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|payload|payload|payload|payload|payload|payload|payload|payload|payload|payload|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD4 |0x00    |0x0C   |0x0A   |0x01   |0x64   |0x14   |0xFF   |0xFF   |0x00   |0x00   |0x01   |0x02   |0x03   |0x04   |-       |0x7E    |
\n
 *
 * @return
 * The server will return status OK(0x00).  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD4 |0x00    |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define SET_IP 0xD4

/**
 * System update
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD5  |
| Palyoad Len   | 0x00  |
 *
 * Before running this command, the update.tar.gz should be put on the reader \n
 * via FTP. The update flow is: \n
 * 1. Connect to reader by FTP client(ex. FileZilla) \n
 * 2. copy the update.tar.gz file to it \
 * 3. write this llc command \n
 *
 * After this command, the reader will update and reboot.
\n
 *
 * @return
 * The server will return status OK(0x00).  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD5 |0x00    |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define SYSTEM_UPDATE 0xD5

/**
 * Module reset
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD6  |
| Palyoad Len   | 0x00  |
 *
 * After this command, the RFID_Module will reset.
\n
 *
 * @return
 * The server will return status OK(0x00).  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD6 |0x00    |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define MODULE_RESET 0xD6

/**
 * Write GPIO
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD7  |
| Palyoad Len   | 0x04  |
 *
 * In our reader, it can control 4 output gpio pins.
 * Fill '1' to gpio port will set this port to high level.
 * Fill '0' to gpio port will set this port to low level.
 *
 * Payload format:\n
|Port1 |Port2 |Port3 |Port4 |
|:---: |:---: |:---: |:---: |
|0x00  |0x00  |0x00  |0x00  |
 *
 *
 * Example: Set gpio port 1 and 3 to high level \n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|payload|payload|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:   |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD7 |0x00    |0x04   |0x01   |0x00   |0x01   |0x00   |   -    |0x7E    |
\n
\n
 *
 * @return
 * The server will return gpio status.  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|payload|payload|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:   |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD7 |0x00    |0x04   |0x01   |0x00   |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define SET_GPIO 0xD7

/**
 * Read GPIO
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD8  |
| Palyoad Len   | 0x00  |
 *
 * In our reader, it can read 2 input gpio pins.
 *
 *
 * Example: read gpio input ports status\n
|preamble| MT  |MC   |PL(high)|PL(low)|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD8 |0x00    |0x00   |   -    |0x7E    |
\n
\n
 *
 * @return
 * The server will return gpio status.  \n
 *
 * Example: return gpio port 1 is high and port 2 is low\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD8 |0x00    |0x02   |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define GET_GPIO 0xD8

/**
 * SYSTEM REBOOT
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xD9  |
| Palyoad Len   | 0x00  |
 *
 * After this command, the RFID_SYSTEM will reboot.
\n
 *
 * @return
 * The server will return status OK(0x00).  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xD9 |0x00    |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define SYSTEM_REBOOT 0xD9

/**
 * RAW DATA MODE
 *
 * In this mode, reader won't process any tag data. \n
 * But the data may get the wrong antenna port information. \n
 * This mode should be DEBUG only. \n
 *
 * \n
 * \n Command format:
| Name          | Length|
| :----:        | :----:|
| Message type  | 0x00  |
| Message code  | 0xDF  |
| Palyoad Len   | 0x01  |
 *
 * Payload format:\n
|Enable |
|:---:  |
|0x00   |
 *
 *
 * Example: Enable Raw data mode \n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xDF |0x00    |0x01   |0x01   |   -    |0x7E    |
\n
 *
 * @return
 * The server will return status OK(0x00).  \n
 *
 * Example:\n
|preamble| MT  |MC   |PL(high)|PL(low)|payload|checksum|End mark|
|:--:    |:--: |:--: |:--:    |:--:   |:--:   |:--:    |:--:    |
|0x5A    |0x00 |0xDF |0x00    |0x01   |0x00   |-       |0x7E    |
 * <pre> </pre>
 *\n
 *
 */
#define RAW_DATA_MODE 0xDF

/*! \cond PRIVATE */
// Enable/Disalbe debug message on Server
#define DEBUG_MODE 0xDE
/*! \endcond */
#endif
