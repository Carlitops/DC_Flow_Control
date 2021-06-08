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
 *
 * \file udp_ue_dc.c
 * \brief task for handle udp messages in UE
 * \author Carlos Pupiales
 * \company UPC
 * \email: carlos.pupiales.upc.edu
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
#include "intertask_interface.h"
#include "assertions.h"
#include "ue_dc.h"
#include "common/utils/LOG/log.h"

//function prototyping

static
int udp_ue_create_socket(uint32_t	port, char	*address);

void udp_ue_recv_data(int sd);

void *udp_ue_dc_task(void *arg);

//function declaration

void *udp_ue_dc_task(void *arg){
	MessageDef *received_msg = NULL;
	struct epoll_event *events;
	int sd = -1; //descriptor for socket
	int udp_events = 0;

	LOG_D(UDP_UE_DC,"Starting UDP_UE_DC task\n");
	itti_mark_task_ready(TASK_UDP_UE_DC);

	  while(1) {
	    itti_receive_msg(TASK_UDP_UE_DC, &received_msg);

	    if (received_msg != NULL) {

	      switch (ITTI_MSG_ID(received_msg)) {

	      case TERMINATE_MESSAGE:
	      		LOG_W(UDP_UE_DC," *** Exiting UDP_UE_DC thread\n");
	      		itti_exit_task();
	      		break;

	      case UDP_INIT: {
	        LOG_I(UDP_UE_DC, "Received UDP_INIT\n");
	        sd = udp_ue_create_socket(received_msg->ittiMsg.udp_init.port, received_msg->ittiMsg.udp_init.address);
	      	}
	      	  break;

	      case UDP_DATA_REQ: {
	        ssize_t bytes_written;
	        udp_data_req_t           *udp_data_req_p;
	        struct sockaddr_in        peer_addr;

	        udp_data_req_p = &received_msg->ittiMsg.udp_data_req;
	        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
	        peer_addr.sin_family       = AF_INET;
	        peer_addr.sin_port         = htons(udp_data_req_p->peer_port);
	        peer_addr.sin_addr.s_addr  = udp_data_req_p->peer_address;

	        bytes_written = sendto(sd, &udp_data_req_p->buffer[udp_data_req_p->buffer_offset],
	                          udp_data_req_p->buffer_length,
	                          0,
	                          (struct sockaddr *)&peer_addr,
	                          sizeof(struct sockaddr_in));

	        if (bytes_written != udp_data_req_p->buffer_length) {
	          LOG_E(UDP_UE_DC, "There was an error while writing to socket %d\n", sd);
	        }
	        itti_free(ITTI_MSG_ORIGIN_ID(received_msg), udp_data_req_p->buffer);
	      }

	      	  break;

	      default: {
	        LOG_W(UDP_UE_DC, "Unkwnon message ID %d:%s\n",
	              ITTI_MSG_ID(received_msg),
	              ITTI_MSG_NAME(received_msg));
	      	  }

	      	  break;
	      }
	      itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
	   	  received_msg = NULL;
	    }

	    udp_events = itti_get_events(TASK_UDP_UE_DC, &events);
	    int i=0;
	    for (i=0;i<udp_events;i++){
	    	if (sd == events[i].data.fd){
	    		    	udp_ue_recv_data(sd);
	    		    	break;
	    		    }
	    	}
	   }

	  LOG_I(UDP_UE_DC, "Task UDP UE exiting\n");
	  return NULL;
}



static
int udp_ue_create_socket(uint32_t	port, char	*address) {
	/*This function handle the creation of udp sockets for Dual Connectivity in UEs*/

	struct sockaddr_in	sin;
	int sock; //socket descriptor

	LOG_I(UDP_UE_DC, "Initializing UDP for local address %s with port %u\n", address, port);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		LOG_E(UDP_UE_DC,"Failed to create new socket\n");
	}
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(port);
	sin.sin_addr.s_addr = inet_addr(address);

	if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
	    close(sock);
	    LOG_E(UDP_UE_DC, "Failed to bind socket for address %s and port %d\n", address, port);
	  }
	  itti_subscribe_event_fd(TASK_UDP_UE_DC, sock);
	  LOG_I(UDP_UE_DC, "Initializing UDP for local address %s with port %d: DONE\n", address, port);
	  return sock;
}

void udp_ue_recv_data(int sd){

	  uint8_t                   l_buffer[2048];
	  int                n;
	  socklen_t          from_len;
	  struct sockaddr_in addr;
	  MessageDef               *message_p        = NULL;
	  udp_data_ind_t           *udp_data_ind_p   = NULL;
	  uint8_t                  *forwarded_buffer = NULL;

	  if (1) {
	    from_len = (socklen_t)sizeof(struct sockaddr_in);

	    if ((n = recvfrom(sd, l_buffer, sizeof(l_buffer), 0,
	                      (struct sockaddr *)&addr, &from_len)) < 0) {
	      LOG_E(UDP_UE_DC,"Recvfrom failed\n");
	      return;
	    } else if (n == 0) {
	      LOG_W(UDP_UE_DC, "Recvfrom returned 0\n");
	      return;
	    } else {
	      forwarded_buffer = itti_malloc(TASK_UDP_UE_DC, TASK_UE_DC, n*sizeof(uint8_t));
	      memcpy(forwarded_buffer, l_buffer, n);
	      message_p = itti_alloc_new_message(TASK_UDP_UE_DC, UDP_DATA_IND);
	      udp_data_ind_p = &message_p->ittiMsg.udp_data_ind;
	      udp_data_ind_p->buffer        = forwarded_buffer;
	      udp_data_ind_p->buffer_length = n;
	      udp_data_ind_p->peer_port     = htons(addr.sin_port);
	      udp_data_ind_p->peer_address  = addr.sin_addr.s_addr;

	      if (itti_send_msg_to_task(TASK_UE_DC, INSTANCE_DEFAULT, message_p) < 0) {
	        LOG_E(UDP_UE_DC, "Failed to send message to task UDP_UE_DC\n");
	        itti_free(TASK_UDP_UE_DC, message_p);
	        itti_free(TASK_UDP_UE_DC, forwarded_buffer);
	        return;
	      }
	      LOG_I(UDP_UE_DC, "Message has been forwarded to TASK_UE_DC\n");
	    }
	  }
	}
