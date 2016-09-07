#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>

#include "demo.h"

static char* const short_options = "l:h";
static struct option long_options[] = {
     { "log",	1,	NULL,	'l'},
     { "help",	0,	NULL,	'h'},
     {  NULL,	0,	NULL,	0},
};


static FILE *log_file = NULL;

void print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);

	if (log_file) {
		va_start(ap, fmt);
		vfprintf(log_file, fmt, ap);
		va_end(ap);
	}
}


static void usage(const char *name)
{
	printf("usage::\n");
	printf("%s [OPTIONS]...\n", name);
	printf("  -l [--log]   saving to log file\n");
	printf("  -h [--help]  help file\n");
	printf("\n\n\n");
}

int main(int argc, char *argv[])
{
	int opt;
	char *log_name;

	log_name = NULL;
	while((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ) {
		switch(opt) {
			case 'l':
				log_name = malloc(strlen(optarg));
				if (!log_name)
					perror("out of memory");
				strncpy(log_name, optarg, strlen(optarg));
				break;
			case 'h':
				usage(argv[0]);
				break;
		}
	}

	if (log_name) {
		log_file = fopen(log_name, "w");
		if (!log_file) {
			fprintf(stderr, "error when opening file %s", log_name);
			perror("");
		}
		print("open log file: %s, %s\n", log_name, log_file ? "success" : "failed");
	}

	if (log_name)
		free(log_name);
	if (log_file)
		fclose(log_file);

	return 0;
}
