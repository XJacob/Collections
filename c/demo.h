#ifndef __DEMO_H__
#define __DEMO_H__

#define CHK_AND_FREE(x) do { \
							if(x) free(x);	\
						} while(0);	\

extern void print(const char *, ...);

#endif
