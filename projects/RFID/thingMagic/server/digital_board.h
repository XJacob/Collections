#ifndef __DIGITAL_BOARD_H__
#define __DIGITAL_BOARD_H__

/* system config file */
#define SYS_CONFIG_FILE "/home/root/system.config"


#define SCAN_GPIO 10

extern char __module_reset(void);
extern void oneshot_scan_led(int);
extern void __set_gpio(unsigned char gpio1, unsigned char gpio2, unsigned char gpio3, unsigned char gpio4);
extern void __get_gpio(unsigned char *gpio1, unsigned char *gpio2);
extern unsigned char __get_fw_version(void);
extern void __device_reboot();
extern void __system_update();
extern void __set_system_ip(const unsigned char *buf, int len);
extern void __set_ant_led(unsigned char led1, unsigned char led2, unsigned char led3, unsigned char led4, unsigned short time);
#endif
