#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include "pro_tools.h"

static bool is_force_run = false;
FILE *log_file = NULL;

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

static bool is_run_test(void)
{
	bool ret = true;
	ret &= is_jig_in();
	print("Detecting jig......(%c)\n", ret ? 'O' : 'X');
	return ret;
}

static void usage(const char *name)
{
	print("usage::\n");
	print("%s [OPTIONS]...\n", name);
	print("  -f    force run. Do not detect JIG.\n");
	print("\n\n\n");
}

static void start(bool is_log, const char *filename)
{
	if (is_log) {
		log_file = fopen(filename, "w");
		print("Enable log file: %s, %s\n", filename, log_file ? "success" : "failed");
	}
	test_start();

	if (log_file)
		fclose(log_file);
}

int main(int argc, char *argv[])
{
	int opt;
	bool is_log_file = false;
	char *log_file;

	while((opt = getopt(argc, argv, "fL:h")) != -1 ) {
		switch(opt) {
			case 'f':
				is_force_run = true;
				break;
			case 'L':
				is_log_file = true;
				log_file = malloc(strlen(optarg));
				if (!log_file) {
					printf("out of memory!\n");
					is_log_file = false;
				}
				strncpy(log_file, optarg, strlen(optarg));
				break;
			case 'h':
				usage(argv[0]);
				break;
		}
	}

	print("Production tools V%d.%d\n", MAJOR_VER, MINOR_VER);
	if (is_run_test()) {
		print("Running production test...\n");
		start(is_log_file, log_file);
	} else if (is_force_run) {
		print("Run production test because force flag is set..\n");
		start(is_log_file, log_file);
	} else
		print("Skip production test..\n");

	if (log_file)
		free(log_file);

	return 0;
}


