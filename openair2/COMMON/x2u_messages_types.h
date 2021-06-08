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

/*! \file x2u_messages_types.h
 * \brief definition of messages for X2U
 * \author Carlos Pupiales
 * \date 2019
 * \email carlos.pupiales@upc.edu
 * \version 1.0
 */
#include "s1ap_messages_types.h"
#include "x2u_enb.h"

#ifndef X2U_MESSAGES_TYPES_H_
#define X2U_MESSAGES_TYPES_H_

// Definitions to access message fields.

#define DC_ENB_INIT(mSGpTR)                   (mSGpTR)->ittiMsg.dc_enb_init
#define DC_ENB_DATA_REQ(mSGpTR)				  (mSGpTR)->ittiMsg.dc_enb_data_req



// Define structures

/*typedef struct dc_enb_init_s {
	boolean_t		enabled;
	boolean_t		enb_type; //True (1)for MeNB, False (0) for SeNB
	uint32_t		port_for_x2u;
	char			remote_enb_address[16];
	char			local_address_for_x2u[16];
} dc_enb_init_t;*/

typedef struct dc_enb_data_req_s {
	rb_id_t 		rb_id_dc;
	uint16_t 		pdu_size_dc;
	unsigned char 	*pdu_buffer_dcP;
}dc_enb_data_req_t;


#endif /* X2U_MESSAGES_TYPES_H_ */
