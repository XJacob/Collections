/*
 *ThingMagic module API call
 */
#include <tm_reader.h>
#include <stdint.h>
#include <serial_reader_imp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "thingMagic_fn.h"

/* Enable this to use transportListener */
//#define USE_TRANSPORT_LISTENER 1

static pthread_mutex_t tag_data_lock;
static int uniqueCount=0,totalCount=0,list_len=0;
static unsigned long int first_tag_time_L, first_tag_time_H, last_tag_time_L, last_tag_time_H;
static tagdb_table *seenTags_head,*seenTags_end;
static const char *targetNames[] = {"A", "B", "AB", "BA"};

TMR_Reader r, *rp;
static uint8_t inventory_run=0, tid_read=0;
static TMR_ReadListenerBlock rlb;
static TMR_ReadExceptionListenerBlock reb;
static TMR_StatsListenerBlock slb;

static pthread_t printf_tag_thread;


/* Enable this to use transportListener */
#if USE_TRANSPORT_LISTENER
static TMR_TransportListenerBlock tb;
#endif

#if USE_TRANSPORT_LISTENER
static void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
		uint32_t timeout, void *cookie)
{
	FILE *out = cookie;
	uint32_t i;

	fprintf(out, "%s", tx ? "Sending: " : "Received:");
	for (i = 0; i < dataLen; i++)
	{
		if (i > 0 && (i & 15) == 0)
			fprintf(out, "\n         ");
		fprintf(out, " %02x", data[i]);
	}
	fprintf(out, "\n");
}

static void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
	FILE *out = cookie;

	fprintf(out, "%s", tx ? "Sending: " : "Received:");
	fprintf(out, "%s\n", data);
}
#endif
static void __ant_led_switch(uint8_t* ant,uint8_t ant_count,short int dwell_time)
{
	TMR_Status ret;
	uint8_t data[11]={0},len,opcode,i=0;
	uint32_t timeoutMs=100;

	len=6;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_SET_LED;
	while (i<ant_count)
	{
		data[ant[i]+2]=1;
		i++;
	}
	data[7]=(dwell_time & 0xff00) >> 8;
	data[8]=dwell_time & 0xff;
	printf("dwell_time is %d\n",dwell_time);

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}

	ret = TMR_SR_receiveMessage(rp, data, opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Receive Message time out!\n");
		return;
	}
}

static char *my_strdup(const char *string)
{
 	char *nstr;

	nstr = (char *) malloc(strlen(string) + 1);
	if (nstr)
	{
		strcpy(nstr, string);
	}
	return nstr;
}

static tagdb_table *db_lookup(char *epc, const TMR_TagReadData *t, char *tidStr)
{
	tagdb_table *list;


	for(list = seenTags_head; list != NULL; list = list->next){
		if (strcmp(epc, list->epc) == 0 && list->antenna == t->antenna){
			list->counts = list->counts + t->readCount;
			list->rssi = t->rssi;
			list->phase = t->phase;
			list->timestampLow = t->timestampLow;
			if (t->data.len > 0 && tid_read==1) {
				list->tids++;
				free(list->tid);
				list->tid = my_strdup(tidStr);
			}
			return list;
		}
	}

	return NULL;
}

static TMR_Status db_insert(char *epc,const TMR_TagReadData *t, char *tidStr)
{
	tagdb_table *new_list;

	if ((new_list = malloc(sizeof(tagdb_table))) == NULL){
		return TMR_ERROR_OUT_OF_MEMORY;
	}
	memset(new_list,0,sizeof(tagdb_table));
	/* Insert the record now */
	new_list->epc = my_strdup(epc);
	new_list->antenna = t->antenna;
	new_list->timestampLow = t->timestampLow;
	new_list->timestampHigh = t->timestampHigh;
	new_list->rssi = t->rssi;
	new_list->tag = t->tag;
	new_list->phase = t->phase;
	new_list->counts = t->readCount;
	new_list->tids = 0;
	if (tid_read==1 && t->data.len > 0)
	{
		new_list->tid = my_strdup(tidStr);
		new_list->tids = 1;
	}

	if (!seenTags_head){
		seenTags_head = new_list;
		seenTags_end = new_list;
		first_tag_time_L = new_list->timestampLow;
		first_tag_time_H = new_list->timestampHigh;
	} else {
		seenTags_end->next = new_list;
		seenTags_end = new_list;
	}

	list_len++;

	return TMR_SUCCESS;
}

void db_free(void)
{
	tagdb_table *list, *temp;

	if (seenTags_head == NULL)
	{
		return;
	}
	/**
	* Free the memory for every item in tag_database
	*/

	pthread_mutex_lock(&tag_data_lock);
	list = seenTags_head;
	while (list != NULL)
	{
		temp = list;
		list = list->next;
		free(temp->epc);
		if (temp->tid != NULL)
			free(temp->tid);
		temp->tid = NULL;
		free(temp);
	}
	seenTags_head=NULL;
	pthread_mutex_unlock(&tag_data_lock);
}
/* return the match id */
static int listid(const char *list[], int listlen, const char *name)
{
	int i;
	for(i=0; i<listlen; i++) {
		if (list[i]) {
			if (!strcmp(name, list[i]))
				return i;
		} else {
			printf("find null element!!");
			return -1;
		}
	}
	return -1;
}

static void errx(int exitval, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);

	exit(exitval);
}

static void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
	if (TMR_SUCCESS != ret)
	{
		errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
	}
}

static void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
	char epcStr[128];
	char tidStr[256];

	TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);

	if (t->data.len>0 && tid_read ==1)
	{
		TMR_bytesToHex(t->data.list, t->data.len, tidStr);
		//printf("Background epc read: %s tid read: %s ant :%d readCount: %d \n", epcStr, tidStr, t->antenna, t->readCount);
	}
	//printf("Background read: %s\n", epcStr);
	//printf("Frequency %d %d\n",t->frequency,t->timestampLow);
	totalCount = totalCount + t->readCount;
	/**
	* If the tag is not present in the database, only then
	* insert the tag.
	*/
	if ( NULL == db_lookup(epcStr,t,tidStr))
	{
		db_insert(epcStr,t,tidStr);
		uniqueCount ++;
	}

	last_tag_time_L = t->timestampLow;
	last_tag_time_H = t->timestampHigh;
}

static void statsCallback (TMR_Reader *reader, const TMR_Reader_StatsValues* stats, void *cookie)
{
	/** Each  field should be validated before extracting the value */
	/** Currently supporting only temperature value */
	if (TMR_READER_STATS_FLAG_TEMPERATURE & stats->valid)
	{
		printf("Temperature %d(C)\n", stats->temperature);
	}
}

static void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
	fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

/* tag list should be protected before calling this function */
static void show_tag_list_data(void)
{
	tagdb_table *tmp;
	unsigned int i, timing_H_diff;
	unsigned long int timing_L_diff;
	double rate, timing_interval;
	pthread_mutex_lock(&tag_data_lock);
	if (seenTags_head!=NULL){
		tmp=seenTags_head;
		timing_H_diff=last_tag_time_H - first_tag_time_H;

		if (timing_H_diff==0) {
			timing_L_diff = last_tag_time_L - first_tag_time_L;
			timing_interval = timing_L_diff;
		}else {
			timing_L_diff = first_tag_time_L - last_tag_time_L;
			timing_interval = (0xFFFFFFFF*timing_H_diff - timing_L_diff + timing_H_diff);
		}
		rate = (double)(totalCount/timing_interval)*1000;

		printf("\n==================================================\n");
		printf("Found [%d] / [%d] tags:(%.3lf s) rate:(%lf tag/sec) \n",
				totalCount, uniqueCount, timing_interval/1000, rate);
		if (tid_read==1) {
			printf("[tag num] [antenna:counts] [rssi] [-------epc_block--------] [time stamp] [-------tid-------] [tid counts]\n");
			for (i=0;i<list_len;i++)
			{
				printf("[%7d] [%7d:%6d] [%4d] [%s] [%u] [%s] [%u]\n",i+1,tmp->antenna,tmp->counts,tmp->rssi,tmp->epc,tmp->timestampLow,tmp->tid,tmp->tids);
				tmp=tmp->next;
			}
		} else {
			printf("[tag num] [antenna:counts] [rssi] [-------epc_block--------] [time stamp] [phase]\n");
			for (i=0;i<list_len;i++)
			{
				printf("[%7d] [%7d:%6d] [%4d] [%s] [%u] [%5d]\n",i+1,tmp->antenna,tmp->counts,tmp->rssi,tmp->epc,tmp->timestampLow,tmp->phase);
				tmp=tmp->next;
			}
		}
		printf("==================================================\n");
		printf("\n");
	}
	pthread_mutex_unlock(&tag_data_lock);
}

static void* printf_tag_info(void *tmp)
{
	while(inventory_run)
	{
		show_tag_list_data();
		sleep(1);
	}
	return NULL;
}

static void setTargetName(const char *name)
{
	TMR_GEN2_Target value;
	int i;

	i = listid(targetNames, numberof(targetNames), name);
	if (i == -1)
	{
		printf("Can't parse '%s' as a Gen2 target name\n", name);
		return;
	}
	printf("Will set target name to %s\n", name);
	value = i;
	if (TMR_SUCCESS != TMR_paramSet(rp, TMR_PARAM_GEN2_TARGET, &value)) {
		printf("Set target error!\n");
	}
}

void tmr_reader_init(const char* tcp_buf)
{
	char str[64];
	char tcp[64];
	static int ret_connect =0;
	TMR_Status ret;
	TMR_Region region;
	TMR_String model;

	rp = &r;

	if (ret_connect==1){
		ret = TMR_destroy(rp);
		printf("reader connection destroy!\n");
		checkerr(rp, ret, 1, "destory reader");
	}else {
		ret = TMR_setSerialTransport("tcp", &TMR_SR_SerialTransportTcpNativeInit);
		checkerr(rp, ret, 1, "adding the custom transport scheme");
	}

	ret = TMR_setSerialTransport("tcp", &TMR_SR_SerialTransportTcpNativeInit);
	checkerr(rp, ret, 1, "adding the custom transport scheme");
	if (*tcp_buf != 't'){
		strcpy(tcp, "tcp://");
		strcat(tcp, tcp_buf);
		strcat(tcp, ":");
		strcat(tcp, "50005");
	} else {
		strcpy(tcp,tcp_buf);
	}
	printf("TCP is %s\n",tcp);

	ret = TMR_create(rp, tcp);
	checkerr(rp, ret, 1, "creating reader");

#if USE_TRANSPORT_LISTENER

	if (TMR_READER_TYPE_SERIAL == rp->readerType)
	{
		tb.listener = serialPrinter;
	}
	else
	{
		tb.listener = stringPrinter;
	}
	tb.cookie = stdout;

	TMR_addTransportListener(rp, &tb);
#endif

	ret = TMR_connect(rp);
	checkerr(rp, ret, 1, "connecting reader");
	ret_connect=1;
	region = TMR_REGION_OPEN;

	TMR_RegionList regions;
	TMR_Region _regionStore[32];
	regions.list = _regionStore;
	regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
	regions.len = 0;

	ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
	checkerr(rp, ret, __LINE__, "getting supported regions");
	//printf all region ID
	/*
	 *for (i=0;i<regions.len;i++)
	 *    printf("regions.list[%d] is %d \n",i,regions.list[i]);
	 */
	if (regions.len < 1)
	{
		checkerr(rp, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't supportany regions");
	}
	ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
	checkerr(rp, ret, 1, "setting region");

	static char *regionlist[256] = {"Unspecified region","North America","European Union","Korea","India","Japan","People's Republic of China","European Union 2","European Union 3","Korea 2","People's Republic of China(840MHZ)","Australia","New Zealand !!EXPERIMENTAL!!","Reduced FCC region","Reduced FCC region"};
	*(regionlist + 255) = "Open";
	ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
	checkerr(rp, ret, 1, "getting region");
	printf("Region ID is %s \n",regionlist[region]);

	model.value = str;
	model.max = 64;
	ret = TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
	checkerr(rp, ret, 1, "get module version model");
	printf("Module name: %s\n", model.value);

	ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SOFTWARE, &model);
	checkerr(rp, ret, 1, "getting software version");
	printf("Now firmware version: %s\n", model.value);

	rlb.listener = callback;
	rlb.cookie = NULL;

	reb.listener = exceptionCallback;
	reb.cookie = NULL;

	slb.listener = statsCallback;
	slb.cookie = NULL;

	ret = TMR_addReadListener(rp, &rlb);
	checkerr(rp, ret, 1, "adding read listener");

	ret = TMR_addReadExceptionListener(rp, &reb);
	checkerr(rp, ret, 1, "adding exception listener");
	setTargetName("AB");

	ret = TMR_addStatsListener(rp, &slb);
	checkerr(rp, ret, 1, "adding the stats listener");

	TMR_uint32List value;
	uint32_t valueList[19];
	int i,size=19;

	/* Hardcoded frequency hopping table - these are the frequencies observed on the Favite module */
	for (i = 0; i < 19; i++)
	{
		valueList[i] = 922750 + i * 250;
	}
	srand((unsigned)time(NULL));
	int j,randindex;
	for (i=0;i<size;i++)
	{
		randindex=rand()%(size-i)+i;
		j = valueList[i];
		valueList[i]=valueList[randindex];
	    valueList[randindex]=j;
	}
	value.list = valueList;
	value.len = 19;

	ret = TMR_paramSet(rp, TMR_PARAM_REGION_HOPTABLE, &value);
	if (TMR_SUCCESS != ret)
	{
		printf("fail to set hopping table %x\n",ret);
	}
#if 0
	else
	{
		/* Read back to make sure the write was successful */
		ret = TMR_paramGet(rp, TMR_PARAM_REGION_HOPTABLE, &value);
		if (TMR_SUCCESS != ret)
		{
			printf("fail\n");
		}
		else {
			putchar('[');
			for (i = 0; i < value.len; i++)
			{
				printf("%d ",value.list[i]);
			}
			putchar(']');
			printf("\n");
		}
	}
#endif
}

void tm_check_antenna(void)
{
	TMR_PortValueList rl;
	TMR_PortValue rllist[TMR_SR_MAX_ANTENNA_PORTS];
	TMR_Status ret;
	uint8_t i,num_detected_ports = 0;
	uint8_t detected_ports[TMR_SR_MAX_ANTENNA_PORTS];
#define TM_ANTENNA_DETECT_THRESHOLD 4

	rl.max = sizeof(rllist)/sizeof(rllist[0]);
	rl.list = rllist;

	ret = TMR_paramGet(rp, TMR_PARAM_ANTENNA_RETURNLOSS, &rl);

	if (TMR_SUCCESS == ret) {
		for (i = 0; i < rl.len; i++) {
			//printf("port %d value %d\n",rl.list[i].port,rl.list[i].value);
			if (rl.list[i].value >= TM_ANTENNA_DETECT_THRESHOLD) {
				detected_ports[num_detected_ports++] = rl.list[i].port;
			}
		}
		if (num_detected_ports == 0) {
			printf("Didn't detect any connected antennas on any of the ports!\n");
		}
		else {
			printf("Detected antennas on port(s) ");
			for (i = 0; i < num_detected_ports; i++) {
				printf("%d ",detected_ports[i]);
			}
			printf("\n");
		}
	}
	else
	{
		printf("Error measuring antennna port loss!\n");
	}
}

void tm_startInv(uint8_t *ant, uint8_t ant_count, uint8_t fastsearch)
{
	uint8_t i;
	short int dwell_time;
	TMR_Status ret;
	TMR_ReadPlan plan;
	uint32_t on_time = 5000;

	if (!rp) {
		printf("error! init reader first!\n");
		return;
	}

	if (inventory_run) {
		printf("error! stop inventory first!\n");
		return;
	}

	printf("will enable antenna:");
	for(i=0; i<ant_count; i++) {
		printf(" %d ", ant[i]);
	}
	dwell_time = 200;
	__ant_led_switch(ant,ant_count,dwell_time);
	ret = TMR_paramSet(rp, TMR_PARAM_READ_ASYNCONTIME, &on_time);
	checkerr(rp, ret, 1, "setting async on time");

	inventory_run = 1;
#if 1

	ret = TMR_RP_init_simple(&plan, ant_count , ant, TMR_TAG_PROTOCOL_GEN2, 10);
	checkerr(rp, ret, 1, "init simple plan");
	if (tid_read==1) {
		TMR_TagOp tagop;
		ret = TMR_TagOp_init_GEN2_ReadData(&tagop,TMR_GEN2_BANK_TID, 0, 0);
		ret = TMR_RP_set_tagop(&plan, &tagop);
	}
	ret = TMR_RP_set_useFastSearch(&plan, fastsearch);
	checkerr(rp, ret, 1, "fastsearch set");
	ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
	checkerr(rp, ret, 1, "setting read plan");
	ret = TMR_startReading(rp);
	checkerr(rp, ret, 1, "starting reading");
#else

	TMR_ReadPlan *subplans;
	TMR_ReadPlan **subplanPtrs;
	int subplanCount = 0;
	int j;
	if ((subplans = malloc(sizeof(TMR_ReadPlan)*ant_count)) == NULL){
		printf("Malloc Fail for *sublplans\n");}
	if ((subplanPtrs = malloc(sizeof(TMR_ReadPlan *)*ant_count)) == NULL){
		printf("Malloc Fail for *sublplanPtes\n");}

	for (j = 0; j < ant_count; j++)
	{
		ret = TMR_RP_init_simple(&subplans[subplanCount++],1 , &ant[j], TMR_TAG_PROTOCOL_GEN2, 10);
	}

	{
		int k;
		for (k=0; k<subplanCount; k++)
		{
			subplanPtrs[k] = &subplans[k];
		}
	}
	ret = TMR_RP_init_multi(&plan, subplanPtrs, subplanCount, 1000);
	checkerr(rp, ret, 1, "creating multi read plan");
	if (tid_read==1) {
		TMR_TagOp tagop;
		ret = TMR_TagOp_init_GEN2_ReadData(&tagop,TMR_GEN2_BANK_TID, 0, 0);
		ret = TMR_RP_set_tagop(&plan, &tagop);
	}
	ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
	checkerr(rp, ret, 1, "setting read plan");
	ret = TMR_startReading(rp);
	checkerr(rp, ret, 1, "starting reading");


#endif


	if(pthread_create(&printf_tag_thread, NULL, printf_tag_info, (void *)NULL))
	{
		printf("Create Pthread Error!\n");
		exit (1);
	}
}

void tm_stopInv(void)
{
	TMR_Status ret;
	uint8_t ant[4] = {0};

	if (!rp) {
		printf("error! init reader first!\n");
		return;
	}

	if (!inventory_run) {
		printf("error! inventory is not running!\n");
		return;
	}

	ret = TMR_stopReading(rp);
	checkerr(rp, ret, 1, "stopping reading");

	printf("Stop inventory!\n");
	inventory_run = 0;
	__ant_led_switch(ant,0,0);
	pthread_join(printf_tag_thread,NULL);
}

void tm_fw_reload(void)
{
	TMR_Status ret;
	TMR_String value;
	char value_data[64],buf[100],*filepath;
	FILE* f = NULL;

	printf("Please input the files path(included file name):");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	printf("Reload FW files from %s\n",buf);
	filepath = (char *) malloc(strlen(buf));
	//becuase "buf" last characters is "\n", we need to delete it!.
	memcpy(filepath, buf, strlen(buf)-1);
	f = fopen(filepath, "rb");
	if (NULL == f)
	{
		perror("Can't open FW reload file");
		return;
	}
	printf("Loading firmware.... it maybe wait a few minutes.\n");

	ret = TMR_firmwareLoad(rp, f, TMR_fileProvider);
	checkerr(rp, ret, 1, "loading firmware");

	value.value = value_data;
	value.max = sizeof(value_data)/sizeof(value_data[0]);

	ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SOFTWARE, &value);
	checkerr(rp, ret, 1, "getting software version");

	fclose(f);
	printf("New firmware version: %s\n", value.value);
}
uint32_t tm_getpower(void)
{
	uint32_t value;
	if (TMR_SUCCESS != TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value)) {
		printf("Get power error!\n");
	}
	return value;
}

void set_gen2_protocol(uint32_t enable)
{
	const char *encodingtable[] = {"FM0", "M2", "M4", "M8"};
	const char *blftable[] = {"250 kHz", "320 kHz", "640 kHz", "640 kHz", "640 kHz"};
	const char *taritable[] = {"25 us", "12.5 us", "6.25 us"};
	TMR_Status ret;
	TMR_GEN2_TagEncoding encode;
	TMR_GEN2_LinkFrequency blf;
	TMR_GEN2_Tari tari;

	if (enable==1){
		encode = 0;
		blf = 640;
		tari = 2;
	}
	else {
		encode = 2;
		blf = 250;
		tari = 0;
	}

	ret = TMR_paramSet(rp, TMR_PARAM_GEN2_BLF, &blf);
	checkerr(rp, ret, 1, "GEN2 protocol BLF set");
	ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TARI, &tari);
	checkerr(rp, ret, 1, "GEN2 protocol Tari set");
	ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TAGENCODING, &encode);
	checkerr(rp, ret, 1, "GEN2 protocol encoding set");
	ret = TMR_paramGet(rp, TMR_PARAM_GEN2_BLF, &blf);
	checkerr(rp, ret, 1, "GEN2 protocol BLF get");
	ret = TMR_paramGet(rp, TMR_PARAM_GEN2_TARI, &tari);
	checkerr(rp, ret, 1, "GEN2 protocol Tari get");
	ret = TMR_paramGet(rp, TMR_PARAM_GEN2_TAGENCODING, &encode);
	checkerr(rp, ret, 1, "GEN2 protocol encoding get");
	printf("\nGEN2 protocol : BLF now is %s\n", blftable[blf]);
	printf("GEN2 protocol : Tari now is %s\n", taritable[tari]);
	printf("GEN2 protocol : Encoding now is %s\n", encodingtable[encode]);
}

void tm_gettemperature(char flag)
{
	TMR_Status ret;
	TMR_Reader_StatsFlag setFlag;

	if (inventory_run) {
		printf("error! stop inventory first!\n");
		return;
	}

	if (flag==1){
		setFlag = TMR_READER_STATS_FLAG_TEMPERATURE;
	}else{
		setFlag = TMR_READER_STATS_FLAG_NONE;
	}

	/** request for the statics fields of your interest, before search */
	ret = TMR_paramSet(rp, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
	checkerr(rp, ret, 1, "setting the  fields");
}

void tm_setpower(uint32_t value)
{
	if (TMR_SUCCESS != TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &value)) {
		printf("Set power error!\n");
	}
}

void tm_clear_db(void)
{
	db_free();
	uniqueCount=totalCount=list_len=first_tag_time_H=first_tag_time_L=last_tag_time_H=last_tag_time_L=0;
}

void __set_module_reset(const char* tcp_buf)
{
	TMR_Status ret;
	uint8_t data[5],len,opcode;
	uint32_t timeoutMs=100;

	len=0;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_MODULE_RESET;

	if (inventory_run) {
		tm_stopInv();
	}
	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}
	//sleep time should >= 2 seconds
	usleep(2000000);
	tmr_reader_init(tcp_buf);
}

void __system_reboot(void)
{
	TMR_Status ret;
	uint8_t data[5],len,opcode;
	uint32_t timeoutMs=100;

	len=0;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_DEVICE_REBOOT;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}
}

void __system_update(void)
{
	TMR_Status ret;
	uint8_t data[5],len,opcode;
	uint32_t timeoutMs=100;

	len=0;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_EN_UPDATE;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}
}

void __set_network(void)
{
	TMR_Status ret;
	uint8_t data[15],len,opcode;
	uint32_t timeoutMs=100;
	unsigned int tmp;
	char inp[50];
	struct in_addr tmp_addr_ip, tmp_addr_mask, tmp_addr_gateway;

	printf("Input ip address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_ip)) {
		printf("Get wrong IP address:%s\n", inp);
		return;
	}
	printf("Input netmask address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_mask)) {
		printf("Get wrong netmask address:%s\n", inp);
		return;
	}
	printf("Input gateway address:");
	fgets(inp, sizeof(inp), stdin);
	if (!inet_aton(inp, &tmp_addr_gateway)) {
		printf("get wrong gateway address:%s\n", inp);
		return;
	}
	printf("Will set ip   to %s\n.", inet_ntoa(tmp_addr_ip));
	printf("   & set netmask to %s\n.", inet_ntoa(tmp_addr_mask));
	printf("   & set gateway to %s\n.", inet_ntoa(tmp_addr_gateway));

	len=12;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_SET_NETWORK;

	tmp = tmp_addr_ip.s_addr;
    //ip address
	data[3] = tmp & 0xff;
	data[4] = (tmp & 0xff00) >> 8;
	data[5] = (tmp & 0xff0000) >> 16;
	data[6] = (tmp & 0xff000000) >> 24;
	//netmask
	tmp = tmp_addr_mask.s_addr;
	data[7] = tmp & 0xff;
	data[8] = (tmp & 0xff00) >> 8;
	data[9] = (tmp & 0xff0000) >> 16;
	data[10] = (tmp & 0xff000000) >> 24;
    //gateway
	tmp = tmp_addr_gateway.s_addr;
	data[11] = tmp & 0xff;
	data[12] = (tmp & 0xff00) >> 8;
	data[13] = (tmp & 0xff0000) >> 16;
	data[14] = (tmp & 0xff000000) >> 24;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}
}

void __show_fw_version(void)
{
	TMR_Status ret;
	uint8_t data[8],len,opcode;
	uint32_t timeoutMs=100;

	len=0;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_GET_FW_VERSION;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}

	ret = TMR_SR_receiveMessage(rp, data, opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Receive Message time out!\n");
		return;
	}

	printf("FW Version is %d\n",data[5]);
}

void __set_gpio(uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t gpio4)
{
	TMR_Status ret;
	uint8_t data[9],len,opcode;
	uint32_t timeoutMs=100;

	len=4;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_SET_GPIO;
	data[3]=gpio1;
	data[4]=gpio2;
	data[5]=gpio3;
	data[6]=gpio4;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}
}

void __get_gpio(void)
{
	TMR_Status ret;
	uint8_t data[9],len,opcode;
	uint32_t timeoutMs=100;

	len=0;
	data[0]=0xff;
	data[1]=len;
	data[2]=CMD_GET_GPIO;

	ret = TMR_SR_sendMessage(rp, data, &opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Send Message time out!\n");
		return;
	}

	ret = TMR_SR_receiveMessage(rp, data, opcode, timeoutMs);
	if (TMR_SUCCESS != ret)
	{
		printf("Receive Message time out!\n");
		return;
	}

	printf("GPIO input1 is %d\n",data[5]);
	printf("GPIO input2 is %d\n",data[6]);
}

int write_tags(uint8_t *ant, int start, int epc_len, uint16_t *writeData, int bank)
{
#if 1
	uint8_t epcData[96];
	char buf[10];
	int i, j;
	TMR_Status ret;
	TMR_TagFilter filter;
	tagdb_table *list;
	TMR_TagData mask;
	TMR_TagOp tagop;
	TMR_uint16List data;

	ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &ant[0]);
	checkerr(rp, ret, 1, "setting tagop antenna");

	list=seenTags_head;
	show_tag_list_data();
	if (list == NULL)
	{
		printf("Run inventory before writtng tags!\n");
		return 0;
	}
	printf("Choose tag to do epc filter\n");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%d\n", &i);
	if (i < 0 || i > list_len){
		printf("Tag number is wrong!\n");
		return 0;
	}
	for (j=1;j<i;j++){
		list=list->next;
	}
	printf("Choose tag epc:[%s]\n", list->epc);
	ret = TMR_hexToBytes(list->epc, epcData, strlen(list->epc), NULL);
	checkerr(rp, ret, 1, "Hex to bytes");
	mask.epcByteCount = strlen(list->epc)/2;
	memcpy(mask.epc, epcData, mask.epcByteCount * sizeof(uint8_t));
	ret = TMR_TF_init_tag(&filter, &mask);
	checkerr(rp, ret, 1, "initializing TMR_TagFilter");

	setTargetName("A");
	data.list = writeData;
	data.max = 16;
	data.len = epc_len;
	ret = TMR_TagOp_init_GEN2_BlockWrite(&tagop, TMR_GEN2_BANK_EPC, start, &data);
	checkerr(rp, ret, 1, "creating BlockWrite tagop");
	ret = TMR_executeTagOp(rp, &tagop, &filter, NULL);
	if (!ret)
		printf("Write tag sucess!\n");
	else
		printf("Write tag fault!\n");
	setTargetName("AB");
	return 0;
}

#else
	TMR_Status ret;
	TMR_TagFilter filter;
	TMR_TagOp tagop;
	TMR_uint16List data;
	tagdb_table *list;
	uint8_t mask[96], epcData[96];
	int i, j,count;
	char buf[10], *readData;
	const char *memoryBank[] = {"RESERVED", "EPC", "TID", "USER"};

	ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &ant[0]);
	checkerr(rp, ret, 1, "setting tagop antenna");
	list=seenTags_head;
	show_tag_list_data();
	if (list == NULL)
	{
		printf("Run inventory before write tags!\n");
		return 0;
	}
	printf("Choose tag to do filter\n");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%d\n", &i);
	for (j=1;j<i;j++)
	{
		list=list->next;
	}
	if (list == NULL)
	{
		printf("Wrong tag number!\n");
		return 0;
	}
	if (bank==1)
		readData = list->epc;
	else if (bank==2)
		readData = list->tid;
	printf("Choose tag %s:[%s]\n",memoryBank[bank], readData);
	count = strlen(readData)/2;
	ret = TMR_hexToBytes(readData, epcData, count, NULL);
	checkerr(rp, ret, 1, "Hex to bytes");
	memcpy(mask, epcData,  (count)* sizeof(uint8_t));
	TMR_TF_init_gen2_select(&filter, false, bank, count, 2, mask);
	checkerr(rp, ret, 1, "initializing TMR_TagFilter");

	setTargetName("A");
	data.list = writeData;
	data.max = epc_len;
	data.len = epc_len;
	ret = TMR_TagOp_init_GEN2_BlockWrite(&tagop, TMR_GEN2_BANK_EPC, start, &data);
	checkerr(rp, ret, 1, "creating BlockWrite tagop");
	ret = TMR_executeTagOp(rp, &tagop, &filter, NULL);
	if (!ret)
		printf("Write tag sucess!\n");
	else
		printf("Write tag fault!\n");
	setTargetName("AB");
	return 0;
}
#endif

void __set_tid_read(char flag)
{
	if (inventory_run) {
		printf("error! stop inventory first!\n");
		return;
	}
	if (flag==1){
		tid_read = 1;
	}else{
		tid_read = 0;
	}
}

void tm_get_multi_power(void)
{
	int i;
	uint32_t power_value;
	TMR_PortValueList value;
	TMR_PortValue valueList[4];
	value.max = numberof(valueList);
	value.list = valueList;

	if (TMR_SUCCESS != TMR_paramGet(rp, TMR_PARAM_RADIO_PORTREADPOWERLIST, &value)) {
		printf("Get power error!\n");
	}
		for (i=0;i<4;i++)
		{
			if (value.list[i].port == (i+1))
				printf("port:%d, power:%.1lf dB\n", value.list[i].port, (double)value.list[i].value/100);
			else
			{
				TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &power_value);
				printf("port:%d, power:%.1lf dB\n", i+1, (double)power_value/100);
			}
		}
}

void tm_set_multi_power(int *powerlist)
{
	int i;
	TMR_PortValueList value;
	TMR_PortValue valueList[4];
	value.list = valueList;
	value.len = 4;
	for (i=0;i<4;i++)
	{
		value.list[i].port=i+1;
		value.list[i].value=powerlist[i];
	}

	if (TMR_SUCCESS != TMR_paramSet(rp, TMR_PARAM_RADIO_PORTREADPOWERLIST, &value)) {
		printf("Set power error!\n");
	}
}

void tm_config_op(int op, const char* tcp_buf)
{
	static char *regionlist[256] = {"Unspecified region","North America","European Union","Korea","India","Japan","People's Republic of China","European Union 2","European Union 3","Korea 2","People's Republic of China(840MHZ)","Australia","New Zealand !!EXPERIMENTAL!!","Reduced FCC region","Reduced FCC region"};
	*(regionlist + 255) = "Open";
	const char *encodingtable[] = {"FM0", "M2", "M4", "M8"};
	const char *blftable[] = {"250 kHz", "320 kHz", "640 kHz", "640 kHz", "640 kHz"};
	const char *taritable[] = {"25 us", "12.5 us", "6.25 us"};
	int value;
	TMR_Status ret;
	TMR_Region region;
	TMR_GEN2_Target target;
	TMR_GEN2_TagEncoding encode;
	TMR_GEN2_LinkFrequency blf;
	TMR_GEN2_Tari tari;
	TMR_SR_UserConfigOp config;
	enum {show =1, save, restore, clear};

	switch(op)
	{
		case show:
			ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
			checkerr(rp, ret, 1, "show region");
			/*
			 *ret = TMR_paramGet(rp, TMR_PARAM_BAUDRATE, &value);
			 *checkerr(rp, ret, 1, "show baudrate");
			 */
			ret = TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value);
			checkerr(rp, ret, 1, "show power");
			ret = TMR_paramGet(rp, TMR_PARAM_GEN2_TARGET, &target);
			checkerr(rp, ret, 1, "show gen2 target");
			ret = TMR_paramGet(rp, TMR_PARAM_GEN2_BLF, &blf);
			checkerr(rp, ret, 1, "show gen2 blf");
			ret = TMR_paramGet(rp, TMR_PARAM_GEN2_TARI, &tari);
			checkerr(rp, ret, 1, "show gen2 tari");
			ret = TMR_paramGet(rp, TMR_PARAM_GEN2_TAGENCODING, &encode);
			checkerr(rp, ret, 1, "show gen2 encode");
			printf("Region ID is %s\n", regionlist[region]);
//			printf("current baud rate is %d\n", value);
			printf("Current read power is %.1lf dB\n", (double)value/100);
			tm_get_multi_power();
			printf("GEN2 protocol : Target now is %s\n", targetNames[target]);
			printf("GEN2 protocol : BLF now is %s\n", blftable[blf]);
			printf("GEN2 protocol : Tari now is %s\n", taritable[tari]);
			printf("GEN2 protocol : Encoding now is %s\n", encodingtable[encode]);
			break;
		case save:
			ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_SAVE);
			checkerr(rp, ret, 1, "setting user configuration: save all config init");
			ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
			checkerr(rp, ret, 1, "setting user configuration: save all configuration");
			printf("User config set option:save all configuration\n");
			break;
		case restore:
			ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_RESTORE);
			checkerr(rp, ret, 1, "setting user configuration: restore all config init");
			ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
			if (TMR_SUCCESS == ret)
				printf("User config set option:restore all saved configuration params\n");
			else
				printf("User config set option:no saved config\n");
			break;
		/*
		 *case clear:
		 *    ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_CLEAR);
		 *    checkerr(rp, ret, 1, "setting user configuration: clear all config init");
		 *    ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
		 *    checkerr(rp, ret, 1, "setting user configuration: clear all configuration");
		 *    printf("User config set option:clear all saved configuration params\n");
		 *    tmr_reader_init(tcp_buf);
		 *    break;
		 */
		default :
			break;
	}
}


