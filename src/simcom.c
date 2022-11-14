/*
 * simcom.c
 *
 *  Created on: Nov 7, 2022
 *      Author: janoko
 */

#include "include/simcom.h"
#include "include/simcom/core.h"
#include "include/simcom/net.h"
#include "include/simcom/utils.h"
#include "events.h"
#include <stdlib.h>

#define IS_EVENT(evt_notif, evt_wait) SIM_BITS_IS(evt_notif, evt_wait)

static SIM_Status_t checkState(SIM_HandlerTypeDef*);
static void onReady(void *app, AT_Data_t*);


SIM_Status_t SIM_Init(SIM_HandlerTypeDef *hsim)
{
  if (hsim->getTick == 0) return SIM_ERROR;
  if (hsim->delay == 0) return SIM_ERROR;

  hsim->atCmd.serial.readline = hsim->serial.readline;
  hsim->network_status = 0;

  hsim->atCmd.serial.read     = hsim->serial.read;
  hsim->atCmd.serial.readinto = hsim->serial.readinto;
  hsim->atCmd.serial.readline = hsim->serial.readline;
  hsim->atCmd.serial.write    = hsim->serial.write;

  hsim->atCmd.rtos.mutexLock    = hsim->rtos.mutexLock;
  hsim->atCmd.rtos.mutexUnlock  = hsim->rtos.mutexUnlock;
  hsim->atCmd.rtos.eventSet     = hsim->rtos.eventSet;
  hsim->atCmd.rtos.eventWait    = hsim->rtos.eventWait;
  hsim->atCmd.rtos.eventClear   = hsim->rtos.eventClear;

  AT_Config_t config;
  config.timeout = 30000; // ms

  if (AT_Init(&hsim->atCmd, &config) != AT_OK) return SIM_ERROR;

  AT_On(&hsim->atCmd, "RDY", hsim, 0, 0, onReady);

  hsim->key = SIM_KEY;

  SIM_NET_Init(&hsim->net, hsim);
  SIM_NTP_Init(&hsim->ntp, hsim);
  SIM_SockManager_Init(&hsim->socketManager, hsim);

  return SIM_OK;
}

// SIMCOM Application Threads
void SIM_Thread_Run(SIM_HandlerTypeDef *hsim)
{
  uint32_t notifEvent;
  uint32_t timeout = 2000; // ms
  uint32_t lastTO = 0;

  for (;;) {
    if (hsim->rtos.eventWait(SIM_RTOS_AVT_ALL, &notifEvent, timeout) == AT_OK) {
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_READY)) {

      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_NEW_STATE)) {
        checkState(hsim);
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_ACTIVED)) {
        SIM_NET_SetState(&hsim->net, SIM_NET_STATE_ONLINE);
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_NET_NEW_STATE)) {
        SIM_NET_CheckState(&hsim->net);
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_NET_ONLINE)) {
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_SOCKMGR_NEW_STATE)) {
        SIM_SockManager_OnNewState(&hsim->socketManager);
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_SOCKCLIENT_NEW_EVT)) {
        SIM_SockManager_CheckSocketsEvents(&hsim->socketManager);
      }
      if (IS_EVENT(notifEvent, SIM_RTOS_EVT_NTP_SYNCED)) {
        SIM_NTP_OnSynced(&hsim->ntp);
      }
      goto next;
    }

    lastTO = hsim->getTick();

    checkState(hsim);
    SIM_NET_CheckState(&hsim->net);
    SIM_SockManager_Loop(&hsim->socketManager);
    SIM_NTP_Loop(&hsim->ntp);

  next:
    timeout = 2000 - (hsim->getTick() - lastTO);
    if (timeout > 2000) timeout = 1;
  }
}


// AT Command Threads
void SIM_Thread_ATCHandler(SIM_HandlerTypeDef *hsim)
{
  AT_Process(&hsim->atCmd);
}

void SIM_SetState(SIM_HandlerTypeDef *hsim, uint8_t newState)
{
  hsim->setState = newState;
  hsim->rtos.eventSet(SIM_RTOS_EVT_NEW_STATE);
}

static SIM_Status_t checkState(SIM_HandlerTypeDef *hsim)
{
  // if simcom not active yet
  if (hsim->state == hsim->setState) return SIM_OK;

  switch (hsim->state) {
  case SIM_STATE_NON_ACTIVE:
  case SIM_STATE_CHECK_AT:
    if (SIM_CheckAT(hsim) != SIM_OK) {
      return SIM_ERROR;
    }
    break;

  case SIM_STATE_CHECK_SIMCARD:
    SIM_Debug("Checking SIM Card....");
    if (SIM_CheckSIMCard(hsim) == SIM_OK) {
      SIM_Debug("SIM card OK");
    } else {
      SIM_Debug("SIM card Not Ready");
      return SIM_ERROR;
    }
    break;

  case SIM_STATE_CHECK_NETWORK:
    SIM_Debug("Checking cellular network....");
    if (SIM_CheckNetwork(hsim) == SIM_OK) {
      SIM_Debug("Cellular network registered", (hsim->network_status == 5)? " (roaming)":"");
      hsim->rtos.eventSet(SIM_RTOS_EVT_NEW_STATE);
    }
    else if (hsim->network_status == 0) {
      SIM_ReqisterNetwork(hsim);
    }
    else if (hsim->network_status == 2) {
      SIM_Debug("Searching network....");
      return SIM_ERROR;
    }
    break;

  default: break;
  }

  if (hsim->state != hsim->setState) {
    hsim->rtos.eventSet(SIM_RTOS_EVT_NEW_STATE);
    return SIM_ERROR;
  }

  if (hsim->state == SIM_STATE_ACTIVE) {
    hsim->rtos.eventSet(SIM_RTOS_EVT_ACTIVED);
  }

  return SIM_OK;
}

static void onReady(void *app, AT_Data_t *_)
{
  SIM_HandlerTypeDef *hsim = (SIM_HandlerTypeDef*)app;

  hsim->status  = 0;
  hsim->events  = 0;
  SIM_Debug("Starting...");

  SIM_SetState(app, SIM_STATE_ACTIVE);
}
