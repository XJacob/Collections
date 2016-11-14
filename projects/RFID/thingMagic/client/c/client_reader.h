#ifndef __LOCAL_READER_H_
#define __LOCAL_READER_H_

#define APP_VERSION 1

extern void tmr_reader_init(const char* tcp_buf);
extern void tm_startInv(uint8_t *ant, uint8_t ant_count, uint8_t fastsearch);
extern void tm_stopInv(void);
extern uint32_t tm_getpower(void);
extern void tm_setpower(uint32_t);
extern void tm_clear_db(void);
extern void set_gen2_protocol(uint32_t enable);
extern void tm_gettemperature(char);
extern void tm_check_antenna(void);
extern int write_tags(uint8_t *ant, int start, int epc_len, uint16_t *writeData, int bank);
extern void __set_module_reset(const char* tcp_buf);
extern void __set_network(void);
extern void tm_fw_reload(void);
extern void __system_reboot(void);
extern void __system_update(void);
extern void __show_fw_version(void);
extern void __set_gpio(uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t gpio4);
extern void __get_gpio(void);
extern void __set_tid_read(char flag);
extern void tm_get_multi_power(void);
extern void tm_set_multi_power(int *powerlist);
extern void tm_config_op(int op, const char* tcp_buf);

typedef struct config_set
{
	char *pPOWER;
	char *pSLEEP;
	char *pANT;
	char *pREGION;
	char *pHOPTABLE;
	char *pHOPTIME;
	char *pTID;
	char *pFAST_INV;
	char *pTEMP;
	char *pACCESSPASSWORD;
	char *pQ;
	char *pTAGENCODING;
	char *pSESSION;
	char *pTARGET;
	char *pBLF;
	char *pTARI;
} config_set;

extern void getcmd(int argc, char *argv[]);
extern void setcmd(char* name, char *value);
extern void getoneparam(const char *paramName);
#endif
