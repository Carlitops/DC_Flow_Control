/*
 * flow_control_messages_types.h
 *
 *  Created on: Jan 11, 2021
 *      Author: pupi
 */

#ifndef FLOW_CONTROL_MESSAGES_TYPES_H_
#define FLOW_CONTROL_MESSAGES_TYPES_H_

#include "pdcp_flow_control.h"

#define FC_TBS_UPDATE(mSGpTR)	(mSGpTR)->ittiMsg.fc_tbs_update
#define FC_TBS_X2U(mSGpTR)		(mSGpTR)->ittiMsg.fc_tbs_x2u




#endif /* FLOW_CONTROL_MESSAGES_TYPES_H_ */
