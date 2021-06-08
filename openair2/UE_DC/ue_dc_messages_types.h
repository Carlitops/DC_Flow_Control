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

/*! \file ue_dc_messages_types.h
 * \brief definition of messages for ue_dc
 * \author Carlos Pupiales
 * \date 2019
 * \email carlos.pupiales@upc.edu
 * \version 1.0
 */

#ifndef UE_DC_MESSAGES_TYPES_H_
#define UE_DC_MESSAGES_TYPES_H_

// Definitions to access message fields.

#define UE_DC_DATA_REQ(mSGpTR)                   (mSGpTR)->ittiMsg.ue_dc_data_req
#define UE_DC_DATA_IND(mSGpTR)                   (mSGpTR)->ittiMsg.ue_dc_data_ind
#define CTXT_UE_DC(mSGpTR)						 (mSGpTR)->ittiMsg.ctxt_ue_dc



// Define structures

typedef struct ue_dc_data_req_s {
	rb_id_t 		rb_id_dc;
	sdu_size_t 		sdu_size_dc;
	unsigned char 	*sdu_buffer_dc_p;
} ue_dc_data_req_t;

typedef struct ue_dc_data_ind_s {
	rb_id_t 		rb_id_dc;
	sdu_size_t 		sdu_size_dc;
	unsigned char 	*sdu_buffer_dc_p;
} ue_dc_data_ind_t;

typedef struct ctxt_ue_dc_s {
	module_id_t module_id;
	eNB_flag_t  enb_flag;
	instance_t  instance;
	rnti_t      rnti;
	frame_t     frame;         	//based on protocol_ctxt_t
	sub_frame_t subframe;
	eNB_index_t eNB_index;
	boolean_t   configured;
	boolean_t	brOption;
}ctxt_ue_dc_t;

#endif /* UE_DC_MESSAGES_TYPES_H_ */
