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

#include <string.h>
#include "x2u_procedures.h"
#include "enb_config.h"
#include "intertask_interface.h"
#include "common/ran_context.h"
#include "rlc.h"

extern RAN_CONTEXT_t RC;

static
void udp_init_for_dc(void);

static
void x2u_handle_dc_data_req(x2u_dc_data_req_t *x2u_dc_data_req_p);

static
void x2u_handle_dc_data_ind(udp_data_ind_t *x2u_dc_data_ind_p);

// Creation of UDP sockets for DC
static
void udp_init_for_dc(void){
	MessageDef	*msg_p;

	printf("X2U, Let's create UDP Socket for %s\n", RC.dc_eNB_dataP->local_eNB_address);

	msg_p = itti_alloc_new_message(TASK_X2U, UDP_INIT);
	if (msg_p != NULL) {
		UDP_INIT(msg_p).port 	= RC.dc_eNB_dataP->x2u_port;
		UDP_INIT(msg_p).address = RC.dc_eNB_dataP->local_eNB_address;

		if (itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, msg_p) != 0) {
			printf("X2U,The socket for X2U has not been created\n");
		}
	} else
		printf("X2U, Error allocating the message udp_init\n");
}

int is_DC_enabled(void){
	int ret;

	if(RCconfig_DC()){
		ret = 1;
		printf("Dual Connectivity is enabled\n");
	} else {
		printf("Dual Connectivity is not enabled\n");
		ret = 0;
	}

return ret;
}

void x2u_init_for_dc(char local_ip_address[16]){
	 MessageDef		*msg_p = NULL;
	 dc_eNB_data_t	*x2u_dc_data_p;

	 msg_p = itti_alloc_new_message(TASK_X2AP, X2U_DC_INIT);
	 if (msg_p != NULL){
		 x2u_dc_data_p = &msg_p->ittiMsg.x2u_dc_init;
 		 strcpy(x2u_dc_data_p->local_eNB_address, local_ip_address);

 		itti_send_msg_to_task(TASK_X2U, INSTANCE_DEFAULT, msg_p);
	 } else
		printf("Error, it's not possible to allocate message for TASK_X2U");
}

static
void x2u_handle_dc_data_req(x2u_dc_data_req_t *x2u_dc_data_req_p){
	/* Transmitter side: Send PDU from MeNB -> SeNB through UDP */
	MessageDef 		*message_p = NULL;
	udp_data_req_t	*udp_data_req_p = NULL;
	unsigned char 	*payload = NULL;

	payload = (unsigned char *)malloc(x2u_dc_data_req_p->pdu_size_dc);
	memcpy(payload, x2u_dc_data_req_p->pdu_buffer_dcP, x2u_dc_data_req_p->pdu_size_dc);

	message_p = itti_alloc_new_message(TASK_X2U, UDP_DATA_REQ);
	udp_data_req_p = &message_p->ittiMsg.udp_data_req;
	udp_data_req_p->peer_port		= RC.dc_eNB_dataP->x2u_port;
	udp_data_req_p->peer_address 	= inet_addr(RC.dc_eNB_dataP->remote_eNB_address);
	udp_data_req_p->buffer 			= payload;
	//udp_data_req_p->buffer 			= x2u_dc_data_req_p->pdu_buffer_dcP;
	udp_data_req_p->buffer_length 	= (uint32_t)x2u_dc_data_req_p->pdu_size_dc;
	udp_data_req_p->buffer_offset 	= 0;

	itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
}

static
void x2u_handle_dc_data_ind(udp_data_ind_t *x2u_dc_data_ind_p){
	/* Receiver side: Receive from UDP and send to RLC SeNB*/
	// SB = Split Bearer
	mem_block_t	*SB_pdu_SeNB = NULL;

	SB_pdu_SeNB = get_free_mem_block(x2u_dc_data_ind_p->buffer_length, __func__);

	assert(SB_pdu_SeNB != NULL && "X2U, Error forwarding Split Bearer to RLC in SeNB\n");

	memcpy(SB_pdu_SeNB->data, x2u_dc_data_ind_p->buffer, x2u_dc_data_ind_p->buffer_length);
	rlc_data_req(&RC.dc_eNB_dataP->ctxt, FALSE, FALSE, RC.dc_eNB_dataP->drb_id, RLC_MUI_UNDEFINED, RLC_SDU_CONFIRM_NO,
			x2u_dc_data_ind_p->buffer_length, SB_pdu_SeNB, NULL, NULL);
}

void *x2u_eNB_task(void *arg){
	MessageDef *rcv_msg = NULL;

	printf("X2U, Starting TASK_X2U\n");
	itti_mark_task_ready(TASK_X2U);

	while(1){
		itti_receive_msg(TASK_X2U, &rcv_msg);

		switch (ITTI_MSG_ID(rcv_msg)){

		case TERMINATE_MESSAGE:
			printf("X2U, Leaving the TASK_X2U\n");
			itti_exit_task();
			break;

		case X2U_DC_INIT:
			strcpy(RC.dc_eNB_dataP->local_eNB_address,rcv_msg->ittiMsg.x2u_dc_init.local_eNB_address);
			udp_init_for_dc();

			break;

		case X2U_DC_DATA_REQ:
			x2u_handle_dc_data_req(&X2U_DC_DATA_REQ(rcv_msg));

			break;

		case UDP_DATA_IND:
			x2u_handle_dc_data_ind(&rcv_msg->ittiMsg.udp_data_ind);

			break;

		default:
			printf("X2U, Received unhandled message\n");
			break;

		}
		itti_free(ITTI_MSG_ORIGIN_ID(rcv_msg), rcv_msg);
		rcv_msg = NULL;
	}

	return NULL;
}
