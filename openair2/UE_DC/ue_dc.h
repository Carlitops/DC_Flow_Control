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
 * \\brief ue_dc task for UE in Dual Connectivity
 * \author Carlos Pupiales
 * \date 2019
 * \email carlos.pupiales@upc.edu
 * \version 1.0
 */
#include <pthread.h>
#include "common/config/config_paramdesc.h"
#include "platform_types.h"

#ifndef UE_DC_H_
#define UE_DC_H_

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* Dual_Connectivity configuration parameters section name for UE */
#define UE_CONFIG_STRING_DC_CONFIG    	 			  "DUAL_CONNECTIVITY_UE"

/* Dual_Connectivity configuration parameters names   */

#define UE_CONFIG_STRING_DC_UE_ENABLED            	  "DC_UE_ENABLED"
#define UE_CONFIG_STRING_DC_UE_TYPE                  "DC_UE_TYPE"
#define UE_CONFIG_STRING_DC_LOCAL_UE_ADDRESS         "DC_LOCAL_UE_ADDRESS"
#define UE_CONFIG_STRING_DC_REMOTE_UE_ADDRESS        "DC_REMOTE_UE_ADDRESS"
#define UE_CONFIG_STRING_DC_REORDERING_ENABLED		 "DC_REORDERING_ENABLED"

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
}

#define DC_UE_ENABLED_IDX                0
#define DC_UE_TYPE_IDX      			 1
#define DC_LOCAL_UE_ADDRESS_IDX          2
#define DC_REMOTE_UE_ADDRESS_IDX         3
#define DC_REORDERING_ENABLED_IDX		 4

/*---------------------------------------------------------------------------------------------------------------------------------------*/
typedef struct dc_ue_data_s {
	boolean_t		enabled;
	boolean_t		ue_type; //TRUE (1)for mUE, False (0) for sUE
	boolean_t		reordering; //TRUE (1) if pdcp reordering is enabled, False (0) otherwise
	char			remote_ue_address[16];
	char			local_ue_address[16];
} dc_ue_data_t;



void *ue_dc_task(void *arg);
int is_ue_dc_enabled(void);

void *udp_ue_dc_task(void *arg);
void udp_ue_recv_data(int sd);

#endif /* OPENAIR2_UE_DC_UE_DC_H_ */
