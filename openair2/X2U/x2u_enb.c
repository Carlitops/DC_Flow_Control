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

/*! \file x2u_enb.c
 * \brief manage the procedures for Dual Connectivity
 * \author Carlos Pupiales
 * \date 2019
 * \email carlos.pupiales@upc.edu
 * \version 1.0
 */

#include "x2u_enb.h"
#include "pdcp.h"
#include "LAYER2/RLC/rlc.h"
#include "common/utils/LOG/log.h"
#include <inttypes.h>
#include "platform_types.h"
#include "intertask_interface.h"
#include "assertions.h"
#include "enb_paramdef.h"
#include "common/ran_context.h"
#include "strings.h"
#include "common/config/config_userapi.h"
#include "pdcp_flow_control.h"

extern RAN_CONTEXT_t RC;

static
void dc_init_udp(dc_eNB_data_t *dc_enb_init_p);

static
void x2u_pdu_to_udp(dc_enb_data_req_t *dc_enb_data_req_p);

static
void handle_tbs_update_from_mac(flow_control_data_t *fc_tbs_x2u);

int is_dc_enabled(void);

static
void x2u_send_pdu_to_rlc(udp_data_ind_t *udp_data_ind);

void *x2u_enb_task(void *arg) {
	MessageDef *received_msg = NULL;
	//protocol_ctxt_t		new_ctxt;
	//rb_id_t	eps_bearerID;//revisar

	LOG_I(X2U,"Starting X2U task\n");
	itti_mark_task_ready(TASK_X2U);

	while (1) {
		itti_receive_msg(TASK_X2U, &received_msg);

		switch (ITTI_MSG_ID(received_msg)) {

		case TERMINATE_MESSAGE:
			LOG_W(X2U," *** Exiting X2U thread\n");
			itti_exit_task();
			break;

		case DC_ENB_INIT:
		{
			RC.dc_enb_dataP->eNB_type  	= received_msg->ittiMsg.dc_enb_init.eNB_type;
			RC.dc_enb_dataP->flow_control_type = received_msg->ittiMsg.dc_enb_init.flow_control_type;
			RC.dc_enb_dataP->x2u_port 	= received_msg->ittiMsg.dc_enb_init.x2u_port;
			memcpy(RC.dc_enb_dataP->local_eNB_address, received_msg->ittiMsg.dc_enb_init.local_eNB_address,
					sizeof(received_msg->ittiMsg.dc_enb_init.local_eNB_address));
			memcpy(&RC.dc_enb_dataP->remote_eNB_address, &received_msg->ittiMsg.dc_enb_init.remote_eNB_address,
								sizeof(received_msg->ittiMsg.dc_enb_init.remote_eNB_address));
			dc_init_udp(&received_msg->ittiMsg.dc_enb_init);

		  //Flow control
			if (RC.dc_enb_dataP->eNB_type == MeNB){
			  list_init(&flow_control_buffer, NULL);
			  if (pthread_mutex_init(&RC.dc_enb_dataP->FCbuffer, NULL) != 0){; //initialize mutex to avoid read/write errors for temporal buffer
			  		printf("Mutex initialization for Flow Control buffer has failed\n");
			  	}

			}
		}

			break;

		case GTPV1U_ENB_GET_CTXT:
			RC.dc_enb_dataP->ctxt.module_id = GTPV1U_ENB_GET_CTXT(received_msg).enb_id_for_DC;
			RC.dc_enb_dataP->ctxt.enb_flag	= ENB_FLAG_YES;
			RC.dc_enb_dataP->ctxt.rnti		= GTPV1U_ENB_GET_CTXT(received_msg).ue_id_for_DC;
			RC.dc_enb_dataP->ctxt.frame		= 0;
			RC.dc_enb_dataP->ctxt.subframe	= 0;
			RC.dc_enb_dataP->ctxt.eNB_index	= GTPV1U_ENB_GET_CTXT(received_msg).enb_id_for_DC;
			RC.dc_enb_dataP->drb_id			= GTPV1U_ENB_GET_CTXT(received_msg).eps_bearer_id_for_DC;

		    break;

		case DC_ENB_DATA_REQ:
			x2u_pdu_to_udp(&received_msg->ittiMsg.dc_enb_data_req);

			break;


		case UDP_DATA_IND:

			if (RC.dc_enb_dataP->eNB_type == SeNB){
				//Now in SeNB, then Forward the PDU to RLC
				x2u_send_pdu_to_rlc(&received_msg->ittiMsg.udp_data_ind);

			} else {
				//Now in MeNB, send the TBS update coming from SeNB to PDCP sub-task
				MessageDef		*itti_msg_p = NULL;
				uint32_t		*rcv_data = NULL;
				rcv_data = (uint32_t *)malloc(sizeof(uint32_t));

				itti_msg_p	=	itti_alloc_new_message(TASK_X2U, FC_TBS_UPDATE);
				memcpy(rcv_data, (uint32_t *)&received_msg->ittiMsg.udp_data_ind.buffer[0], sizeof(uint32_t));
				FC_TBS_UPDATE(itti_msg_p).avg_tbs = *rcv_data;
				memcpy(rcv_data, (uint16_t *)&received_msg->ittiMsg.udp_data_ind.buffer[sizeof(uint32_t)], sizeof(uint16_t));
				FC_TBS_UPDATE(itti_msg_p).avg_system_time		= *rcv_data;
				FC_TBS_UPDATE(itti_msg_p).tbs_source 			= 2; //SeNB
				if (itti_send_msg_to_task(TASK_PDCP_ENB, INSTANCE_DEFAULT, itti_msg_p) != 0){
					printf("X2U, Error sending TBS UPDATE\n");
				}

			}

			break;

		case FC_TBS_X2U:

			handle_tbs_update_from_mac(&received_msg->ittiMsg.fc_tbs_x2u);

			break;



		default:
			LOG_E(X2U, "Received unhandled message: %d:%s\n", ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
			break;
		}

		itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
		received_msg = NULL;
	}
	return NULL;
}

static
void x2u_send_pdu_to_rlc(udp_data_ind_t *udp_data_ind){
	/*This function forward the incoming pdcp_pdu to rlc_data_req in SeNB*/

	mem_block_t	*pdcp_pdu_SeNB_p = NULL;
	uint16_t	pdcp_pdu_SeNB_size = 0;
	rlc_op_status_t status;

	pdcp_pdu_SeNB_size = (unsigned short)udp_data_ind->buffer_length;
	pdcp_pdu_SeNB_p = (mem_block_t *)malloc(pdcp_pdu_SeNB_size + sizeof(mem_block_t));
	pdcp_pdu_SeNB_p->next = NULL;
	pdcp_pdu_SeNB_p->previous = NULL;
	pdcp_pdu_SeNB_p->data = ((unsigned char *)pdcp_pdu_SeNB_p) + sizeof(mem_block_t);
	pdcp_pdu_SeNB_p->size = pdcp_pdu_SeNB_size;

	memcpy(pdcp_pdu_SeNB_p->data, udp_data_ind->buffer, pdcp_pdu_SeNB_size);
	status = rlc_data_req(&RC.dc_enb_dataP->ctxt, FALSE, FALSE, 1, 0, SDU_CONFIRM_NO, pdcp_pdu_SeNB_size, pdcp_pdu_SeNB_p, NULL, NULL);
	if(status){
		LOG_D(X2U, "PDCP_PDU has been forwarded successfully to rlc_data_req in SeNB\n");
	}else{
		LOG_E(X2U, "Error forwarding PDCP_PDU to rlc_data_req\n");
	}

}

static
void dc_init_udp(dc_eNB_data_t *dc_enb_init_p){
	/*This function handle the creation of udp sockets for Dual Connectivity in eNBs*/
	MessageDef	*message_p;

	message_p = itti_alloc_new_message(TASK_X2U, UDP_INIT);
	if (message_p == NULL) {
		LOG_E(X2U,"Error allocating the message udp_init\n");
	}
	UDP_INIT(message_p).port = dc_enb_init_p->x2u_port;
	UDP_INIT(message_p).address = dc_enb_init_p->local_eNB_address;
	if (itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p) != 0) {
			LOG_E(X2U,"The socket for X2U has not been created\n");
	}
}

int is_dc_enabled(void){
	paramdef_t	DCParams[] = DCPARAMS_DESC;
	int dc_enabled;

	RC.dc_enb_dataP =(dc_eNB_data_t *)malloc(sizeof(dc_eNB_data_t));
	config_get(DCParams, sizeof(DCParams)/sizeof(paramdef_t), ENB_CONFIG_STRING_DC_CONFIG);
	if (strcasecmp(*(DCParams[DC_ENABLED_IDX].strptr), "yes") == 0){
		RC.dc_enb_dataP->enable 	  = TRUE;
		dc_enabled = 1;
	}else {
		dc_enabled = 0;
		RC.dc_enb_dataP->enable 	  = FALSE;
	}
	//cqi hack
	RC.dc_enb_dataP->fc_data_mac.cqi_vector.cqi_granularity = *(DCParams[DC_CQI_HACK_IDX].uptr);
	if(RC.dc_enb_dataP->fc_data_mac.cqi_vector.cqi_granularity != 0){
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.cqi_hack_enabled  = TRUE;
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.index  = 0;
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time = 0;
		if(strcasecmp(*(DCParams[DC_ENB_TYPE_IDX].strptr), "menb") == 0){
			RC.dc_enb_dataP->eNB_type = 1;
		}else {
			RC.dc_enb_dataP->eNB_type = 2;
		}
	} else
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.cqi_hack_enabled  = FALSE;

return dc_enabled;
}

static
void x2u_pdu_to_udp(dc_enb_data_req_t *dc_enb_data_req_p){
	/*This function sends the pdcp_pdu from MeNB to SeNB through udp socket*/
	MessageDef		*message_p = NULL;
	udp_data_req_t	*udp_data_req_p;
	unsigned char 	*buffer;

	buffer = (unsigned char *)malloc(dc_enb_data_req_p->pdu_size_dc);
	memcpy(buffer, dc_enb_data_req_p->pdu_buffer_dcP, dc_enb_data_req_p->pdu_size_dc);
	message_p = itti_alloc_new_message(TASK_X2U, UDP_DATA_REQ);
	udp_data_req_p = &message_p->ittiMsg.udp_data_req;
	udp_data_req_p->peer_address  = inet_addr(RC.dc_enb_dataP->remote_eNB_address);
	udp_data_req_p->peer_port 	  = RC.dc_enb_dataP->x2u_port;
	udp_data_req_p->buffer 		  = buffer;
	udp_data_req_p->buffer_length = (uint32_t)dc_enb_data_req_p->pdu_size_dc;
	udp_data_req_p->buffer_offset = 0;
	if(itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p) == 0){
		LOG_D(X2U, "PDU has been forwarded to peer eNB\n");
	}else {
		LOG_E(X2U, "Message didn't send to peer eNB\n");
	}
}

static
void handle_tbs_update_from_mac(flow_control_data_t *fc_tbs_x2u){
	//It receives the TBS from MAC and forwards to X2U in MeNB
	MessageDef		*message_p = NULL;
	udp_data_req_t	*udp_data_req_p = NULL;
	unsigned char 	*buffer;
	//int32_t			*temp_data = NULL;

	buffer = (unsigned char *)malloc(sizeof(uint32_t) + sizeof(fc_tbs_x2u->avg_system_time));
	memcpy(&buffer[0], &fc_tbs_x2u->avg_tbs, sizeof(fc_tbs_x2u->avg_tbs));
	memcpy(&buffer[sizeof(fc_tbs_x2u->avg_tbs)], &fc_tbs_x2u->avg_system_time, sizeof(fc_tbs_x2u->avg_system_time));

	message_p = itti_alloc_new_message(TASK_X2U, UDP_DATA_REQ);

	udp_data_req_p = &message_p->ittiMsg.udp_data_req;
	udp_data_req_p->peer_address  = inet_addr(RC.dc_enb_dataP->remote_eNB_address);
	udp_data_req_p->peer_port 	  = RC.dc_enb_dataP->x2u_port;
	udp_data_req_p->buffer 		  = buffer;
	udp_data_req_p->buffer_length = sizeof(fc_tbs_x2u->avg_tbs) + sizeof(fc_tbs_x2u->avg_system_time);
	udp_data_req_p->buffer_offset = 0;
	if(itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p) != 0){
		LOG_E(X2U, "TBS update was not sent to MeNB\n");
	}

}
