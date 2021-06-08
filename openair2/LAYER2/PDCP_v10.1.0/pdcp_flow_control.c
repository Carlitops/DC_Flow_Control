/*
 * pdcp_dc_flow_control.c
 *
 *  Created on: Jan 11, 2021
 *      Author: pupi
 */

#include "pdcp_flow_control.h"
#include "pdcp.h"
#include "common/ran_context.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "COMMON/platform_types.h"
#include "rlc.h"
#include "mem_block.h"
#include "x2u_messages_types.h"

extern RAN_CONTEXT_t RC;

void update_flow_control_variables(flow_control_data_t *tbs_data_p){
	pdcp_t      	*pdcp_dataP = NULL;
	hashtable_rc_t  h_rc;
	list_t			*bufferP = &flow_control_buffer;

	h_rc = hashtable_get(pdcp_coll_p, RC.dc_enb_dataP->pdcp_hash_key_eNB, (void **)&pdcp_dataP);
	if (h_rc == HASH_TABLE_OK) {

		if (tbs_data_p->tbs_source == 1){
			pdcp_dataP->fc_MN.current_avg_tbs 	= tbs_data_p->avg_tbs;
			pdcp_dataP->fc_MN.avg_system_time 	= tbs_data_p->avg_system_time;
		} else {
			pdcp_dataP->fc_SN.current_avg_tbs 	= tbs_data_p->avg_tbs;
			pdcp_dataP->fc_SN.avg_system_time 	= tbs_data_p->avg_system_time;
		}

		if (RC.dc_enb_dataP->flow_control_type != 2){
			//printf("Avg_RLC_delay: %d ms source: %d\n", tbs_data_p->avg_system_time, tbs_data_p->tbs_source);
		}

		if (RC.dc_enb_dataP->flow_control_type == 3 && bufferP->head != NULL){

			if  (pdcp_dataP->fc_MN.avg_system_time <= 30 || pdcp_dataP->fc_SN.avg_system_time <= 40){
			    	flush_FCbuffer_Laselva();
			}
		}
		//
	}
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

void store_pdu_FCbuffer(unsigned char *pdu_data, sdu_size_t pdu_size, rb_id_t drb, protocol_ctxt_t  *ctxt){
	mem_block_t *stored_pdu = NULL;
	list_t	*bufferP = &flow_control_buffer;

	stored_pdu = get_free_mem_block(pdu_size + sizeof (fc_buffer_header_t), __func__);
	AssertError (stored_pdu != NULL, break, "Error getting memory to store the PDU for Flow Control\n");
	memset(stored_pdu->data, 0, sizeof (fc_buffer_header_t));
	((fc_buffer_header_t *) stored_pdu->data)->pdu_size 		= pdu_size;
	((fc_buffer_header_t *) stored_pdu->data)->drb 				= drb;
	((fc_buffer_header_t *) stored_pdu->data)->ctxt 			= ctxt;
	((fc_buffer_header_t *) stored_pdu->data)->time_in_FCbuffer = mib_get_time_us();
	memcpy(&stored_pdu->data[sizeof(fc_buffer_header_t)], pdu_data, pdu_size);
	if (bufferP->nb_elements == 1)
		pthread_mutex_lock(&RC.dc_enb_dataP->FCbuffer);

	list_add_tail_eurecom (stored_pdu, &flow_control_buffer);
	pthread_mutex_unlock(&RC.dc_enb_dataP->FCbuffer);
}

void flush_FCbuffer(void){
	mem_block_t 	*first_pdu = NULL;
	pdcp_t          *pdcp_dataP = NULL;
	list_t			*buffer = &flow_control_buffer;
	hashtable_rc_t  h_rc;
	uint32_t		bytes_to_mn = 0;
	uint32_t		bytes_to_sn = 0;
	uint32_t		bytes_sent_mn = 0;
	uint32_t		bytes_sent_sn = 0;
	int				max_delay_ms = RC.dc_enb_dataP->ccw_parameters.Dq_max;
	int64_t			sum_times = 0;
	int				sent_pdus_mn = 0;
	int				sent_pdus_sn = 0;
	int				nb_pdus = buffer->nb_elements;

	h_rc = hashtable_get(pdcp_coll_p, RC.dc_enb_dataP->pdcp_hash_key_eNB, (void **)&pdcp_dataP);
	AssertError (h_rc == HASH_TABLE_OK, break, "Error getting PDCP parameters at flush-FCbuffer\n");

	//Calculate the amount of bytes to send according the delay constraint
	if (max_delay_ms - pdcp_dataP->fc_SN.avg_system_time <= 0){
		bytes_to_sn = 0;
	} else if (max_delay_ms - pdcp_dataP->fc_SN.avg_system_time >= RC.dc_enb_dataP->ccw_parameters.Tccw){
		bytes_to_sn = pdcp_dataP->fc_SN.current_avg_tbs * RC.dc_enb_dataP->ccw_parameters.Tccw;
	} else {
		bytes_to_sn = pdcp_dataP->fc_SN.current_avg_tbs * (max_delay_ms - pdcp_dataP->fc_SN.avg_system_time);
	}

	if (max_delay_ms - pdcp_dataP->fc_MN.avg_system_time <= 0){
		bytes_to_mn = 0;
	} else if (max_delay_ms - pdcp_dataP->fc_MN.avg_system_time >= RC.dc_enb_dataP->ccw_parameters.Tccw){
		bytes_to_mn = pdcp_dataP->fc_MN.current_avg_tbs * RC.dc_enb_dataP->ccw_parameters.Tccw;
	} else {
		bytes_to_mn = pdcp_dataP->fc_MN.current_avg_tbs * (max_delay_ms - pdcp_dataP->fc_MN.avg_system_time);
	}

	while((first_pdu = list_get_head (&flow_control_buffer)) != NULL){
		sdu_size_t  pdu_size = ((fc_buffer_header_t *) first_pdu->data)->pdu_size;

		if (bytes_sent_sn < bytes_to_sn){
			MessageDef     *message       = NULL;
			unsigned char *sent_pduP;

			sent_pduP = (unsigned char *)malloc(pdu_size);
			memcpy(sent_pduP, &(first_pdu->data[sizeof(fc_buffer_header_t)]), pdu_size );
			message = itti_alloc_new_message(TASK_PDCP_ENB, DC_ENB_DATA_REQ);
			DC_ENB_DATA_REQ(message).pdu_size_dc = pdu_size;
			DC_ENB_DATA_REQ(message).pdu_buffer_dcP = sent_pduP;
			itti_send_msg_to_task(TASK_X2U, INSTANCE_DEFAULT, message);
			bytes_sent_sn += pdu_size;
			sent_pdus_sn++;

		} else if (bytes_sent_mn < bytes_to_mn){
			rb_id_t  drbID = ((fc_buffer_header_t *) first_pdu->data)->drb;
			protocol_ctxt_t  *ctxtP = ((fc_buffer_header_t *) first_pdu->data)->ctxt;
			mem_block_t       *pdu_p      = NULL;
			//printf("PDU sent via MN, remaining %d\n",buffer->nb_elements-1);
			pdu_p = get_free_mem_block(pdu_size, __func__);
			memcpy(pdu_p->data, &(first_pdu->data[sizeof(fc_buffer_header_t)]), pdu_size );
			rlc_data_req(ctxtP, 0, 0, drbID, 0, 0, pdu_size, pdu_p, NULL, NULL);
			bytes_sent_mn += pdu_size;
			sent_pdus_mn++;

		} else
			break;

		sum_times += (mib_get_time_us() - ((fc_buffer_header_t *) first_pdu->data)->time_in_FCbuffer)*0.001;

		if (buffer->nb_elements == 1)
			pthread_mutex_lock(&RC.dc_enb_dataP->FCbuffer);

		list_remove_head (&flow_control_buffer);
		pthread_mutex_unlock(&RC.dc_enb_dataP->FCbuffer);
		free(first_pdu);
	}
	float pdcp_delay = (float) sum_times/(sent_pdus_sn + sent_pdus_mn);
	/*printf("DC scheduler-> avg_delay_pdcp: %.0f FCbuffer_size: %d avg_delay_mn: %d avg_delay_sn: %d pdus_to_mn %d pdus_to_sn %d\n", pdcp_delay,
			nb_pdus, pdcp_dataP->fc_MN.avg_system_time, pdcp_dataP->fc_SN.avg_system_time, sent_pdus_mn, sent_pdus_sn);*/
}

void flush_FCbuffer_Laselva(void){
	mem_block_t 	*first_pdu = NULL;
	pdcp_t          *pdcp_dataP = NULL;
	list_t			*buffer = &flow_control_buffer;
	hashtable_rc_t  h_rc;
	boolean_t		to_mn = FALSE;
	boolean_t		to_sn = FALSE;
	int64_t			sum_times = 0;
	int				sent_pdus = 0;
	int				buffer_size = buffer->nb_elements;

	h_rc = hashtable_get(pdcp_coll_p, RC.dc_enb_dataP->pdcp_hash_key_eNB, (void **)&pdcp_dataP);
	AssertError (h_rc == HASH_TABLE_OK, break, "Error getting PDCP parameters at flush-FCbuffer\n");

	if (pdcp_dataP->fc_MN.avg_system_time < pdcp_dataP->fc_SN.avg_system_time){
		// MN < SN, then send through MN or quit
	    if (pdcp_dataP->fc_MN.avg_system_time <= 30)
	    	to_mn = TRUE;
	} else {
		// MN > SN, then send through SN or quit
	    if (pdcp_dataP->fc_SN.avg_system_time <= 40)
	    	to_sn = TRUE;
	}

	while((first_pdu = list_get_head (&flow_control_buffer)) != NULL){
		sdu_size_t  pdu_size = ((fc_buffer_header_t *) first_pdu->data)->pdu_size;
		if (to_sn){
			MessageDef     *message       = NULL;
			unsigned char *sent_pduP;

			sent_pduP = (unsigned char *)malloc(pdu_size);
			memcpy(sent_pduP, &(first_pdu->data[sizeof(fc_buffer_header_t)]), pdu_size );
			message = itti_alloc_new_message(TASK_PDCP_ENB, DC_ENB_DATA_REQ);
			DC_ENB_DATA_REQ(message).pdu_size_dc = pdu_size;
			DC_ENB_DATA_REQ(message).pdu_buffer_dcP = sent_pduP;
			itti_send_msg_to_task(TASK_X2U, INSTANCE_DEFAULT, message);

		} else if (to_mn){
			rb_id_t  drbID = ((fc_buffer_header_t *) first_pdu->data)->drb;
			protocol_ctxt_t  *ctxtP = ((fc_buffer_header_t *) first_pdu->data)->ctxt;
			mem_block_t       *pdu_p      = NULL;

			pdu_p = get_free_mem_block(pdu_size, __func__);
			memcpy(pdu_p->data, &(first_pdu->data[sizeof(fc_buffer_header_t)]), pdu_size );
			rlc_data_req(ctxtP, 0, 0, drbID, 0, 0, pdu_size, pdu_p, NULL, NULL);

		} else
			break;

		sum_times += (mib_get_time_us() - ((fc_buffer_header_t *) first_pdu->data)->time_in_FCbuffer)*0.001;
		sent_pdus++;

		if (buffer->nb_elements == 1)
			pthread_mutex_lock(&RC.dc_enb_dataP->FCbuffer);

		list_remove_head (&flow_control_buffer);
		pthread_mutex_unlock(&RC.dc_enb_dataP->FCbuffer);
		free(first_pdu);

	}

	if (sent_pdus > 0){
		float delay = (float) sum_times/(sent_pdus);
		/*printf("Laselva-> avg_delay_pdcp: %.0f FCbuffer_size: %d pdus_sent %d via %s\n", delay, buffer_size, sent_pdus, (to_mn==1?"MN":"SN"));*/
	}

}

uint8_t hacked_cqi_eNB(void){
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

	//printf("Elapsed time between cqi calls %ld\n", t_ms - RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time);
	if (RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time == 0){
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time = t_ms;
		ret_cqi = cqi_matrix[RC.dc_enb_dataP->eNB_type][0];
		//printf("The cqi reported to eNB is %d\n",ret_cqi);

	} else if (t_ms - RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time >= RC.dc_enb_dataP->fc_data_mac.cqi_vector.cqi_granularity){
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.previous_time = t_ms;
		RC.dc_enb_dataP->fc_data_mac.cqi_vector.index++;
			if (RC.dc_enb_dataP->fc_data_mac.cqi_vector.index >= 60)
				RC.dc_enb_dataP->fc_data_mac.cqi_vector.index = 0;

			ret_cqi = cqi_matrix[RC.dc_enb_dataP->eNB_type][RC.dc_enb_dataP->fc_data_mac.cqi_vector.index];
			//printf("The cqi reported to eNB is %d\n",ret_cqi);

	} else
		ret_cqi = cqi_matrix[RC.dc_enb_dataP->eNB_type][RC.dc_enb_dataP->fc_data_mac.cqi_vector.index];

	return ret_cqi;
}
