#ifndef __MODULE_H__
#define __MODULE_H__

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
