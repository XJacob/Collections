#ifndef __PRO_TOOLS__H_
#define __PRO_TOOLS__H_

#include <stdbool.h>

#define MAJOR_VER	1
#define MINOR_VER	0

extern FILE *log_file;
extern bool is_jig_in(void);
extern bool test_start(void);
extern void print(const char *fmt, ...);
#endif
