#ifndef __TARGET_A23_H__
#define __TARGET_A23_H__

// how many lines will clear screen?
#define CLEAN_SCRREN_LINE 15
#define LEFT_MARGIN "          "

//net test
#define NET_NAME "eth0"

//usb test
#define USB_MOUNT_PATH "/media/sda1/test.txt"

//module test
#define MODULE_PATH "/dev/ttymxc3"
#define TEST_PATTERN "abcdefg"

// gpio test
// the gpio pin to detect JIG is inserted
#define GPIO_JIG_DET     12  //TP199
#define GPIO_IN1         18
#define GPIO_IN2         19
#define GPIO_OUT3        16
#define GPIO_OUT4        17
#define GPIO_OUT5        20
#define GPIO_OUT6        21

#define START_DELAY      2000 * 1000 //us
#endif
