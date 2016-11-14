#ifndef __DIGITAL_BOARD_H__
#define __DIGITAL_BOARD_H__

#define ANT_GPIO1 7
#define ANT_GPIO2 8
#define ANT_GPIO3 9
#define SCAN_GPIO 10
#define ANT4_Y0 3
#define ANT3_Y2 2
#define ANT2_Y1 1
#define ANT1_Y5 0
#define SCAN 9

extern char __module_reset(void);
extern void oneshot_scan_led(int);
extern void __set_gpio(unsigned char gpio1, unsigned char gpio2, unsigned char gpio3, unsigned char gpio4);
extern void __get_gpio(unsigned char *gpio1, unsigned char *gpio2);
#endif
