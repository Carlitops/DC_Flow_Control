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

#include "platform_types.h"
#include "UTIL/MEM/mem_block.h"

#ifndef X2U_PROCEDURES_H_
#define X2U_PROCEDURES_H_

#define MeNB	1
#define	SeNB	2


// Define structures for DC

typedef struct dc_eNB_data_s{
	protocol_ctxt_t	ctxt;  // Protocol context
	rb_id_t			drb_id; // Data Radio Bearer assigned to UE
	boolean_t		enable;
	uint32_t		x2u_port;
	int				eNB_type; //MeNB or SeNB
	int 			flow_control_type;
	char			remote_eNB_address[16];
	char			local_eNB_address[16]; //Should be similar to X2C
} dc_eNB_data_t;


void *x2u_eNB_task(void *arg);

int is_DC_enabled(void);

void x2u_init_for_dc(char local_ip_address[16]);

#endif /* OPENAIR2_X2U_X2U_PROCEDURES_H_ */
