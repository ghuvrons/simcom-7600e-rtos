/*
 * net.h
 *
 *  Created on: Nov 9, 2022
 *      Author: janoko
 */

#ifndef SIMCOM_7600E_NET_H_
#define SIMCOM_7600E_NET_H_

#include "types.h"
#include "conf.h"


#define SIM_NET_STATUS_APN_WAS_SET      0x08
#define SIM_NET_STATUS_GPRS_REGISTERED  0x10
#define SIM_NET_STATUS_GPRS_ROAMING     0x20
#define SIM_NET_STATUS_NTP_WAS_SET      0x40
#define SIM_NET_STATUS_NTP_WAS_SYNCED   0x80

enum {
  SIM_NET_STATE_NON_ACTIVE,
  SIM_NET_STATE_SETUP_APN,
  SIM_NET_STATE_CHECK_GPRS,
  SIM_NET_STATE_ONLINE,
};

typedef struct {
  void *hsim;         // SIM_HandlerTypeDef
  uint8_t status;
  uint8_t state;
  uint8_t events;

  uint8_t setState;

  struct {
    char *APN;
    char *user;
    char *pass;
  } APN;

  void (*onOpening)(void);
  void (*onOpened)(void);
  void (*onOpenError)(void);
  void (*onClosed)(void);

  uint8_t gprs_status;
} SIM_NET_HandlerTypeDef;


SIM_Status_t SIM_NET_Init(SIM_NET_HandlerTypeDef*, void *hsim);
void         SIM_NET_SetupAPN(SIM_NET_HandlerTypeDef*, char *APN, char *user, char *pass);

void         SIM_NET_SetState(SIM_NET_HandlerTypeDef*, uint8_t newState);
SIM_Status_t SIM_NET_CheckState(SIM_NET_HandlerTypeDef*);
SIM_Status_t SIM_NET_GPRS_Check(SIM_NET_HandlerTypeDef*);
SIM_Status_t SIM_NET_SetAPN(SIM_NET_HandlerTypeDef*, char *APN, char *user, char *pass);

#endif /* SIMCOM_7600E_NET_H_ */
