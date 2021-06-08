/*
 * pdcp_dc_flow_control.h
 *
 *  Created on: Jan 11, 2021
 *      Author: pupi
 */

#ifndef PDCP_FLOW_CONTROL_H_
#define PDCP_FLOW_CONTROL_H_

#include "COMMON/platform_types.h"
#include "UTIL/LISTS/list.h"

typedef struct cqi_vector_s{
	boolean_t	cqi_hack_enabled;
	int64_t		previous_time;
	int			index;
	int			cqi_granularity;
	int			cqi_reported;
}cqi_vector_t;

typedef struct ccw_parameters_s{
	int		Tccw; 	//splitting periodicity
	int 	Dq_max;	//Max buffering time
	int		CCr;	//Capacity and Congestion Report
	float 	alpha;	//EWMA smoothing factor
}ccw_parameters_t;

typedef struct flow_control_data_s {
	float		pdu_arrival_rate;
	float		avg_pdu_size;
	int			tbs_source; 	// 1=MeNB, 2=SeNB
	uint32_t 	current_avg_tbs; //current average TBS value
	uint32_t 	avg_tbs; 		// average value transmitted from MAC
	uint32_t	nb_samples;		// number of samples for tbs or pdu
	uint32_t	nb_pdus_tx;	// number of pdus to send either to MN or SN
	uint16_t	old_nb_pdus_tx;
	uint16_t	avg_system_time;
	uint32_t 	sum_tbs_values;
	uint32_t 	sum_pdu_size;
	int64_t	 	start_time;
	cqi_vector_t	cqi_vector;
}flow_control_data_t;

list_t                 flow_control_buffer;

typedef struct fc_buffer_header_s{
	protocol_ctxt_t  *ctxt;
	sdu_size_t  	pdu_size;
	rb_id_t 		drb;
	int64_t	 		time_in_FCbuffer;

}fc_buffer_header_t;


void update_flow_control_variables(flow_control_data_t *tbs_data_p);

int64_t mib_get_time_us(void);

void store_pdu_FCbuffer(unsigned char *pdu_data, sdu_size_t pdu_size, rb_id_t drb, protocol_ctxt_t  *ctxt);

void flush_FCbuffer(void);

void flush_FCbuffer_Laselva(void);

uint8_t hacked_cqi_eNB(void);


#endif /* OPENAIR2_LAYER2_PDCP_V10_1_0_PDCP_FLOW_CONTROL_H_ */
