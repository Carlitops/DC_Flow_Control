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

#include "intertask_interface.h"
#include "pdcp_reordering.h"
#include "COMMON/platform_types.h"
#include <sys/time.h>
#include <time.h>
#include "pdcp.h"
#include "common/ran_context.h"
#include "pdcp_sequence_manager.h"
#include "assertions.h"
#include "common/utils/threadPool/thread-pool.h"

extern RAN_CONTEXT_t RC;

static
void pdcp_reordering_init(void);

static
void process_reordering_data_req(void);

static
void process_reordering_timer_expiration(pdcp_t	*pdcp_data);

static
dc_buffer_t *get_head_temp_buffer(dc_buffer_t *buffer);


void remove_head_temp_buffer(list_t *buffer);

static
void remove_head_reord_buffer(dc_buffer_t *buffer);

static
dc_buffer_t deliver_sdus_3gppNEW(pdcp_sn_t rx_reord_sn,pdcp_hfn_t rx_reord_hfn, uint16_t max_sn);

////////////

static
void pdcp_reordering_init(void){
	dc_buffer_t	*buffer = NULL;

	buffer = &reordering_buffer;
	buffer->head = NULL;
	buffer->next = NULL;
	buffer->tail = NULL;

	list_init(&pdcp_temporal_buffer, NULL);

	RC.dc_ue_dataP->reordering_utils.pdcp_hash_key = HASHTABLE_NOT_A_KEY_VALUE;
	RC.dc_ue_dataP->reordering_utils.Packet_counter = 0;
	RC.dc_ue_dataP->reordering_utils.start_time = 0;

	//Initialize Mutex
	if (pthread_mutex_init(&RC.dc_ue_dataP->reordering_utils.temp_buffer, NULL) != 0){; //initialize mutex to avoid read/write errors for temporal buffer
		printf("Mutex initialization for temp buffer tail has failed\n");
	}

}

void store_sdu_in_reordering_buffer(pdcp_sn_t SN, pdcp_hfn_t HFN, unsigned char *sdu_data, sdu_size_t sdu_size, sdu_size_t offset, rb_id_t drb){
	dc_buffer_t *new_element = NULL; //save the current sdu
	dc_buffer_t *head = NULL;
	dc_buffer_t *tail = NULL;
	dc_buffer_t *buffer = NULL;

	buffer = &reordering_buffer;

	new_element = (dc_buffer_t *)malloc(sdu_size - offset + sizeof(dc_buffer_t)); // add memory for the new element
	assert(sdu_size > 0 && "Size of PDU in Reordering Buffer is not correct\n");
	new_element->sdu_data = ((unsigned char *)new_element) + sizeof(dc_buffer_t);
	memcpy(new_element->sdu_data, &sdu_data[offset], sdu_size-offset);
	new_element->sdu_hfn = HFN;
	new_element->sdu_sn = SN;
	new_element->sdu_size = sdu_size-offset;
	new_element->drb = drb;
	new_element->time_at_buffer = mib_get_time_us(); // register the time when PDU is stored in in us
	new_element->next = NULL;

	head = buffer->head;
	tail = buffer->tail;

	if (head == NULL){
		buffer->head = new_element;
		buffer->tail = new_element;

	} else if ((SN < head->sdu_sn && HFN == head->sdu_hfn) || (HFN < head->sdu_hfn) ){
		// The SDU is the new HEAD
		new_element->next = head;
		buffer->head = new_element;
	} else if ((SN > tail->sdu_sn && HFN >= tail->sdu_hfn) || HFN > tail->sdu_hfn){
			// The SDU is the new TAIL
			tail->next = new_element;
			buffer->tail = new_element;
	} else {
		// The new SDU is between HEAD and TAIL
		dc_buffer_t *aux = head;
		while ( aux != NULL && ( (SN > aux->next->sdu_sn && HFN >= aux->next->sdu_hfn) || (HFN > aux->next->sdu_hfn ) ) ){
			aux = aux->next;
		}
		new_element->next = aux->next;
		aux->next = new_element;
	}
	buffer->buffer_size++;
}

void add_in_pdcp_temp_buffer(unsigned char *SB_PDU, sdu_size_t pdu_size, int rlc_source){
	mem_block_t	*new_pdu = NULL;
	new_pdu = get_free_mem_block(pdu_size, __func__);
	assert(pdu_size > 0 && "Size of PDU is not valid\n");

	memcpy(new_pdu->data, SB_PDU, pdu_size);

	pthread_mutex_lock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
	list_add_tail_eurecom (new_pdu, &pdcp_temporal_buffer);
	pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);

	//get_arrival_rate();
}

void remove_head_temp_buffer(list_t *buffer){
	mem_block_t *temp = NULL;

	pthread_mutex_lock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);

	temp = buffer->head;
	buffer->head = temp->next;
	buffer->nb_elements = buffer->nb_elements - 1;
	if (buffer->head == NULL){
		buffer->tail = NULL;
	} else
		temp->next = NULL;

	pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
}

static
void remove_head_reord_buffer(dc_buffer_t *buffer){
	dc_buffer_t *temp = NULL;

	temp = buffer->head;
	if (temp != NULL){
		buffer->head = temp->next;

		if(buffer->head == NULL){
			buffer->tail = NULL;
		} else
			temp->next = NULL;

		buffer->buffer_size--;
	}
}

static
dc_buffer_t *get_head_temp_buffer(dc_buffer_t *buffer){
	return buffer->head;
}

static
void process_reordering_data_req(void){
	mem_block_t		*new_pdcp_pdu = NULL;

	while ( (new_pdcp_pdu = list_get_head (&pdcp_temporal_buffer)) != NULL ){

		get_pdcp_data_ind_func()(&RC.dc_ue_dataP->ctxt, SRB_FLAG_NO, MBMS_FLAG_NO,1, new_pdcp_pdu->size, new_pdcp_pdu,NULL,NULL);
	}
}

void start_t_reordering(pdcp_sn_t sn, pdcp_hfn_t hfn, int64_t timeout){

	timer_setup(0,timeout,TASK_PDCP_REORDERING,INSTANCE_DEFAULT,TIMER_ONE_SHOT,NULL,&RC.dc_ue_dataP->reordering_utils.t_reord_id);
	//printf("Timer starts at %.0f for rx_reord: %d\n", (mib_get_time_us() - RC.dc_ue_dataP->reordering_utils.start_time)*0.001, sn);

}

static
void process_reordering_timer_expiration(pdcp_t	*pdcp_data){
	uint16_t		max_sn = pdcp_calculate_max_seq_num_for_given_size(pdcp_data->seq_num_size);
	dc_buffer_t	temp_data;

	temp_data = deliver_sdus_3gppNEW(pdcp_data->rx_reord_sn, pdcp_data->rx_reord_hfn, max_sn);
	update_variables(pdcp_data, temp_data, max_sn);

	if ((pdcp_data->rx_deliv_sn < pdcp_data->rx_next_sn && pdcp_data->rx_deliv_hfn <= pdcp_data->rx_next_hfn)
			|| pdcp_data->rx_deliv_hfn < pdcp_data->rx_next_hfn){

		pdcp_data->rx_reord_sn = pdcp_data->rx_next_sn;
		pdcp_data->rx_reord_hfn = pdcp_data->rx_next_hfn;

		start_t_reordering(pdcp_data->rx_reord_sn,pdcp_data->rx_reord_hfn,RC.dc_ue_dataP->algorithm_value);
		pdcp_data->t_reordering_running = TRUE;
	}
}

float get_arrival_rate(int64_t t){

	if (RC.dc_ue_dataP->reordering_utils.start_time == 0){
		RC.dc_ue_dataP->reordering_utils.start_time = t;
	}

	int64_t cumulative_time = (t - RC.dc_ue_dataP->reordering_utils.start_time)*0.001;
	RC.dc_ue_dataP->reordering_utils.Packet_counter++;

	//RC.dc_ue_dataP->reordering_utils.arrival_rate = (float) cumulative_time/RC.dc_ue_dataP->reordering_utils.Packet_counter;
	float rate = (float) cumulative_time/RC.dc_ue_dataP->reordering_utils.Packet_counter;

	return rate;
}

int64_t mib_get_time_us(void){

  struct timespec tms;

  /* The C11 way */
  /* if (! timespec_get(&tms, TIME_UTC))  */

  /* POSIX.1-2008 way */
  if (clock_gettime(CLOCK_REALTIME,&tms)) {
    return -1;
  }
  /* seconds, multiplied with 1 million */
  int64_t micros = tms.tv_sec * 1000000;
  /* Add full microseconds */
  micros += tms.tv_nsec/1000;
  /* round up if necessary */
  if (tms.tv_nsec % 1000 >= 500) {
    ++micros;
  }
  return micros;
}

void update_variables(pdcp_t *pdcpP, dc_buffer_t data, uint16_t max_sn){

	pdcpP->last_submitted_pdcp_rx_sn = data.sdu_sn;
	pdcpP->last_submitted_pdcp_rx_hfn = data.sdu_hfn;
	pdcpP->rx_deliv_sn = pdcpP->last_submitted_pdcp_rx_sn + 1;
	pdcpP->rx_deliv_hfn = pdcpP->last_submitted_pdcp_rx_hfn;
	if (pdcpP->rx_deliv_sn > max_sn){
		pdcpP->rx_deliv_sn = 0;
		pdcpP->rx_deliv_hfn++;
	}
}

void *pdcp_reordering_task(void *arg){
	MessageDef 		*received_msg = NULL;
	itti_mark_task_ready(TASK_PDCP_REORDERING);
	pdcp_reordering_init();

	while (1){
		itti_receive_msg(TASK_PDCP_REORDERING, &received_msg);
		if(received_msg != NULL){
			switch (ITTI_MSG_ID(received_msg)) {

			case REORDERING_DATA_REQ:

				process_reordering_data_req();

				break;

			case TIMER_HAS_EXPIRED:
			{
				pdcp_t      	*pdcp_data = NULL;
				hashtable_rc_t  h_rc;
				dc_buffer_t		*reordering_bufferP = NULL;
				int64_t t = mib_get_time_us();
				reordering_bufferP = &reordering_buffer;

				pthread_mutex_lock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);

				h_rc = hashtable_get(pdcp_coll_p, RC.dc_ue_dataP->reordering_utils.pdcp_hash_key, (void **)&pdcp_data);
				if (h_rc != HASH_TABLE_OK) {
					printf("Error getting PDCP DATA from hash table\n");
					pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
					break;
				}
				pdcp_data->t_reordering_running = FALSE;
				/*printf("Timer has expired at %.0f for rx_reord %d with buffer size: %d discard SDU: %d\n", (t-RC.dc_ue_dataP->reordering_utils.start_time)*0.001,
						pdcp_data->rx_reord_sn,reordering_bufferP->buffer_size, pdcp_data->rx_deliv_sn);*/

				if (reordering_bufferP->head == NULL){
					pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
					printf("Timer has expired but Reordering Buffer is empty\n");
					break;
				} else {
					process_reordering_timer_expiration(pdcp_data);
					pthread_mutex_unlock(&RC.dc_ue_dataP->reordering_utils.temp_buffer);
				}
			}
				break;

			}//end switch
			itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
			received_msg = NULL;

		}//end if
	}//end while

	return NULL;
}

uint8_t hacked_cqi(void){
	/* CQI matrix:
	 * row 0 empty to match format for sUE/mUE
	 * row 1: Set 1 for MN
	 * row 2: Set 2 for SN
	 * */

	uint8_t cqi_matrix [3][60] = { {},{12, 12, 12, 15, 15, 9, 15, 15, 12, 12, 12, 12, 15, 15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 12,
			12, 12,	14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 13, 13, 13, 13, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 15},
			{14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 10, 10, 12, 12, 13, 15, 15, 12, 11, 11, 13, 13, 15, 12,
			12, 12, 12, 11, 11, 11, 10, 15, 15, 15, 15, 15, 15, 15, 15, 15, 12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15} };

	uint8_t ret_cqi = 0;
	int64_t t_ms = mib_get_time_us()*0.001;

	if (RC.dc_ue_dataP->cqi_vector.previous_time == 0){
		RC.dc_ue_dataP->cqi_vector.previous_time = t_ms;
		ret_cqi = cqi_matrix[RC.dc_ue_dataP->ue_type][0];
		//printf("The cqi reported to eNB is %d\n",ret_cqi);

	} else if (t_ms - RC.dc_ue_dataP->cqi_vector.previous_time >= 100){
			RC.dc_ue_dataP->cqi_vector.previous_time = t_ms;
			RC.dc_ue_dataP->cqi_vector.index++;
			if (RC.dc_ue_dataP->cqi_vector.index >= 60)
				RC.dc_ue_dataP->cqi_vector.index = 0;

			ret_cqi = cqi_matrix[RC.dc_ue_dataP->ue_type][RC.dc_ue_dataP->cqi_vector.index];
			//printf("The cqi reported to eNB is %d\n",ret_cqi);

	} else
		ret_cqi = cqi_matrix[RC.dc_ue_dataP->ue_type][RC.dc_ue_dataP->cqi_vector.index];

	return ret_cqi;
}


static
dc_buffer_t deliver_sdus_3gppNEW(pdcp_sn_t rx_reord_sn, pdcp_hfn_t rx_reord_hfn, uint16_t max_sn){
	notifiedFIFO_elt_t		*new_sdu 		= NULL;
	dc_buffer_t		*first_element	= NULL;
	dc_buffer_t		ret;
	pdcp_sn_t		sn_ref;

	sn_ref	= rx_reord_sn;

	while ((first_element = get_head_temp_buffer(&reordering_buffer)) != NULL){

		if( ((first_element->sdu_sn < rx_reord_sn && first_element->sdu_hfn <= rx_reord_hfn ) || first_element->sdu_hfn < rx_reord_hfn)
				|| first_element->sdu_sn == sn_ref ){

			new_sdu = newNotifiedFIFO_elt(first_element->sdu_size + sizeof (pdcp_data_ind_header_t), 0, NULL, NULL);
			pdcp_data_ind_header_t * pdcpHead = (pdcp_data_ind_header_t *)NotifiedFifoData(new_sdu);
			memset(pdcpHead, 0, sizeof (pdcp_data_ind_header_t));
			pdcpHead->data_size = first_element->sdu_size;
			pdcpHead->rb_id = first_element->drb;
			pdcpHead->inst  = RC.dc_ue_dataP->ctxt.module_id; //check;
			AssertFatal((first_element->sdu_size >= 0), "invalid PDCP SDU size!");
			memcpy(pdcpHead+1,first_element->sdu_data,first_element->sdu_size);
			pushNotifiedFIFO(&pdcp_sdu_list, new_sdu);

			if (first_element->sdu_sn == sn_ref){
				sn_ref++;
				if (sn_ref > max_sn)
					sn_ref = 0;
			}

			ret.sdu_sn = first_element->sdu_sn;
			ret.sdu_hfn = first_element->sdu_hfn;
			int64_t timeinBuffer = mib_get_time_us()*0.001 - first_element->time_at_buffer*0.001;
			//printf("SDU %d was in buffer %ld ms\n",first_element->sdu_sn, timeinBuffer);

			remove_head_reord_buffer(&reordering_buffer);
			free(first_element);
		} else
			return ret;
	}
	return ret;
}

dc_buffer_t	deliver_conse_sdusNEW(pdcp_sn_t sn, uint16_t max_sn){
	notifiedFIFO_elt_t		*new_sdu 		= NULL;
	dc_buffer_t		*first_element	= NULL;
	dc_buffer_t		ret;

	while ( ((first_element = get_head_temp_buffer(&reordering_buffer)) != NULL) && first_element->sdu_sn == sn ){

		new_sdu = newNotifiedFIFO_elt(first_element->sdu_size + sizeof (pdcp_data_ind_header_t), 0, NULL, NULL);
		pdcp_data_ind_header_t * pdcpHead = (pdcp_data_ind_header_t *)NotifiedFifoData(new_sdu);
		memset(pdcpHead, 0, sizeof (pdcp_data_ind_header_t));
		pdcpHead->data_size = first_element->sdu_size;
		pdcpHead->rb_id = first_element->drb;
		pdcpHead->inst  = RC.dc_ue_dataP->ctxt.module_id; //check;
		AssertFatal((first_element->sdu_size >= 0), "invalid PDCP SDU size!");
		memcpy(pdcpHead+1,first_element->sdu_data,first_element->sdu_size);
		pushNotifiedFIFO(&pdcp_sdu_list, new_sdu);

		sn++;
		if (sn > max_sn)
			sn = 0;

		ret.sdu_sn = first_element->sdu_sn;
		ret.sdu_hfn = first_element->sdu_hfn;
		int64_t timeinBuffer = (mib_get_time_us() - first_element->time_at_buffer)*0.001;
		//printf("SDU %d was in buffer %ld ms\n",first_element->sdu_sn, timeinBuffer);

		remove_head_reord_buffer(&reordering_buffer);
		free(first_element);
	}
	return ret;
}
