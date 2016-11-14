#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

extern char main_menu(const char *tcp_buf);
extern char main_config(const char *tcp);

int main(int argc, char *argv[])
{
	FILE *fp;
	char line[100];
	char tcp_buf[100] = {0};
	char str[] = "command_en=";
	char str_tcp[] = "uri=";
	int command_en=1;

	fp = fopen("config.prop","r");
	if (fp==NULL)
	{
		printf("Can't find the file:config.prop\n");
	}
	while (!feof(fp)) {
		memset(line,0,100);
		fgets(line,100,fp);
		if (*line != '#'){
			if(!strncmp(line,str,strlen(str)) && (strlen(line)-1)!=strlen(str)) {
				command_en = atoi(line+strlen(str));
			}
			if(!strncmp(line,str_tcp,strlen(str_tcp)) && (strlen(line)-1)!=strlen(str_tcp)) {
				memcpy(tcp_buf,line+strlen(str_tcp),strlen(line)-strlen(str_tcp)-1);
			}
		}
	}
	fclose(fp);
	if (command_en>0) {
		printf("Switch to  Command Line mode\n");
		if (argv[1] == NULL)
			printf("Input Error! Please key in ip. ex ./client_reader 192.168.10.10\n");
		else
			main_menu(argv[1]);
	}
	else {
		printf("Switch to  Config File mode\n");
		printf("Switch to  Config File mode, ip is %s\n",tcp_buf);
		main_config(tcp_buf);
	}

	return 0;
}
