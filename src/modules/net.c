/*
 * net.h
 *
 *  Created on: Nov 9, 2022
 *      Author: janoko
 */

#include "../events.h"
#include "../include/simcom.h"
#include "../include/simcom/net.h"
#include "../include/simcom/utils.h"
#include <stdlib.h>
#include <string.h>


SIM_Status_t SIM_NET_Init(SIM_NET_HandlerTypeDef *hsimnet, void *hsim)
{
  if (((SIM_HandlerTypeDef*)hsim)->key != SIM_KEY)
    return SIM_ERROR;

  hsimnet->hsim         = hsim;
  hsimnet->status       = 0;
  hsimnet->events       = 0;
  hsimnet->gprs_status  = 0;
  hsimnet->state        = hsimnet->setState = SIM_NET_STATE_NON_ACTIVE;

  return SIM_OK;
}


void SIM_NET_SetState(SIM_NET_HandlerTypeDef *hsimnet, uint8_t newState)
{
  hsimnet->setState = newState;
  ((SIM_HandlerTypeDef*) hsimnet->hsim)->rtos.eventSet(SIM_RTOS_EVT_NET_NEW_STATE);
}


void SIM_NET_SetupAPN(SIM_NET_HandlerTypeDef *hsimnet, char *APN, char *user, char *pass)
{
  hsimnet->APN.APN   = APN;
  hsimnet->APN.user  = 0;
  hsimnet->APN.pass  = 0;

  if (strlen(user) > 0)
    hsimnet->APN.user = user;
  if (strlen(pass) > 0)
    hsimnet->APN.pass = pass;
}

SIM_Status_t SIM_NET_CheckState(SIM_NET_HandlerTypeDef *hsimnet)
{
  SIM_HandlerTypeDef *hsim = hsimnet->hsim;

  if (hsimnet->state == hsimnet->setState) return SIM_OK;

  switch (hsimnet->state) {
  case SIM_NET_STATE_NON_ACTIVE:
  case SIM_NET_STATE_SETUP_APN:
    if (hsimnet->APN.APN != NULL) {
      if (SIM_NET_SetAPN(hsimnet,
                         hsimnet->APN.APN,
                         hsimnet->APN.user,
                         hsimnet->APN.pass) != SIM_OK)
      {
        SIM_Debug("Checking GPRS...");
      }
    } else {
      hsimnet->state += 1;
    }
    break;

  case SIM_NET_STATE_CHECK_GPRS:
    SIM_Debug("Checking GPRS...");
    if (SIM_NET_GPRS_Check(hsimnet) == SIM_OK) {
      SIM_Debug("GPRS registered%s", (hsimnet->gprs_status == 5)? " (roaming)":"");
    }
    else if (hsimnet->gprs_status == 0) {
      hsimnet->state = SIM_NET_STATE_SETUP_APN;
      return SIM_ERROR;
    }
    else if (hsim->network_status == 2) {
      SIM_Debug("GPRS Registering....");
      return SIM_ERROR;
    } else {
      return SIM_ERROR;
    }
    break;

  default: break;
  }

  if (hsimnet->state != hsimnet->setState) {
    hsim->rtos.eventSet(SIM_RTOS_EVT_NET_NEW_STATE);
    return SIM_ERROR;
  }

  if (hsimnet->state == SIM_NET_STATE_ONLINE) {
    hsim->rtos.eventSet(SIM_RTOS_EVT_NET_ONLINE);
  }

  return SIM_OK;
}


SIM_Status_t SIM_NET_GPRS_Check(SIM_NET_HandlerTypeDef *hsimnet)
{
  SIM_HandlerTypeDef *hsim = hsimnet->hsim;
  SIM_Status_t status = SIM_ERROR;

  uint8_t lac[2]; // location area code
  uint8_t ci[2];  // Cell Identify

  AT_Data_t respData[4] = {
    AT_Number(0),
    AT_Number(0),
    AT_Hex(lac),
    AT_Hex(ci),
  };

  memset(lac, 0, 2);
  memset(ci, 0, 2);

  if (AT_Check(&hsim->atCmd, "+CGREG", 4, respData) != AT_OK) return status;
  hsimnet->gprs_status = (uint8_t) respData[1].value.number;

  // check response
  if (hsimnet->gprs_status == 1 || hsimnet->gprs_status == 5) {
    status = SIM_OK;
    if (hsimnet->state <= SIM_NET_STATE_CHECK_GPRS) {
      hsimnet->state = SIM_NET_STATE_CHECK_GPRS+1;
    }
    if (hsimnet->gprs_status == 5)
      SIM_SET_STATUS(hsimnet, SIM_NET_STATUS_GPRS_ROAMING);
  }
  else {
    if (hsimnet->state > SIM_NET_STATE_CHECK_GPRS)
      hsimnet->state = SIM_NET_STATE_CHECK_GPRS;
    SIM_UNSET_STATUS(hsimnet, SIM_NET_STATUS_GPRS_ROAMING);
  }

  return status;
}


SIM_Status_t SIM_NET_SetAPN(SIM_NET_HandlerTypeDef *hsimnet,
                            char *APN, char *user, char *pass)
{
  SIM_HandlerTypeDef *hsim = hsimnet->hsim;
  SIM_Status_t status = SIM_ERROR;
  uint8_t cid = 1;
  AT_Data_t paramData[4] = {
    AT_Number(cid),
    AT_String("IP", 2),
    AT_String(APN, strlen(APN)),
    AT_String(0, 0),
  };

  if (AT_Command(&hsim->atCmd, "+CGDCONT", 3, paramData, 0, 0) != AT_OK) goto endCmd;

  if (user == NULL) {
    AT_DataSetNumber(&paramData[1], 0);
    if (AT_Command(&hsim->atCmd, "+CGAUTH", 2, paramData, 0, 0) != AT_OK) goto endCmd;
  }
  else {
    AT_DataSetNumber(&paramData[1], 3);
    AT_DataSetString(&paramData[2], user, strlen(user));
    AT_DataSetString(&paramData[3], pass, strlen(pass));

    if (pass == NULL) {
      if (AT_Command(&hsim->atCmd, "+CGAUTH", 3, paramData, 0, 0) != AT_OK) goto endCmd;
    }
    else
      if (AT_Command(&hsim->atCmd, "+CGAUTH", 4, paramData, 0, 0) != AT_OK) goto endCmd;
  }

  SIM_SET_STATUS(hsimnet, SIM_NET_STATUS_APN_WAS_SET);
  hsimnet->state = SIM_NET_STATE_SETUP_APN+1;
  status = SIM_OK;

endCmd:
  return status;
}
