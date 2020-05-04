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



#ifndef X2U_MESSAGES_TYPES_H_
#define X2U_MESSAGES_TYPES_H_

#include "x2u_procedures.h"

#define X2U_DC_INIT(mSGpTR)                   (mSGpTR)->ittiMsg.x2u_dc_init
#define X2U_DC_DATA_REQ(mSGpTR)               (mSGpTR)->ittiMsg.x2u_dc_data_req



typedef struct x2u_dc_data_req_s {
	rb_id_t 		rb_id_dc;
	uint16_t 		pdu_size_dc;
	unsigned char 	*pdu_buffer_dcP;
}x2u_dc_data_req_t;

#endif /* OPENAIR2_COMMON_X2U_MESSAGES_TYPES_H_ */
