#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define BUF_LEN 256
#define IF_NAME "eth0"

struct connect_par {
	int sockfd;
	int portno;
	struct in_addr tmp_ip;
};

static int connect_success;
static struct in_addr device_ip;

//return 0: failed
int connect_ok(int sockfd, int portno, struct in_addr *tmp)
{
	struct sockaddr_in dest;

	bzero((char *) &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(portno);
	dest.sin_addr.s_addr = tmp->s_addr;
	return connect(sockfd, (struct sockaddr *)&dest, sizeof(dest));
}

void *connect_ok_thread(void *parameter)
{
	struct connect_par *tmp = parameter;
	struct sockaddr_in dest;

	bzero((char *) &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(tmp->portno);
	dest.sin_addr.s_addr = tmp->tmp_ip.s_addr;
	printf("scan ip : %s port: %d\n", inet_ntoa(dest.sin_addr), tmp->portno);
	if (!(connect(tmp->sockfd, (struct sockaddr *)&dest, sizeof(dest)) < 0)) {
		connect_success = 1;
		device_ip.s_addr = tmp->tmp_ip.s_addr;
		printf("---------------found!\n");
	}
}

int main(int argc, char *argv[])
{
	int sockfd, portno, i;
	struct ifreq ifr_mask;
	struct ifreq ifr_ip;
	struct sockaddr_in *net_mask;
	struct sockaddr_in *ip_addr;
	struct in_addr tmp_ip;
	struct connect_par par[255];
	pthread_t thread_id[255];

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	portno = atoi(argv[1]);
	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("ERROR opening socket");
		return 0;
	}
	memset(&ifr_mask, 0, sizeof(ifr_mask));
	strncpy(ifr_mask.ifr_name, IF_NAME, sizeof(ifr_mask.ifr_name )-1);
	if((ioctl(sockfd, SIOCGIFNETMASK, &ifr_mask)) < 0) {
		printf("getmask ioctl error/n");
		goto exit;
	}
	net_mask = (struct sockaddr_in *) &(ifr_mask.ifr_netmask);
	memset(&ifr_ip, 0, sizeof(ifr_ip));
	strncpy(ifr_ip.ifr_name, IF_NAME, sizeof(ifr_ip.ifr_name)-1);
	if((ioctl(sockfd, SIOCGIFADDR, &ifr_ip)) < 0 ) {
		printf("getip ioctl error/n");
		goto exit;
	}
	ip_addr = (struct sockaddr_in *) &(ifr_ip.ifr_addr);
	printf("local ip:%s, connect port:%d\n",inet_ntoa(ip_addr->sin_addr), portno);
	tmp_ip.s_addr = net_mask->sin_addr.s_addr & ip_addr->sin_addr.s_addr;
	for(i=1; i<253; i++) {
		tmp_ip.s_addr = (tmp_ip.s_addr & 0x00ffffff) | i<<24;
		if (!connect_success) {
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0) {
				printf("ERROR opening socket");
				return 0;
			}
			par[i].portno = portno;
			par[i].sockfd = sockfd;
			par[i].tmp_ip.s_addr = tmp_ip.s_addr;
			pthread_create(&thread_id[i], NULL, (void *)*connect_ok_thread, &par[i]);
		} else {
			printf("Find device!!! ip:%s\n", inet_ntoa(device_ip));
			break;
		}
		/*
		 *printf("scan ip : %s\n", inet_ntoa(tmp_ip));
		 *if (!(connect_ok(sockfd, portno, &tmp_ip) < 0)) {
		 *    printf("Find device!! ip: %s\n", inet_ntoa(tmp_ip));
		 *    break;
		 *}
		 */
	}
	for(i=90; i<254; i++)
		pthread_join(thread_id[i], NULL);
exit:
	close(sockfd);
	return 0;
}
