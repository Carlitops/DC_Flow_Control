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
#include "ue_dc.h"
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "intertask_interface.h"
#include "pdcp.h"
#include "ue_dc_messages_types.h"
#include "pdcp_reordering.h"


extern RAN_CONTEXT_t RC;

static
void init_ue_for_dc(void);

static
void process_dc_ue_data_req(dc_ue_data_req_t *dc_ue_data_reqP);

static
void process_dc_udp_data_ind(udp_data_ind_t	*udp_data_indP);

/////

void *ue_dc_task(void *arg){
	MessageDef	*received_msg;

	itti_mark_task_ready(TASK_UE_DC);
	init_ue_for_dc();

	while (1) {
		itti_receive_msg(TASK_UE_DC, &received_msg);

		switch (ITTI_MSG_ID(received_msg)) {

		case TERMINATE_MESSAGE:
			itti_exit_task();
			break;


		case UDP_DATA_IND:
			process_dc_udp_data_ind(&received_msg->ittiMsg.udp_data_ind);

			break;

		case DC_UE_DATA_REQ:
			process_dc_ue_data_req(&DC_UE_DATA_REQ(received_msg));

			break;

		}
		itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
		received_msg = NULL;
	}

	return NULL;
}

static
void process_dc_udp_data_ind(udp_data_ind_t	*udp_data_indP){
	/*Receives the PDU from UDP and forward it to PDCP*/

	mem_block_t	*SB_pdu = NULL;
	SB_pdu = get_free_mem_block(udp_data_indP->buffer_length, __func__);
	assert(udp_data_indP->buffer_length > 0 && "Impossible to get memory for SB at UE_DC");
	memcpy(SB_pdu->data, udp_data_indP->buffer, udp_data_indP->buffer_length);

	if (RC.dc_ue_dataP->reordering == FALSE){
		get_pdcp_data_ind_func()(&RC.dc_ue_dataP->ctxt, SRB_FLAG_NO, MBMS_FLAG_NO, 1, udp_data_indP->buffer_length, SB_pdu,NULL,NULL);

	} else {
		pthread_mutex_lock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
		get_pdcp_data_ind_func()(&RC.dc_ue_dataP->ctxt, SRB_FLAG_NO, MBMS_FLAG_NO, 1, udp_data_indP->buffer_length, SB_pdu,NULL,NULL);
		pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
	}
}

static
void process_dc_ue_data_req(dc_ue_data_req_t *dc_ue_data_reqP){

	MessageDef	*msg_p = NULL;
	udp_data_req_t	*udp_data_req_p = NULL;
	msg_p = itti_alloc_new_message(TASK_UE_DC, UDP_DATA_REQ);
	udp_data_req_p = &msg_p->ittiMsg.udp_data_req;
	udp_data_req_p->peer_address = inet_addr(RC.dc_ue_dataP->remote_ue_address);
	udp_data_req_p->peer_port = 2154;
	//udp_data_req_p->buffer = buffer;
	udp_data_req_p->buffer = dc_ue_data_reqP->pdu_buffer_dcP;
	udp_data_req_p->buffer_length = (uint32_t)dc_ue_data_reqP->pdu_size_dc;
	udp_data_req_p->buffer_offset = 0;
	if(itti_send_msg_to_task(TASK_UDP_UE, INSTANCE_DEFAULT, msg_p) != 0){
		printf("UE_DC, Message didn't send to mUE\n");
	}
}


int is_dc_ue_enabled(void){

	paramdef_t	UE_DCParams[] = UE_DCPARAMS_DESC;

	RC.dc_ue_dataP =(dc_ue_data_t *)malloc(sizeof(dc_ue_data_t));

	config_get(UE_DCParams, sizeof(UE_DCParams)/sizeof(paramdef_t), UE_CONFIG_STRING_DC_CONFIG);

	//cqi hack
	if(strcasecmp(*(UE_DCParams[DC_CQI_HACK_IDX].strptr), "yes") == 0){
		RC.dc_ue_dataP->cqi_vector.cqi_hack_enabled = TRUE;
		RC.dc_ue_dataP->cqi_vector.index = 0;
		RC.dc_ue_dataP->cqi_vector.previous_time = 0;
	} else
		RC.dc_ue_dataP->cqi_vector.cqi_hack_enabled = FALSE;


	//Dual Connectivity
	if (strcasecmp(*(UE_DCParams[DC_UE_ENABLED_IDX].strptr), "yes") == 0){
		RC.dc_ue_dataP->enabled	= TRUE;
		if(strcasecmp(*(UE_DCParams[DC_UE_TYPE_IDX ].strptr), "mue") == 0){
			RC.dc_ue_dataP->ue_type	= 1; //mUE
		}else
			RC.dc_ue_dataP->ue_type	= 2; //sUE;

		strcpy(RC.dc_ue_dataP->local_ue_address,*(UE_DCParams[DC_LOCAL_UE_ADDRESS_IDX].strptr));
		strcpy(RC.dc_ue_dataP->remote_ue_address,*(UE_DCParams[DC_REMOTE_UE_ADDRESS_IDX].strptr));

		//Reordering
		if(strcasecmp(*(UE_DCParams[DC_REORDERING_ENABLED_IDX ].strptr), "yes") == 0){
			RC.dc_ue_dataP->reordering	= TRUE; //PDCP Reordering enabled
			if(itti_create_task (TASK_PDCP_REORDERING, pdcp_reordering_task, NULL) < 0){
				printf("UE_DC, Create task for REORDERING has failed\n");
			}
			RC.dc_ue_dataP->reordering_algorithm = *(UE_DCParams[DC_REORDERING_ALGORITHM_IDX].uptr);
			if (RC.dc_ue_dataP->reordering_algorithm == 1){
				RC.dc_ue_dataP->algorithm_value = *(UE_DCParams[DC_ALGORITHM_VALUE_IDX].i64ptr)*1000; //saved in micro_seconds
			} else if (RC.dc_ue_dataP->reordering_algorithm == 2){
				RC.dc_ue_dataP->algorithm_value = *(UE_DCParams[DC_ALGORITHM_VALUE_IDX].i64ptr); // Number of of PDUs for Buffer Size
			} else
				RC.dc_ue_dataP->algorithm_value = *(UE_DCParams[DC_ALGORITHM_VALUE_IDX].i64ptr)*1000; // Microseconds for Method 3 or more
		} else
			RC.dc_ue_dataP->reordering	= FALSE; //PDCP Reordering disabled

		return 1;
	} else {
		RC.dc_ue_dataP->enabled	= FALSE;
		RC.dc_ue_dataP->ue_type	= mUE;
		return 0;
	}
}


static
void init_ue_for_dc(){
	/*Create UDP sockets for mUE and sUE*/

	MessageDef		*msg_p = NULL;

	printf("UE_DC, Let's create UDP Socket for %s\n", RC.dc_ue_dataP->local_ue_address);

	msg_p = itti_alloc_new_message(TASK_UE_DC, UDP_INIT);
	if (msg_p != NULL) {
		UDP_INIT(msg_p).port 	= 2154;
		UDP_INIT(msg_p).address = RC.dc_ue_dataP->local_ue_address;

		if (itti_send_msg_to_task(TASK_UDP_UE, INSTANCE_DEFAULT, msg_p) != 0) {
			printf("UE_DC, Error allocating the message UDP_INIT\n");
		}
	}
}

void rrc_dc_get_ctxt(const protocol_ctxt_t *const ctxtP){
	RC.dc_ue_dataP->ctxt.module_id 		= ctxtP->module_id;
	RC.dc_ue_dataP->ctxt.rnti 			= ctxtP->rnti;
	RC.dc_ue_dataP->ctxt.configured 	= ctxtP->configured;
	RC.dc_ue_dataP->ctxt.enb_flag 		= 0;
	RC.dc_ue_dataP->ctxt.instance 		= ctxtP->instance;
	RC.dc_ue_dataP->ctxt.eNB_index 		= ctxtP->eNB_index;

}
