/**
 *@file
 *@brief RFID reader Server application
 *
 *This is a socket server program to control
 *the Delta DRC RFID reader.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "default_val.h"
#include "favite_llc.h"
#include "module.h"

pthread_mutex_t write_socket_lock;

int write_to_client(int sockfd, const unsigned char *buf, int len)
{
	int ret;

	pthread_mutex_lock(&write_socket_lock);
	printf("\n");
	if (debug_flag) {
		print_buf("Write data:", buf, len);
		printf("\n");
	}
	ret = write(sockfd, buf, len);
	pthread_mutex_unlock(&write_socket_lock);
	return ret;
}

/**
 * A server thread to handle the client's request
 *
 * This thread will run the function by the client's request.\n
 * It will keep processing until receiving terminate command.
 *
 * @param sockfd client's fd
 */
void *server_handle_thread(void *parameter)
{
	int client_fd = *(int *)parameter;
	int n, i;
	unsigned char *buf;
	char b_exit;

	buf = malloc(NETWORK_BUFFER_SIZE);
	if (!buf) {
		printf("out of memory!!\n");
		goto exit;
	}
	open_module(client_fd);
	b_exit = false;
	while(!b_exit) {
		bzero(buf, NETWORK_BUFFER_SIZE);
		n = read(client_fd, buf, NETWORK_BUFFER_SIZE);
		if (n <= 0) {
			printf("ERROR reading to socket");
			break;
		}
		if (process_buf_svr(client_fd, buf, n) < 0)
			b_exit = true;
	}
exit:
	printf("close socket fd:%d\n", client_fd);
	close(client_fd);
	close_module();
}

/**
 * An infinite loop server listening function
 *
 * If a client connecting, it will create a thread to
 * handle server/client communication.
 *
 * @param sockfd a connected sockfd
 */
void running_server(int sockfd)
{
	int n;
	struct sockaddr_in client_addr;
	socklen_t len;
	int client_fd;
	pthread_t thread_id;
	char buffer[sizeof(client_addr)];

	if (pthread_mutex_init(&write_socket_lock, NULL) != 0) {
		printf("thread mutex init failed!!");
	}
	while(1) {
		if (-1 == listen(sockfd, NETWORK_LISTEN_NUM)) {
			printf("network listen error!\n");
			return;
		}
		printf("Waiting connection...\n");
		len = sizeof(client_addr);
		client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &len);
		if (client_fd < 0)
			return;
		else
			inet_ntop(AF_INET, &(client_addr.sin_addr), buffer,
					INET_ADDRSTRLEN);
		printf("Got connection from %s, port %d, fd %d.\n"
				,buffer, ntohs(client_addr.sin_port), client_fd);
		if (client_fd < 0) {
			printf("ERROR on accept");
			continue;
		}
		pthread_create(&thread_id, NULL, (void *)*server_handle_thread, &client_fd);
	}
	pthread_mutex_destory(&write_socket_lock);
	close(client_fd);
}

/**
 * Start network socket
 *
 * @param port port number
 * @return
 * 	the connected socket fd\n
 * 	return -1 if connected failed
 */
int start_server(int port)
{
	struct sockaddr_in serv_addr;
	int sockfd;

	printf("Start server on port %d...\n", port);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return sockfd;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding");
		return -1;
	}
	return sockfd;
}

void usage()
{
	printf("reader_srv [-p port_num]\n");
}

int main(int argc, char *argv[])
{
	int opt, net_port, sockfd;

	printf("server version: %d.%d .\n", SERVER_VER_MAJOR, SERVER_VER_MINOR);
	net_port = DEFAULT_NETPORT;
	while((opt = getopt(argc, argv, "p:h")) != -1 ) {
		switch(opt) {
			case 'p':
				net_port = atoi(optarg);
				break;
			case 'h':
			case '?':
				usage();
				break;
		}
	}
	sockfd = start_server(net_port);
	if (sockfd < 0) {
		printf("open network socket error!\n");
		exit(1);
	}
	debug_flag = 0;
	running_server(sockfd);
	close(sockfd);
}
