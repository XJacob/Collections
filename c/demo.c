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

static char* const short_options = "l:hc:";
static struct option long_options[] = {
     { "log",	1,	NULL,	'l'},
     { "config",	1,	NULL,	'c'},
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
	printf("  -c [--config]  config file\n");
	printf("  -l [--log]   saving to log file\n");
	printf("  -h [--help]  help file\n");
	printf("\n\n\n");
}

// direction : 1-> left, 2->right, 3->both side
static void remove_extra_space(char *val, int direction)
{
	int idx = 0;
	int len;

	if (direction == 1 || direction == 3) {
		 len = strlen(val) + 1;
		 while(val[idx] != '\0' && val[idx] == ' ')
			  idx++;
		 if (val[idx] == '\0')
			  return;
		 memmove(val, val + idx, len - idx);
	}
	if (direction == 2 || direction == 3) {
		 len = strlen(val) - 1;
		 idx = len;

		 while(idx && (val[idx] == ' ' ||  val[idx] == '\n'))
			  idx--;
		 val[idx + 1] = '\0';
	}
}

static void process_config(const char *cfg_name, bool show_cfg)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0, read;
	char *default_name = "demo.cfg";
	char *value, *name;
	char *sperator;

	if (!cfg_name)
		 cfg_name = default_name;

	fp = fopen(cfg_name, "r");

	if (!fp) {
		fprintf(stderr, "error when opening file %s : ", cfg_name != NULL ? cfg_name : "demo.cfg");
		perror("");
		return;
	}

	value = NULL;
	name = NULL;
	print(" Config file: %s\n", cfg_name);
	while ((read = getline(&line, &len, fp)) != -1) {
	     if (line[0] != '#' && line[0] != ' ') {
		  sperator = strchr((const char *)line, (int)'=');
		  if (sperator) {
		       name = realloc(name, sperator - line);
		       value = realloc(value, strlen(sperator+1));

		       strncpy(value, sperator+1, strlen(sperator+1));
		       strncpy(name, line, sperator-line);
		       remove_extra_space(name, 2);
		       remove_extra_space(value, 3);
		       if (show_cfg)
			    print("     name= %s value= %s\n", name, value);
		  }
	     }
	}
	CHK_AND_FREE(line);
	CHK_AND_FREE(name);
	CHK_AND_FREE(value);
	fclose(fp);
}

int main(int argc, char *argv[])
{
	int opt;
	char *log_name, *cfg_name;

	log_name = NULL;
	cfg_name = NULL;
	while((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ) {
		switch(opt) {
			case 'l':
				log_name = malloc(strlen(optarg));
				if (!log_name)
					perror("out of memory");
				strncpy(log_name, optarg, strlen(optarg));
				break;
			case 'c':
				cfg_name = malloc(strlen(optarg));
				if (!cfg_name)
					perror("out of memory");
				strncpy(cfg_name, optarg, strlen(optarg));
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

	process_config(cfg_name, true);

	if (log_name)
		free(log_name);
	if (log_file)
		fclose(log_file);

	return 0;
}
