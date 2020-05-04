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


#ifndef UE_DC_H_
#define UE_DC_H_

#include <pthread.h>
#include "common/config/config_paramdesc.h"
#include "platform_types.h"
#include "pdcp_reordering.h"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* Dual_Connectivity configuration parameters for UE */
#define UE_CONFIG_STRING_DC_CONFIG    	 			  "DUAL_CONNECTIVITY_UE"

/* Dual_Connectivity configuration parameters names   */

#define UE_CONFIG_STRING_DC_UE_ENABLED       		 "DC_UE_ENABLED"
#define UE_CONFIG_STRING_DC_UE_TYPE                  "DC_UE_TYPE"
#define UE_CONFIG_STRING_DC_LOCAL_UE_ADDRESS         "DC_LOCAL_UE_ADDRESS"
#define UE_CONFIG_STRING_DC_REMOTE_UE_ADDRESS        "DC_REMOTE_UE_ADDRESS"
#define UE_CONFIG_STRING_DC_REORDERING_ENABLED		 "DC_REORDERING_ENABLED"
#define UE_CONFIG_STRING_DC_REORDERING_ALGORITHM	 "DC_REORDERING_ALGORITHM"
#define UE_CONFIG_STRING_DC_ALGORITHM_VALUE			 "DC_ALGORITHM_VALUE"
#define UE_CONFIG_STRING_DC_PROB_HARQ_DISCARD		 "DC_PROB_HARQ_DISCARD"
#define UE_CONFIG_STRING_DC_CQI_HACK				 "DC_CQI_HACK"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            Dual Connectivity configuration parameters for UE                                              */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define UE_DCPARAMS_DESC {  \
{UE_CONFIG_STRING_DC_UE_ENABLED,                NULL,      0,         strptr:NULL,   	defstrval:NULL,   	TYPE_STRING,   0},          \
{UE_CONFIG_STRING_DC_UE_TYPE,	                NULL,      0,         strptr:NULL,   	defstrval:NULL,		TYPE_STRING,   0},          \
{UE_CONFIG_STRING_DC_LOCAL_UE_ADDRESS,        	NULL,      0,         strptr:NULL,   	defstrval:NULL,   	TYPE_STRING,   0},          \
{UE_CONFIG_STRING_DC_REMOTE_UE_ADDRESS,        	NULL,      0,         strptr:NULL,   	defstrval:NULL,   	TYPE_STRING,   0},          \
{UE_CONFIG_STRING_DC_REORDERING_ENABLED,       	NULL,      0,         strptr:NULL,   	defstrval:NULL,   	TYPE_STRING,   0},          \
{UE_CONFIG_STRING_DC_REORDERING_ALGORITHM,     	NULL,      0,         uptr:NULL,   		defintval:1,	  	TYPE_UINT,     0},          \
{UE_CONFIG_STRING_DC_ALGORITHM_VALUE,       	NULL,      0,         i64ptr:NULL, 		defstrval:NULL,   	TYPE_INT64,   0},          \
{UE_CONFIG_STRING_DC_PROB_HARQ_DISCARD,	     	NULL,      0,         dblptr:NULL,   	defdblval:0,	  	TYPE_DOUBLE,  0},          \
{UE_CONFIG_STRING_DC_CQI_HACK,	                NULL,      0,         strptr:NULL,   	defstrval:"no",   	TYPE_STRING,   0},          \
}

#define DC_UE_ENABLED_IDX                0
#define DC_UE_TYPE_IDX      			 1
#define DC_LOCAL_UE_ADDRESS_IDX          2
#define DC_REMOTE_UE_ADDRESS_IDX         3
#define DC_REORDERING_ENABLED_IDX		 4
#define DC_REORDERING_ALGORITHM_IDX		 5
#define DC_ALGORITHM_VALUE_IDX			 6
#define DC_PROB_HARQ_DISCARD_IDX		 7
#define DC_CQI_HACK_IDX					 8


/*---------------------------------------------------------------------------------------------------------------------------------------*/

#define 	mUE		1
#define 	sUE		2

typedef struct dc_ue_data_s {
	protocol_ctxt_t	ctxt;
	rb_id_t			drb_id[3]; // Data Radio Bearers assigned to UE
	boolean_t		enabled;
	boolean_t		reordering; // True or False
	uint32_t		reordering_algorithm;
	int64_t			algorithm_value;
	int				ue_type; 	// mUE or sUE
	char			remote_ue_address[16];
	char			local_ue_address[16];
	double			prob_harq_discard;
	float			avg_bler;
	reordering_t	reordering_utils;
	cqi_vector_t	cqi_vector;
} dc_ue_data_t;

void rrc_dc_get_ctxt(const protocol_ctxt_t *const ctxtP);

int is_dc_ue_enabled(void);

void *ue_dc_task(void *arg);

void *udp_ue_task(void *arg);



#endif /* OPENAIR2_UE_DC_UE_DC_H_ */
