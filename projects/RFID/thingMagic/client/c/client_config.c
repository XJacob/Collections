#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "client_reader.h"

static uint8_t *ant = NULL;

static int flag_err(char *flag, int value)
{
	if(value > 1 || value < 0) {
		printf("Config File Error!! %s%d is incorrected, flag value should be 1 or 0.\n",flag,value);
		return 1;
	}else return 0;
}

static int config_loading(char *line, char *str, char *param)
{
	if(!strncmp(line,str,strlen(str)) && (strlen(line)-1)!=strlen(str)) {
		memcpy(param,line+strlen(str),strlen(line)-strlen(str)-1);
		printf("Key : %s%s\n",str,param);
		return 0;
	}else return 1;
}

char main_config(const char* tcp)
{
	FILE *fp;
	char inp[10] = {0};
	char line[100],buffer[100];
	char *ant_temp = "1,2,3,4";
	char region[20];
	char hoptable[150];
	char hoptime[10];
	char gen2_Q[10] = "DynamicQ";
	char gen2_password[10] = {0};
	char gen2_tagEncoding[10] = "M4";
	char gen2_session[10] = "S0";
	char gen2_target[10] = "AB";
	char gen2_blf[10] = "250kHz";
	char gen2_tari[10] = "25us";
	int i;
	//#define initial value
	int power=3000, tid_en=0,sleep_time=-1, fastsearch_flag=0, temperature_flag;
	unsigned char ant_cnt=0;

	config_set config = {"power=","sleep=","antenna=","region=","hoptable=","hoptime=","tid=","fast_search=","temperature=","gen2_accesspassword=","gen2_Q=","gen2_tagEncoding=","gen2_session=","gen2_target=","gen2_blf=","gen2_tari="};

	printf("ThingMagic RFID reader V%d.\n", APP_VERSION);

	tmr_reader_init(tcp);
	__show_fw_version();

	fp = fopen("config.prop","r");
	if (fp==NULL)
	{
		printf("Can't find the file:config.prop\n");
		return 0;
	}
	printf("\n------------------config file settings-----------------\n");
	while (!feof(fp)) {
		memset(line,0,100);
		memset(buffer,0,100);
		fgets(line,100,fp);
		if (*line != '#'){
			if (!config_loading(line,config.pPOWER,&buffer[0])){
				power = atoi(buffer);
			}
			else if(!config_loading(line,config.pTID,&buffer[0])) {
				tid_en = atoi(buffer);
				if (flag_err(config.pTID,tid_en)) return 0;
			}
			else if(!config_loading(line,config.pSLEEP,&buffer[0])) {
				sleep_time = atoi(buffer);
			}
			else if(!config_loading(line,config.pANT,&buffer[0])) {
				ant_temp = strtok(buffer,",");
				while (ant_temp != NULL) {
					if (*ant_temp<49 || *ant_temp>52) {
						printf("Config File Error !! Antenna must be 1 ~ 4, EX:antenna=1,2,3,4\n");
						return 0;
					}
					inp[ant_cnt] = (char)(*ant_temp-48);
					ant_cnt++;
					ant_temp = strtok(NULL,",");
				}
				ant = malloc(sizeof(uint8_t)*ant_cnt);
				for (i=0;i<ant_cnt;i++) {
					ant[i] = inp[i];
				}
			}
			else if (!config_loading(line,config.pREGION,&region[0])) {
				setcmd("/reader/region/id",region);
			}
			else if (!config_loading(line,config.pHOPTABLE,&hoptable[0])) {
				setcmd("/reader/region/hopTable",hoptable);
			}
			else if (!config_loading(line,config.pHOPTIME,&hoptime[0])) {
				setcmd("/reader/region/hopTime",hoptime);
			}
			else if (!config_loading(line,config.pFAST_INV,&buffer[0])) {
				fastsearch_flag = atoi(buffer);
				if (flag_err(config.pFAST_INV,fastsearch_flag)) return 0;
			}
			else if (!config_loading(line,config.pTEMP,&buffer[0])) {
				temperature_flag = atoi(buffer);
				if (flag_err(config.pTEMP,temperature_flag)) return 0;
			}
			else if (!config_loading(line,config.pACCESSPASSWORD,&gen2_password[0])) {
				setcmd("/reader/gen2/accessPassword",gen2_password);
			}
			else if (!config_loading(line,config.pQ,&gen2_Q[0])) {
				setcmd("/reader/gen2/q",gen2_Q);
			}
			else if (!config_loading(line,config.pTAGENCODING,&gen2_tagEncoding[0])) {
				setcmd("/reader/gen2/tagEncoding",gen2_tagEncoding);
			}
			else if (!config_loading(line,config.pSESSION,&gen2_session[0])) {
				setcmd("/reader/gen2/session",gen2_session);
			}
			else if (!config_loading(line,config.pTARGET,&gen2_target[0])) {
				setcmd("/reader/gen2/target",gen2_target);
			}
			else if (!config_loading(line,config.pBLF,&gen2_blf[0])) {
				setcmd("/reader/gen2/BLF",gen2_blf);
			}
			else if (!config_loading(line,config.pTARI,&gen2_tari[0])) {
				setcmd("/reader/gen2/tari",gen2_tari);
			}
		}
	}

	tm_gettemperature(temperature_flag);
	__set_tid_read(tid_en);
	tm_setpower(power);
// display Gen2 protocol

//	printf("------------------display config file settings-----------------\n\n");
	getoneparam("/reader/region/id");
	getoneparam("/reader/region/hopTable");
	getoneparam("/reader/region/hopTime");
	getoneparam("/reader/gen2/accessPassword");
	getoneparam("/reader/gen2/q");
	getoneparam("/reader/gen2/tagEncoding");
	getoneparam("/reader/gen2/session");
	getoneparam("/reader/gen2/target");
	getoneparam("/reader/gen2/BLF");
	getoneparam("/reader/gen2/tari");

	tm_startInv(ant, ant_cnt, fastsearch_flag);
	if (sleep_time<0){
		while(1);
	} else {
		sleep(sleep_time);
	}
	tm_stopInv();
	if (NULL != ant){
		free(ant);
		ant = NULL;
	}
	fclose(fp);
	return 0;
}
