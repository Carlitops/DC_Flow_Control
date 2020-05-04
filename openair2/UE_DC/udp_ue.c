/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
  AUTHOR  : Carlos Pupiales
  COMPANY : Universitat Politecnica de Catalunya
  EMAIL   : carlos.pupiales@upc.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "queue.h"
#include "ue_dc.h"
#include "intertask_interface.h"

static
void read_data_from_socket(int fd);

static
int process_udp_init(udp_init_t *udp_init_p);


static
void read_data_from_socket(int fd){
	uint8_t             l_buffer[4096];
	int                	n;
	socklen_t          	from_len;
	struct sockaddr_in 	addr;
	MessageDef          *message_p        = NULL;
	udp_data_ind_t      *udp_data_ind_p   = NULL;
	uint8_t             *forwarded_buffer = NULL;

	if (1) {
		from_len = (socklen_t)sizeof(struct sockaddr_in);

		if ((n = recvfrom(fd, l_buffer, sizeof(l_buffer), 0, (struct sockaddr *)&addr, &from_len)) < 0) {
			printf("UDP_UE, Recvfrom failed\n");
			return;
		} else if (n == 0) {
			printf("UDP_UE, Recvfrom returned 0\n");
			return;
		} else {
			forwarded_buffer = itti_malloc(TASK_UDP_UE, TASK_UE_DC, n*sizeof(uint8_t));
			memcpy(forwarded_buffer, l_buffer, n);

			message_p = itti_alloc_new_message(TASK_UDP_UE, UDP_DATA_IND);
			udp_data_ind_p = &message_p->ittiMsg.udp_data_ind;
			udp_data_ind_p->buffer        = forwarded_buffer;
			udp_data_ind_p->buffer_length = n;
			udp_data_ind_p->peer_port     = htons(addr.sin_port);
			udp_data_ind_p->peer_address  = addr.sin_addr.s_addr;

			if (itti_send_msg_to_task(TASK_UE_DC, INSTANCE_DEFAULT, message_p) < 0) {
			    printf("UDP_UE, Failed to send message to task UE_DC\n");
			    itti_free(TASK_UDP_UE, message_p);
			    itti_free(TASK_UDP_UE, forwarded_buffer);
			    return;
			}
		}
	}
}


static
int process_udp_init(udp_init_t *udp_init_p){
	struct sockaddr_in	sin;
	int sock; //socket descriptor

	printf("UDP_UE, Initializing UDP for local address %s with port %u\n", udp_init_p->address, udp_init_p->port);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		printf("UDP_UE,Failed to create new socket\n");
		return sock;
	}
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(udp_init_p->port);
	sin.sin_addr.s_addr = inet_addr(udp_init_p->address);

	if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
	    close(sock);
	    printf("UDP_UE, Failed to bind socket for address %s and port %d\n", udp_init_p->address, udp_init_p->port);
	}
	itti_subscribe_event_fd(TASK_UDP_UE, sock);
	printf("UDP_UE, Initializing UDP for local address %s with port %d: DONE\n", udp_init_p->address, udp_init_p->port);
	return sock;
}


void *udp_ue_task(void *arg){
	MessageDef	*received_msg;
	struct epoll_event *events;
	int fd = -1; //descriptor for socket
	int udp_events = 0;

	printf("UDP_UE, Starting UDP_UE task\n");
	itti_mark_task_ready(TASK_UDP_UE);

	while (1) {
		itti_receive_msg(TASK_UDP_UE, &received_msg);
		if (received_msg != NULL) {
			switch (ITTI_MSG_ID(received_msg)) {

			case TERMINATE_MESSAGE:
				itti_exit_task();
				break;

			case UDP_INIT:
				fd = process_udp_init(&UDP_INIT(received_msg));
				break;

			case UDP_DATA_REQ:
			{
				ssize_t bytes_written;
				udp_data_req_t           *udp_data_req_p;
				struct sockaddr_in        peer_addr;

				udp_data_req_p = &received_msg->ittiMsg.udp_data_req;
				memset(&peer_addr, 0, sizeof(struct sockaddr_in));
				peer_addr.sin_family       = AF_INET;
				peer_addr.sin_port         = htons(udp_data_req_p->peer_port);
				peer_addr.sin_addr.s_addr  = udp_data_req_p->peer_address;

				bytes_written = sendto(fd, &udp_data_req_p->buffer[udp_data_req_p->buffer_offset],
						udp_data_req_p->buffer_length, 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));

				if (bytes_written != udp_data_req_p->buffer_length) {
					printf("UDP_UE, There was an error while writing to socket %d\n", fd);
				}
				itti_free(ITTI_MSG_ORIGIN_ID(received_msg), udp_data_req_p->buffer);
			}

				break;

			}
			itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
			received_msg = NULL;
		}

		udp_events = itti_get_events(TASK_UDP_UE, &events);
		int i=0;
		for (i=0; i < udp_events; i++){
			if (fd == events[i].data.fd){
			 	read_data_from_socket(fd);
			   	break;
			}
		}
	}
	return NULL;
}
