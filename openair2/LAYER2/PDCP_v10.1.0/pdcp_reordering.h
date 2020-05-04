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

#ifndef PDCP_REORDERING_H_
#define PDCP_REORDERING_H_

#include "COMMON/platform_types.h"
#include "pdcp.h"
#include "UTIL/LISTS/list.h"

#define RLC_MN	1
#define RLC_SN	2

/* Utils To generate a random probability */
#define UPPER_MASK		0x80000000
#define LOWER_MASK		0x7fffffff
#define TEMPERING_MASK_B	0x9d2c5680
#define TEMPERING_MASK_C	0xefc60000
#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */


typedef struct dc_buffer_t{
	struct dc_buffer_t *next;
	struct dc_buffer_t *head;
	struct dc_buffer_t *tail;
	rb_id_t			drb;
	sdu_size_t 		sdu_size;
	pdcp_sn_t 		sdu_sn;
	pdcp_hfn_t 		sdu_hfn;
	int64_t 		time_at_buffer; // Time when the PDU was stored in the buffer.
	int				rlc_source;
	int				buffer_size;
	unsigned char 	*sdu_data;
}dc_buffer_t;

typedef struct reordering_t{
	long 			t_reord_id; //timer ID
	uint64_t		pdcp_hash_key;
	int64_t			start_time;
	pthread_mutex_t temp_buffer;
	uint64_t		Packet_counter;
	float			arrival_rate;
}reordering_t;

typedef struct cqi_vector_s{
	boolean_t	cqi_hack_enabled;
	int64_t		previous_time;
	int			index;
}cqi_vector_t;


dc_buffer_t	reordering_buffer;

//dc_buffer_t	temporal_buffer;
list_t	pdcp_temporal_buffer;

void *pdcp_reordering_task(void *arg);

int64_t mib_get_time_us(void); //function to get the current time in us

void store_sdu_in_reordering_buffer(pdcp_sn_t SN, pdcp_hfn_t HFN, unsigned char *sdu_data, sdu_size_t sdu_size, sdu_size_t offset, rb_id_t drb);


void update_variables(pdcp_t *pdcpP, dc_buffer_t data, uint16_t max_sn);

void add_in_pdcp_temp_buffer(unsigned char *SB_PDU, sdu_size_t pdu_size, int rlc_source);

void remove_head_temp_buffer(list_t *buffer);

void start_t_reordering(pdcp_sn_t sn, pdcp_hfn_t hfn, int64_t timeout);

float get_arrival_rate(int64_t t);

uint8_t hacked_cqi(void);

dc_buffer_t	deliver_conse_sdusNEW(pdcp_sn_t sn, uint16_t max_sn);

#endif /* OPENAIR2_LAYER2_PDCP_V10_1_0_PDCP_REORDERING_H_ */
