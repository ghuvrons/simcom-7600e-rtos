/*
 * events.h
 *
 *  Created on: Nov 9, 2022
 *      Author: janoko
 */

#ifndef SIMCOM_7600E_EVENTS_H
#define SIMCOM_7600E_EVENTS_H

#define SIM_RTOS_EVT_READY      0x10U
#define SIM_RTOS_EVT_NEW_STATE  0x20U
#define SIM_RTOS_EVT_ACTIVED    0x40U
// net
#define SIM_RTOS_EVT_NET_NEW_STATE  0x0080U
//socket
#define SIM_RTOS_EVT_SOCKMGR_NEW_STATE  0x0100U
#define SIM_RTOS_EVT_SOCKCLIENT_NEW_EVT 0x0200U
// ntp
#define SIM_RTOS_EVT_NTP_SYNCED     0x0400U
//socket
#define SIM_RTOS_EVT_GPS_NEW_STATE  0x0800U

#define SIM_RTOS_AVT_ALL  SIM_RTOS_EVT_READY|SIM_RTOS_EVT_NEW_STATE|SIM_RTOS_EVT_ACTIVED|\
                          SIM_RTOS_EVT_NET_NEW_STATE|\
                          SIM_RTOS_EVT_SOCKMGR_NEW_STATE|SIM_RTOS_EVT_SOCKCLIENT_NEW_EVT|\
                          SIM_RTOS_EVT_NTP_SYNCED|\
                          SIM_RTOS_EVT_GPS_NEW_STATE


#endif /* SIMCOM_7600E_EVENTS_H */
