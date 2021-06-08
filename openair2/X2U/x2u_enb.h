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

/*! \file ue_dc.c_
 * \\brief headers for Dual Connectivity
 * \author Carlos Pupiales
 * \date 2019
 * \email carlos.pupiales@upc.edu
 * \version 1.0
 */

#ifndef X2U_ENB_H_
#define X2U_ENB_H_

#include "COMMON/platform_types.h"
#include "pdcp_flow_control.h"

#define MeNB	1
#define	SeNB	2

typedef struct dc_eNB_data_s{
	protocol_ctxt_t	ctxt;  // Protocol context
	rb_id_t			drb_id; // Data Radio Bearer assigned to UE
	boolean_t		enable;
	uint32_t		x2u_port;
	uint32_t		flow_control_type;
	int				eNB_type; //MeNB or SeNB
	char			remote_eNB_address[16];
	char			local_eNB_address[16]; //Should be similar to X2C

	//Flow control
	uint64_t 		pdcp_hash_key_eNB;
	flow_control_data_t	fc_data_mac;
	pthread_mutex_t FCbuffer;
	ccw_parameters_t	ccw_parameters;

} dc_eNB_data_t;

void *x2u_enb_task(void *arg);
int is_dc_enabled(void);

#endif /* X2U_ENB_H_ */
